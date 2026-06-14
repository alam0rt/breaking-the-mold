#!/usr/bin/env python3
"""Lightweight linter for PSX decompilation project.

Checks:
1. Struct/typedef in .c files that already exist in include/
2. INCLUDE_ASM paths that don't match an actual .s file
3. Duplicate function definitions across .c files
"""

import os
import re
import sys
from pathlib import Path

SRC_DIR = Path("src")
INCLUDE_DIR = Path("include")
ASM_DIR = Path("asm/nonmatchings")

warnings = []


def warn(path, line, msg):
    warnings.append(f"{path}:{line}: {msg}")


def collect_header_structs():
    """Find all struct/typedef names defined in include/."""
    names = set()
    pattern = re.compile(r"typedef\s+struct\s+(\w+)")
    for f in INCLUDE_DIR.rglob("*.h"):
        for line in f.read_text(errors="replace").splitlines():
            m = pattern.search(line)
            if m:
                names.add(m.group(1))
    return names


def check_struct_redefinitions(header_structs):
    """Warn if a .c file redefines a struct from headers."""
    pattern = re.compile(r"typedef\s+struct\s+(\w+)")
    for f in SRC_DIR.rglob("*.c"):
        lines = f.read_text(errors="replace").splitlines()
        for i, line in enumerate(lines, 1):
            m = pattern.search(line)
            if m and m.group(1) in header_structs:
                # Allow suppression with "// lint:ok" on same or previous line
                prev = lines[i - 2] if i >= 2 else ""
                if "lint:ok" in line or "lint:ok" in prev:
                    continue
                warn(f, i, f"struct '{m.group(1)}' already defined in include/")


def check_include_asm_paths():
    """Warn if INCLUDE_ASM references a nonexistent .s file."""
    pattern = re.compile(r'INCLUDE_ASM\(\s*"([^"]+)"\s*,\s*(\w+)\s*\)')
    for f in SRC_DIR.rglob("*.c"):
        for i, line in enumerate(f.read_text(errors="replace").splitlines(), 1):
            m = pattern.search(line)
            if m:
                asm_path = Path(m.group(1)) / (m.group(2) + ".s")
                if not asm_path.exists():
                    warn(f, i, f"INCLUDE_ASM target missing: {asm_path}")


def check_duplicate_functions():
    """Warn if the same function name is defined in multiple .c files."""
    # Match function definitions (name followed by parens and opening brace)
    pattern = re.compile(r"^(\w+)\s*\([^)]*\)\s*\{", re.MULTILINE)
    func_locations = {}
    for f in SRC_DIR.rglob("*.c"):
        text = f.read_text(errors="replace")
        for m in pattern.finditer(text):
            name = m.group(1)
            if name in ("if", "while", "for", "switch", "return", "typedef", "struct"):
                continue
            func_locations.setdefault(name, []).append(f)
    for name, files in func_locations.items():
        unique = list(set(str(f) for f in files))
        if len(unique) > 1:
            warn(unique[0], 0, f"function '{name}' also defined in {unique[1]}")


def main():
    if not SRC_DIR.exists():
        print("Error: src/ not found", file=sys.stderr)
        return 1

    header_structs = collect_header_structs()
    check_struct_redefinitions(header_structs)
    check_include_asm_paths()
    check_duplicate_functions()

    if warnings:
        for w in sorted(warnings):
            print(w)
        print(f"\n{len(warnings)} warning(s)")
        return 1
    else:
        print("No issues found.")
        return 0


if __name__ == "__main__":
    sys.exit(main())
