#!/usr/bin/env python3
"""Find signatures that match a specific PSY-Q version and NOT a competitor.
Useful for pinpointing the exact version when 4.0 vs later are both candidates.
"""
import json
import re
import sys
from pathlib import Path

REPO = Path("/tmp/psx_psyq_signatures")
BIN = Path(__file__).resolve().parent.parent / "bin" / "SLES_010.90"

SEGMENTS = {
    "LIBCD":      (473088, 498404),
    "LIBGPU":     (498404, 524896),
    "LIBSPU":     (524896, 528336),
    "LIBS_80FD0": (528336, 528364),
}

def sig_to_regex(sig: str):
    parts = []
    for tok in sig.split():
        if tok == "??":
            parts.append(b".")
        else:
            parts.append(re.escape(bytes.fromhex(tok)))
    return re.compile(b"".join(parts), re.DOTALL)

def load_version(ver: str):
    """Returns {lib_file: [(name, sig, regex, bytes_count), ...]}."""
    out = {}
    vdir = REPO / ver
    for f in sorted(vdir.glob("*.json")):
        entries = []
        try:
            data = json.loads(f.read_text())
        except Exception:
            continue
        for e in data:
            sig = e.get("sig", "")
            bytes_count = len([t for t in sig.split() if t != "??"])
            try:
                rx = sig_to_regex(sig)
            except Exception:
                continue
            entries.append((e["name"], sig, rx, bytes_count))
        out[f.name] = entries
    return out

def matches(blob: bytes, version_data) -> dict:
    """Returns {(libfile, name): start_offset}."""
    res = {}
    for libfile, entries in version_data.items():
        for name, sig, rx, _ in entries:
            m = rx.search(blob)
            if m:
                res[(libfile, name)] = m.start()
    return res

def main():
    data = BIN.read_bytes()
    blob = b""
    offsets = []
    cursor = 0
    for name, (start, end) in SEGMENTS.items():
        offsets.append((cursor, cursor + (end - start), name, start))
        blob += data[start:end] + b"\x00" * 32
        cursor = len(blob)

    def file_offset(blob_off):
        for blob_s, blob_e, seg, file_s in offsets:
            if blob_s <= blob_off < blob_e:
                return seg, file_s + (blob_off - blob_s)
        return "?", blob_off

    v_target = sys.argv[1] if len(sys.argv) > 1 else "400"
    v_compare = sys.argv[2] if len(sys.argv) > 2 else "410"

    print(f"Loading {v_target} and {v_compare}...")
    vd_tgt = load_version(v_target)
    vd_cmp = load_version(v_compare)

    m_tgt = matches(blob, vd_tgt)
    m_cmp = matches(blob, vd_cmp)

    # Compare by sig text — if same sig appears in both versions, the match
    # is uninformative. We want sigs that match only in v_target.
    def sig_index(vd):
        # (libfile, name) -> sig text
        return {k: s for libfile, entries in vd.items()
                for n, s, _, _ in entries
                for k in [(libfile, n)]}
    sig_t = sig_index(vd_tgt)
    sig_c = sig_index(vd_cmp)

    # All sig texts that match in v_target
    matched_sigs_tgt = {sig_t[k] for k in m_tgt}
    matched_sigs_cmp = {sig_c[k] for k in m_cmp}

    only_tgt = matched_sigs_tgt - matched_sigs_cmp
    only_cmp = matched_sigs_cmp - matched_sigs_tgt
    shared = matched_sigs_tgt & matched_sigs_cmp

    print(f"\nMatched in v{v_target}: {len(matched_sigs_tgt)} unique sigs")
    print(f"Matched in v{v_compare}: {len(matched_sigs_cmp)} unique sigs")
    print(f"Shared (same sig text):     {len(shared)}")
    print(f"Only in v{v_target}:        {len(only_tgt)}")
    print(f"Only in v{v_compare}:       {len(only_cmp)}")

    # Print first 30 names from "only in target"
    if only_tgt:
        print(f"\n=== Signatures matching ONLY v{v_target} (the discriminators) ===")
        names = sorted({f"{lf:<22} {n}" for (lf, n) in m_tgt if sig_t[(lf, n)] in only_tgt})
        for n in names[:40]:
            print(f"  {n}")
        if len(names) > 40:
            print(f"  ... +{len(names) - 40} more")

    if only_cmp:
        print(f"\n=== Signatures matching ONLY v{v_compare} (=> game closer to {v_compare}) ===")
        names = sorted({f"{lf:<22} {n}" for (lf, n) in m_cmp if sig_c[(lf, n)])] in only_cmp})
        for n in names[:40]:
            print(f"  {n}")
        if len(names) > 40:
            print(f"  ... +{len(names) - 40} more")

if __name__ == "__main__":
    main()
