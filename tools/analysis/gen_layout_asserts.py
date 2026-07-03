#!/usr/bin/env python3
"""Generate machine-checked struct-layout assertions from the /* 0xNN */ offset
and /* Size: 0xNN */ comments in include/Game/*.h.

Why this exists
---------------
Struct layouts in this decomp are documented only in comments:

    /* 0x68 */ s16 worldX;        /* ... */
    ...
    } PlayerEntity;  /* Size: 0x1B4 (436 bytes) */

Comments don't compile, so a header refactor can silently shift a field or
resize a struct and you won't know until the SHA1 mismatches after a full build.
This tool turns those comments into `_Static_assert(offsetof(...))` /
`_Static_assert(sizeof(...))` invariants, one assertion TU per header, checked
by a modern compiler under the PSX ABI (`-target mipsel-unknown-none-elf`,
4-byte pointers/long, `-fno-short-enums`). See tools/analysis/README.md.

This is part of the *analysis build* only. It never feeds the matching build
and never touches the produced bytes.

Design notes (all verified against the real headers, 2026-07):
  * Structs are flat except two anonymous inline `struct { ... } field;` members
    (editor_entities.h, sprite_records.h); fields *inside* those inner structs
    are skipped (their names aren't reachable by a single offsetof).
  * No PSY-Q GPU type (SPRT/POLY_*/...) is used by value, so sizeof/offsetof are
    computable with no psyq/ headers present.
  * 5 typedef names collide across headers (SpriteContext, SpriteRenderContext,
    DecorSpawnData, HazardTimerEntity, SpriteContextCallbackTable) -> one TU per
    header keeps them isolated.
  * common.h is prepended so fsm_dispatch.h (which omits it and uses raw s16)
    can still be asserted; its standalone-compile bug is reported elsewhere.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", ".."))
GAME_DIR = os.path.join(REPO, "include", "Game")
OUT_DIR = os.path.join(HERE, "asserts")

# /* 0xNN */ at the start of a field line -> capture the hex offset.
OFFSET_RE = re.compile(r"/\*\s*0x([0-9A-Fa-f]+)\s*\*/")
# Aggregate/enum opener. We must track enum and union bodies too so their braces
# balance and their `} Name;` closers don't steal a struct frame pop. Captures
# the kind (struct/union/enum) and the optional tag (`struct Entity {`).
OPEN_RE = re.compile(r"\b(?:typedef\s+)?(struct|union|enum)\b\s*([A-Za-z_]\w*)?[^{;]*\{")
# Aggregate closer:  } Name;  (typedef name)  or  };  (tag-only definition).
CLOSE_RE = re.compile(r"^\s*\}\s*([A-Za-z_]\w*)?\s*;")
# Size comment, two observed spellings:
#   /* Size: 0x1B4 (436 bytes) */      and      /* 0x70 = 112 bytes */
SIZE_RE = re.compile(r"/\*\s*(?:Size:\s*)?0x([0-9A-Fa-f]+)\s*(?:=|\()")
# A trailing /* ... */ comment on a field line (strip before name extraction).
TRAILING_COMMENT_RE = re.compile(r"/\*.*?\*/")
# Function-pointer declarator:  RET (*name)(args)
FNPTR_RE = re.compile(r"\(\s*\*\s*([A-Za-z_]\w*)\s*\)")


def strip_line_comments(text):
    """Remove /* ... */ comments from a single declarator fragment."""
    return TRAILING_COMMENT_RE.sub("", text)


def extract_field_name(decl):
    """Given a field declarator (no leading offset comment, no trailing comment,
    no semicolon), return the member identifier, or None if it can't be named
    (anonymous inner struct opener, blank, etc.)."""
    decl = decl.strip()
    if not decl or decl.endswith("{"):
        return None  # anonymous inline struct opener: `struct {`
    # Function pointer: the name lives inside (*name).
    m = FNPTR_RE.search(decl)
    if m:
        return m.group(1)
    # Strip array subscripts: `_pad80[8]` -> `_pad80`, `a[2][3]` -> `a`.
    decl = re.sub(r"\[[^\]]*\]", "", decl)
    # The member name is the last C identifier token (after type/`*` qualifiers).
    idents = re.findall(r"[A-Za-z_]\w*", decl)
    if not idents:
        return None
    return idents[-1]


def parse_header(path):
    """Return a list of structs: [{name, size, fields:[(off,name)]}] in file
    order.

    Uses a kind-aware brace stack so enum/union bodies balance correctly and
    their `} Name;` closers don't steal a struct frame pop. Only `struct`
    frames at the outer nesting level (depth 1) produce assertions; fields of
    nested/anonymous inner structs are skipped (their names aren't reachable by
    a single offsetof against the outer type).
    """
    structs = []
    frame_stack = []  # each: {kind, tag, depth, fields:[(off,name)], anon_offset}
    depth = 0

    def close_frame(name_from_closer, size_line):
        nonlocal depth
        frame = frame_stack.pop()
        depth -= 1
        if frame["kind"] != "struct":
            return  # enum/union: balanced, but never asserted
        # Prefer the typedef name on the closer; fall back to the opener tag
        # (e.g. `struct Entity { ... };` closes with a bare `};`).
        name = name_from_closer or frame["tag"]
        if frame["depth"] == 1:
            size = None
            sm = SIZE_RE.search(size_line)
            if sm:
                size = int(sm.group(1), 16)
            if name:
                structs.append(
                    {"name": name, "size": size, "fields": frame["fields"]}
                )
        elif frame_stack and frame["anon_offset"] is not None and name:
            # Anonymous inner struct exposed as a named field of its parent.
            frame_stack[-1]["fields"].append((frame["anon_offset"], name))

    with open(path, "r") as fh:
        for raw in fh:
            line = raw.rstrip("\n")

            om_open = OPEN_RE.search(line)
            cm = CLOSE_RE.match(line)

            # --- single-line aggregate: `typedef struct { ... } Name;` ---
            if om_open and "}" in line[om_open.end():]:
                # Opens and closes on one line -> no /* 0xNN */ fields to mine.
                # Nothing to assert; do not perturb the frame stack.
                continue

            # --- aggregate close ---
            if cm and frame_stack:
                close_frame(cm.group(1), line)
                continue

            # --- aggregate open ---
            if om_open:
                depth += 1
                anon_off = None
                off_match = OFFSET_RE.search(line)
                if off_match and frame_stack:
                    anon_off = int(off_match.group(1), 16)
                frame_stack.append({
                    "kind": om_open.group(1),
                    "tag": om_open.group(2),
                    "depth": depth,
                    "fields": [],
                    "anon_offset": anon_off,
                })
                continue

            # --- field line: only outer-level struct fields ---
            top = frame_stack[-1] if frame_stack else None
            if top and top["kind"] == "struct" and top["depth"] == 1:
                om = OFFSET_RE.match(line.lstrip())
                if not om:
                    continue
                off = int(om.group(1), 16)
                rest = line[line.index("*/", line.index("/*")) + 2 :]
                rest = strip_line_comments(rest)
                rest = rest.split(";")[0]
                fname = extract_field_name(rest)
                if fname:
                    top["fields"].append((off, fname))

    return structs


def emit_assert_tu(header_name, structs):
    """Return C source for one header's assertion TU."""
    guard = "GEN_" + re.sub(r"\W", "_", header_name).upper()
    lines = [
        "/* AUTO-GENERATED by tools/analysis/gen_layout_asserts.py -- DO NOT EDIT.",
        " * Layout invariants for include/Game/%s (analysis build only)." % header_name,
        " */",
        "#include \"common.h\"",
        "#include <stddef.h>            /* offsetof */",
        "#include \"Game/%s\"" % header_name,
        "",
        "/* Sanity: this TU must be compiled under the PSX ABI (4-byte pointers). */",
        "_Static_assert(sizeof(void *) == 4, \"analysis build must target mipsel LP32\");",
        "_Static_assert(sizeof(long) == 4, \"analysis build must target mipsel LP32\");",
        "",
    ]
    total_size = 0
    total_off = 0
    for s in structs:
        lines.append("/* --- %s --- */" % s["name"])
        if s["size"] is not None:
            lines.append(
                "_Static_assert(sizeof(%s) == 0x%X, \"%s: %s size drift\");"
                % (s["name"], s["size"], header_name, s["name"])
            )
            total_size += 1
        for off, fname in s["fields"]:
            lines.append(
                "_Static_assert(offsetof(%s, %s) == 0x%X, \"%s: %s.%s offset drift\");"
                % (s["name"], fname, off, header_name, s["name"], fname)
            )
            total_off += 1
        lines.append("")
    lines.append("/* generated: %d size asserts, %d offset asserts */"
                 % (total_size, total_off))
    lines.append("")
    return "\n".join(lines), total_size, total_off


def main():
    if not os.path.isdir(GAME_DIR):
        sys.exit("no include/Game dir at %s" % GAME_DIR)
    os.makedirs(OUT_DIR, exist_ok=True)

    headers = sorted(f for f in os.listdir(GAME_DIR) if f.endswith(".h"))
    grand_size = grand_off = grand_structs = 0
    emitted = []
    for h in headers:
        structs = parse_header(os.path.join(GAME_DIR, h))
        if not structs:
            continue
        src, ns, no = emit_assert_tu(h, structs)
        out_path = os.path.join(OUT_DIR, h.replace(".h", ".asserts.c"))
        with open(out_path, "w") as fh:
            fh.write(src)
        emitted.append(os.path.basename(out_path))
        grand_structs += len(structs)
        grand_size += ns
        grand_off += no
        print("  %-28s %2d structs  %2d size  %3d offset"
              % (h, len(structs), ns, no))

    print("\ngenerated %d assertion TUs into %s" % (len(emitted), OUT_DIR))
    print("totals: %d structs, %d size asserts, %d offset asserts"
          % (grand_structs, grand_size, grand_off))


if __name__ == "__main__":
    main()
