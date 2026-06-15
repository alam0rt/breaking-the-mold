#!/usr/bin/env python3
"""Crack the whole Skullmonkeys asset-id pool and maintain a resumable ledger.

Runs kcrack attacks against every known id and merges results into one CSV
(docs/analysis/asset-identification/cracked_names.csv). The CSV is the source
of truth: re-running merges new findings and NEVER clobbers rows whose status
is 'verified' or 'manual' (so visual confirmations / hand edits survive).

  python3 tools/scripts/crack_all.py --words /tmp/dict_all.txt
  python3 tools/scripts/crack_all.py --words W --combine POOL --depth 2

Status per id:
  verified  - confirmed name (seeded anchors, or hand-set status=verified/manual)
  likely    - exactly one clean wordlike candidate (len==floor): probably real
  ambiguous - several candidates hit this id (collisions): needs visual check
  uncracked - nothing found
"""
from __future__ import annotations
import argparse, csv, subprocess, collections, os, sys, tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SEED = 0x28C0E011
KCRACK = ROOT / "tools/kcrack"
LEDGER = ROOT / "docs/analysis/asset-identification/cracked_names.csv"
CATS = ROOT / "docs/analysis/asset-identification/asset_categories.csv"
MASTER = ROOT / "docs/reference/asset-ids-master.csv"

# confirmed anchors (rendered on-screen text / proven this investigation)
VERIFIED = {
    0x29C0E211: "NO", 0x2AD0F011: "YES", 0x68C0F413: "QUIT",
    0x69C04050: "CONTINUE", 0x69C8F473: "QUIT GAME", 0x0AD0F813: "PAUSED",
}
def floor(idv): return bin((idv ^ SEED) & 0xFFFFFFFF).count("1")


def load_pool():
    """id -> {floor, type, levels} from categories (fallback to master)."""
    info = {}
    if CATS.exists():
        with CATS.open(newline="") as f:
            for r in csv.DictReader(f):
                i = int(r["id_hex"], 16)
                info[i] = {"type": r.get("type", ""), "levels": r.get("levels", "")}
    with MASTER.open() as f:
        next(f)
        for line in f:
            c = line.split(",")
            try: i = int(c[1])
            except Exception: continue
            info.setdefault(i, {"type": c[2] if len(c) > 2 else "", "levels": ""})
    return info


def run_match(words: Path, ids: Path, overshoot, wordlike):
    """Return id -> list of candidate strings (unique)."""
    args = [str(KCRACK), "match", "--overshoot", str(overshoot)]
    if wordlike: args.append("--wordlike")
    args.append(str(ids))
    hits = collections.defaultdict(set)
    with words.open("rb") as w:
        p = subprocess.run(args, stdin=w, capture_output=True, text=True)
    for line in p.stdout.splitlines():
        c = line.split("\t")
        if len(c) >= 2:
            hits[int(c[0], 16)].add(c[1])
    return hits


def run_combine(pool: Path, ids: Path, depth, overshoot):
    args = [str(KCRACK), "combine", "--depth", str(depth), "--overshoot", str(overshoot),
            "--wordlike", str(pool), str(ids)]
    p = subprocess.run(args, capture_output=True, text=True)
    hits = collections.defaultdict(set)
    for line in p.stdout.splitlines():
        c = line.split("\t")
        if len(c) >= 2:
            hits[int(c[0], 16)].add(c[1])
    return hits


def run_extend(root: str, ids: Path, suf=4):
    """kcrack extend a confirmed name -> {id: {sibling strings}}."""
    args = [str(KCRACK), "extend", "--suf", str(suf), "--overshoot", "0",
            "--wordlike", root, str(ids)]
    p = subprocess.run(args, capture_output=True, text=True)
    hits = collections.defaultdict(set)
    for line in p.stdout.splitlines():
        c = line.split("\t")
        if len(c) >= 2:
            hits[int(c[0], 16)].add(c[1])
    return hits


def ingest_tsv(path: Path):
    """Fold a streamed kcrack hits file (id\\tstring\\t...) -> {id: {strings}}."""
    hits = collections.defaultdict(set)
    with path.open() as f:
        for line in f:
            c = line.rstrip("\n").split("\t")
            if len(c) >= 2 and c[0].startswith("0x"):
                hits[int(c[0], 16)].add(c[1])
    return hits


def readability(cand, words):
    """Higher = more name-like: real dictionary tokens, few tokens, little filler."""
    toks = [t for t in cand.replace("-", "_").split("_") if t]
    if not toks:
        return -99
    known = sum(1 for t in toks if t in words)
    covered = sum(len(t) for t in toks if t in words)
    total = sum(len(t) for t in toks)
    return known * 12 + covered * 2 - len(toks) * 3 - (total - covered) * 4


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--words", help="wordlist for match pass")
    ap.add_argument("--combine", help="word pool for combinator pass")
    ap.add_argument("--depth", type=int, default=2)
    ap.add_argument("--overshoot", type=int, default=0)
    ap.add_argument("--ingest", help="fold in a streamed kcrack hits file (resumable long runs)")
    ap.add_argument("--extend-from", action="store_true", help="kcrack-extend every verified/manual name -> siblings")
    ap.add_argument("--dict", default="/tmp/dict_all.txt", help="wordlist for readability scoring")
    ap.add_argument("--cap", type=int, default=15, help="max alternative candidates kept per id")
    args = ap.parse_args()
    if not KCRACK.exists():
        sys.exit("build kcrack first: gcc -O3 -pthread -o tools/kcrack tools/kcrack.c")

    words_for_score = set()
    if Path(args.dict).exists():
        words_for_score = {w.strip().upper() for w in Path(args.dict).read_text().splitlines() if w.strip()}

    info = load_pool()
    ids_file = Path(tempfile.mktemp())
    ids_file.write_text("\n".join(str(i) for i in info) + "\n")

    # load existing ledger first (so --extend-from can read confirmed names)
    prev = {}
    if LEDGER.exists():
        with LEDGER.open(newline="") as f:
            for r in csv.DictReader(f):
                prev[int(r["id_hex"], 16)] = r

    # gather candidates per id from the attack passes
    found = collections.defaultdict(set)
    if args.words:
        for idv, cands in run_match(Path(args.words), ids_file, args.overshoot, True).items():
            found[idv] |= {(c, "dict") for c in cands}
    if args.combine:
        for idv, cands in run_combine(Path(args.combine), ids_file, args.depth, args.overshoot).items():
            found[idv] |= {(c, "combo") for c in cands}
    if args.ingest and Path(args.ingest).exists():
        for idv, cands in ingest_tsv(Path(args.ingest)).items():
            found[idv] |= {(c, "combo") for c in cands}
    if args.extend_from:
        roots = {VERIFIED[i] for i in VERIFIED}
        roots |= {r["name"] for r in prev.values() if r.get("status") in ("verified", "manual") and r.get("name")}
        print(f"extend-from: {len(roots)} confirmed roots", file=sys.stderr)
        for root in sorted(roots):
            for idv, cands in run_extend(root, ids_file).items():
                found[idv] |= {(c, "extend") for c in cands}
    ids_file.unlink(missing_ok=True)

    rows = []
    counts = collections.Counter()
    for idv in sorted(info):
        old = prev.get(idv)
        # keep human-confirmed rows untouched
        if old and old.get("status") in ("verified", "manual"):
            rows.append(old); counts[old["status"]] += 1; continue
        if idv in VERIFIED:
            status, name, method, alts = "verified", VERIFIED[idv], "anchor", ""
        else:
            cands = set(found.get(idv, set()))
            # accumulate prior auto findings so re-runs never lose progress
            if old and old.get("name"):
                cands.add((old["name"], old.get("method", "prev")))
                for a in (old.get("alts") or "").split("|"):
                    if a: cands.add((a, old.get("method", "prev")))
            meth = {}
            for c, m in cands:
                meth[c] = "dict" if (meth.get(c) == "dict" or m in ("dict", "prev")) else m
            dict_names = sorted([c for c in meth if meth[c] == "dict"])
            other = [c for c in meth if meth[c] != "dict"]
            # order non-dict candidates by how name-like they read
            other.sort(key=lambda c: -readability(c, words_for_score))
            if not meth:
                status, name, method = "uncracked", "", ""
            elif dict_names:                       # clean dictionary hit(s) -> trustworthy
                status = "likely" if len(dict_names) == 1 else "ambiguous"
                name, method = dict_names[0], "dict"
            else:                                  # extend/combinator hits = collision-prone
                status = "candidate"
                name, method = other[0], meth[other[0]]
            ordered = (dict_names + other) if dict_names else other
            alts = "|".join(n for n in ordered if n != name)[:args.cap * 16].rstrip("|")
            alts = "|".join((alts.split("|"))[:args.cap])
        counts[status] += 1
        rows.append({
            "id_hex": f"0x{idv:08x}", "id_dec": str(idv), "floor": str(floor(idv)),
            "type": info[idv].get("type", ""), "levels": info[idv].get("levels", ""),
            "status": status, "name": name, "method": method, "alts": alts,
        })

    # atomic write
    fields = ["id_hex", "id_dec", "floor", "type", "levels", "status", "name", "method", "alts"]
    tmp = LEDGER.with_suffix(".tmp")
    with tmp.open("w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=fields); w.writeheader()
        for r in rows:
            w.writerow({k: r.get(k, "") for k in fields})
    os.replace(tmp, LEDGER)

    print(f"ledger: {LEDGER.relative_to(ROOT)}  ({len(rows)} ids)")
    for s in ("verified", "manual", "likely", "ambiguous", "candidate", "uncracked"):
        if counts[s]: print(f"  {s:9s}: {counts[s]}")
    print("\nnamed (verified/likely):")
    for r in rows:
        if isinstance(r, dict) and r.get("status") in ("verified", "manual", "likely") and r.get("name"):
            print(f"  {r['id_hex']}  {r['status']:8s} floor={r['floor']:>2}  {r['name']}")


if __name__ == "__main__":
    main()
