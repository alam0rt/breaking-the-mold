#!/usr/bin/env python3
"""
Match Skullmonkeys's PSY-Q library segments against lab313ru's
psx_psyq_signatures to identify the exact PSY-Q toolchain version used.

Usage: tools/psyq-version-detect.py [SIG_REPO]

Default SIG_REPO = /tmp/psx_psyq_signatures.
"""
import json
import os
import re
import sys
from pathlib import Path

REPO = Path(sys.argv[1] if len(sys.argv) > 1 else "/tmp/psx_psyq_signatures")
BIN = Path(__file__).resolve().parent.parent / "bin" / "SLES_010.90"

# File offsets of the PSY-Q library text segments from SLES_010.90.yaml
SEGMENTS = {
    "LIBCD":      (473088, 498404),
    "LIBGPU":     (498404, 524896),
    "LIBSPU":     (524896, 528336),
    "LIBS_80FD0": (528336, 528364),
}

VERSIONS = ["260", "300", "330", "340", "350", "3610", "3611", "370",
            "400", "410", "420", "430", "440", "450", "460", "470"]

def sig_to_regex(sig: str) -> re.Pattern:
    """'AB CD ?? EF ' -> compiled regex over raw bytes."""
    parts = []
    for tok in sig.split():
        if tok == "??":
            parts.append(b".")
        else:
            parts.append(re.escape(bytes.fromhex(tok)))
    return re.compile(b"".join(parts), re.DOTALL)

def main():
    data = BIN.read_bytes()
    # Concatenate the library ranges into one search blob (with sentinel
    # so signatures don't accidentally cross segment boundaries).
    blob = b""
    for name, (start, end) in SEGMENTS.items():
        blob += data[start:end] + b"\x00" * 32

    print(f"Binary:    {BIN}")
    print(f"Sig repo:  {REPO}")
    print(f"Library text bytes: {sum(e - s for s, e in SEGMENTS.values())}")
    print()
    print(f"{'version':<8} {'matched':>8} {'unique':>8} {'libfiles':>9}")
    print(f"{'-'*8} {'-'*8} {'-'*8} {'-'*9}")

    by_version = {}
    for ver in VERSIONS:
        vdir = REPO / ver
        if not vdir.is_dir():
            continue
        n_total = 0
        n_match = 0
        unique_names = set()
        libfiles = sorted(vdir.glob("*.json"))
        for libfile in libfiles:
            try:
                entries = json.loads(libfile.read_text())
            except Exception as e:
                continue
            for e in entries:
                sig = e.get("sig", "")
                # Skip trivial sigs (jump table stubs, tiny BIOS thunks).
                bytes_count = len([t for t in sig.split() if t != "??"])
                if bytes_count < 16:
                    continue
                n_total += 1
                try:
                    rx = sig_to_regex(sig)
                except Exception:
                    continue
                if rx.search(blob):
                    n_match += 1
                    unique_names.add(e["name"])
        by_version[ver] = (n_match, n_total, len(unique_names), len(libfiles))
        print(f"{ver:<8} {n_match:>4}/{n_total:<3} {len(unique_names):>8} {len(libfiles):>9}")

    print()
    # Pretty-print as percentage
    print("Match rate (matched / scanned signatures with ≥16 fixed bytes):")
    print(f"{'version':<8} {'rate':>7}")
    for ver, (m, t, u, f) in sorted(by_version.items(), key=lambda kv: -kv[1][0]):
        rate = (m / t * 100) if t else 0
        print(f"  {ver:<6} {rate:>6.2f}%  ({m}/{t})")

if __name__ == "__main__":
    main()
