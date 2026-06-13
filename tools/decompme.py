#!/usr/bin/env python3
"""Create a decomp.me scratch for a function from this project.

By default this reads from ``nonmatchings/<FUNC>/`` (the layout produced by
``decomp-permuter``'s ``import.py``) and POSTs it to https://decomp.me, then
prints the resulting scratch URL plus a claim URL so you can take ownership
of the scratch from your browser.

Inputs (all overridable via flags or environment variables):

  source:        nonmatchings/<FUNC>/base.c       --source / DECOMPME_SRC
  target asm:    nonmatchings/<FUNC>/target.s     --asm    / DECOMPME_ASM
  context file:  (empty)                          --context / DECOMPME_CONTEXT
  compiler:      psyq4.0                          --compiler / DECOMPME_COMPILER
  flags:         -O2 -G8 -fno-builtin
                 -mno-abicalls -mcpu=3000         --flags  / DECOMPME_FLAGS
  api base:      https://decomp.me                --api    / DECOMPME_API

The source is lightly cleaned: any ``PERM_RANDOMIZE(...)`` wrappers inserted
by decomp-permuter are stripped so the code compiles on decomp.me unchanged.

Anonymous scratches work; no auth is needed. The returned ``claim_token`` is
shown in the printed claim URL — open it in your logged-in browser to attach
the scratch to your decomp.me account.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import sys
import urllib.error
import urllib.request
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent

DEFAULT_API = "https://decomp.me"
DEFAULT_COMPILER = "psyq4.0"
DEFAULT_FLAGS = "-O2 -G8 -fno-builtin -mno-abicalls -mcpu=3000"


_PERM_MACRO_RE = re.compile(r"\bPERM_[A-Z_]+\s*\(")


def _split_args_top(text: str) -> list[str]:
    """Split a parenthesised argument list on top-level commas."""
    args: list[str] = []
    depth = 0
    start = 0
    for i, c in enumerate(text):
        if c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
        elif c == "," and depth == 0:
            args.append(text[start:i])
            start = i + 1
    args.append(text[start:])
    return args


def strip_perm_randomize(source: str) -> str:
    """Expand decomp-permuter macros into something decomp.me can compile.

    Replaces ``PERM_RANDOMIZE(body)`` with ``body`` and any other
    ``PERM_XXX(opt1, opt2, ...)`` with ``opt1`` (the first alternative).
    Handles nesting via paren-counting.
    """
    out = []
    i = 0
    while True:
        m = _PERM_MACRO_RE.search(source, i)
        if not m:
            out.append(source[i:])
            break
        out.append(source[i : m.start()])
        j = m.end()
        depth = 1
        start = j
        while j < len(source) and depth > 0:
            c = source[j]
            if c == "(":
                depth += 1
            elif c == ")":
                depth -= 1
                if depth == 0:
                    break
            j += 1
        if depth != 0:
            out.append(source[m.start() :])
            break
        args = _split_args_top(source[start:j])
        out.append(args[0])
        i = j + 1  # skip closing ')'
    return "".join(out)


def read_text(path: Path) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except OSError as e:
        sys.exit(f"error: cannot read {path}: {e}")


def strip_asm_prelude(asm: str, func: str) -> str:
    """Drop decomp-permuter's macro/register prelude from ``target.s``.

    The permuter writes its own ``glabel``/register-alias macros plus
    ``.set gp=64`` at the top of ``target.s`` so the file can be reassembled
    locally. decomp.me prepends its own platform-specific prelude during
    scratch creation, so feeding it ours produces "macro already defined"
    and "gp=64 on a 32-bit processor" errors. Cut everything before the
    first ``glabel <func>`` (or first ``glabel`` if the name is missing).
    """
    lines = asm.splitlines(keepends=True)
    target = f"glabel {func}"
    cut = None
    for i, line in enumerate(lines):
        stripped = line.lstrip()
        if stripped.startswith(target):
            cut = i
            break
    if cut is None:
        for i, line in enumerate(lines):
            if line.lstrip().startswith("glabel "):
                cut = i
                break
    if cut is None:
        return asm
    return "".join(lines[cut:])


def post_scratch(api_base: str, payload: dict) -> dict:
    url = api_base.rstrip("/") + "/api/scratch"
    body = json.dumps(payload).encode("utf-8")
    req = urllib.request.Request(
        url,
        data=body,
        method="POST",
        headers={
            "Content-Type": "application/json",
            "Accept": "application/json",
            # decomp.me bins anonymous bot UAs through Cloudflare; pretend
            # to be a normal browser-style client.
            "User-Agent": "btm-decompme/1.0 (https://github.com/decompme/decomp.me)",
        },
    )
    try:
        with urllib.request.urlopen(req, timeout=30) as resp:
            return json.loads(resp.read().decode("utf-8"))
    except urllib.error.HTTPError as e:
        detail = e.read().decode("utf-8", errors="replace")
        sys.exit(f"error: decomp.me returned HTTP {e.code}\n{detail}")
    except urllib.error.URLError as e:
        sys.exit(f"error: cannot reach {url}: {e}")


def main(argv: list[str] | None = None) -> int:
    p = argparse.ArgumentParser(
        description="Create a decomp.me scratch from nonmatchings/<FUNC>/."
    )
    p.add_argument("func", help="function name (e.g. func_80019F88)")
    p.add_argument(
        "--dir",
        help=(
            "directory containing base.c/target.s. Defaults to "
            "nonmatchings/<FUNC> (falls back to nonmatchings/<FUNC>-rand)."
        ),
    )
    p.add_argument(
        "--source",
        default=os.environ.get("DECOMPME_SRC"),
        help="path to source C file (default: <dir>/base.c)",
    )
    p.add_argument(
        "--asm",
        default=os.environ.get("DECOMPME_ASM"),
        help="path to target asm file (default: <dir>/target.s)",
    )
    p.add_argument(
        "--context",
        default=os.environ.get("DECOMPME_CONTEXT", ""),
        help="path to context C file (default: empty)",
    )
    p.add_argument(
        "--compiler",
        default=os.environ.get("DECOMPME_COMPILER", DEFAULT_COMPILER),
        help=f"decomp.me compiler id (default: {DEFAULT_COMPILER})",
    )
    p.add_argument(
        "--flags",
        default=os.environ.get("DECOMPME_FLAGS", DEFAULT_FLAGS),
        help=f"compiler flags (default: {DEFAULT_FLAGS!r})",
    )
    p.add_argument(
        "--api",
        default=os.environ.get("DECOMPME_API", DEFAULT_API),
        help=f"decomp.me API base URL (default: {DEFAULT_API})",
    )
    p.add_argument(
        "--name",
        help="scratch name (defaults to FUNC)",
    )
    p.add_argument(
        "--dry-run",
        action="store_true",
        help="show the payload that would be sent and exit",
    )
    args = p.parse_args(argv)

    # Resolve the per-function directory.
    if args.dir:
        func_dir = Path(args.dir)
    else:
        candidates = [
            REPO_ROOT / "nonmatchings" / args.func,
            REPO_ROOT / "nonmatchings" / f"{args.func}-rand",
        ]
        for c in candidates:
            if c.is_dir():
                func_dir = c
                break
        else:
            sys.exit(
                f"error: no nonmatchings dir found for {args.func} "
                f"(tried: {', '.join(str(c) for c in candidates)})"
            )

    src_path = Path(args.source) if args.source else func_dir / "base.c"
    asm_path = Path(args.asm) if args.asm else func_dir / "target.s"

    if not src_path.is_file():
        sys.exit(f"error: source file not found: {src_path}")
    if not asm_path.is_file():
        sys.exit(f"error: target asm not found: {asm_path}")

    source_code = strip_perm_randomize(read_text(src_path))
    target_asm = strip_asm_prelude(read_text(asm_path), args.func)
    context = read_text(Path(args.context)) if args.context else ""

    payload = {
        "name": args.name or args.func,
        "compiler": args.compiler,
        "platform": "ps1",
        "compiler_flags": args.flags,
        "source_code": source_code,
        "context": context,
        "target_asm": target_asm,
        "diff_label": args.func,
    }

    if args.dry_run:
        meta = {k: v if k not in ("source_code", "target_asm", "context")
                else f"<{len(v)} bytes>" for k, v in payload.items()}
        print(json.dumps(meta, indent=2))
        return 0

    print(f"Creating scratch for {args.func} on {args.api} "
          f"(compiler={args.compiler})...", file=sys.stderr)
    data = post_scratch(args.api, payload)

    slug = data.get("slug")
    if not slug:
        sys.exit(f"error: unexpected response: {data}")

    base = args.api.rstrip("/")
    print(f"  scratch: {base}/scratch/{slug}")
    token = data.get("claim_token")
    if token:
        print(f"  claim:   {base}/scratch/{slug}/claim?token={token}")
    score = data.get("score", -1)
    max_score = data.get("max_score", -1)
    if score >= 0 and max_score >= 0:
        print(f"  score:   {score} / {max_score}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
