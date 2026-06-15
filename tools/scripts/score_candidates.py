#!/usr/bin/env python3
"""Corroboration scorer for asset-name candidates.

A blind hash hit is unverifiable on its own, but real names form a CONSISTENT
SYSTEM: siblings share a root (PHART_PICKUP / PHART_SPAWN), and related assets
are used together in the code. Collisions are random and share nothing. So we
score each candidate by how much its tokens are corroborated by the candidates
of *related* assets:

  related(X) = assets that are
    - in the same BLB animation family as X            (asset_categories func_family)
    - X's nearest lexical neighbour in hash-space      (asset_categories nn_id)
    - referenced by the same function in the binary    (built here from lui/ori)

  score(cand of X) = sum over tokens t in cand of
      IDF(t)                                  # rare tokens count, 'SHE'/'THE' don't
      * len(t)
      * (1 + BONUS * #related assets whose candidates also contain t)

Tokens shared across UNRELATED ids (common filler) get high document-frequency
-> low IDF -> ignored. A rare token shared between two RELATED ids is the signal.

  python3 tools/scripts/score_candidates.py
Reads docs/analysis/asset-identification/{cracked_names,asset_categories}.csv +
the binary, writes corrob_score back to the ledger and prints the top finds.
"""
from __future__ import annotations
import csv, struct, collections, math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SEED = 0x28C0E011
LEDGER = ROOT / "docs/analysis/asset-identification/cracked_names.csv"
CATS = ROOT / "docs/analysis/asset-identification/asset_categories.csv"
CROSS = ROOT / "docs/reference/asset-ids-cross-build.csv"
BIN = ROOT / "bin/SLES_010.90"
BONUS = 4.0


def build_presence_groups(max_group=40):
    """ids that share a *rare* cross-build presence vector are related (added,
    removed, or localized together). The ubiquitous all-builds group is too
    broad to mean anything, so cap group size."""
    if not CROSS.exists():
        return {}, {}
    cols = ["beta", "demo_eu", "demo_jp", "pal_d1", "pal_d2", "pal_d3", "jp", "us"]
    groups = collections.defaultdict(set)
    nbuilds = {}
    with CROSS.open(newline="") as f:
        for r in csv.DictReader(f):
            i = int(r["id_hex"], 16)
            groups[tuple(r[c] for c in cols)].add(i)
            nbuilds[i] = int(r.get("n_builds", 0))
    related = collections.defaultdict(set)
    for vec, members in groups.items():
        if 1 < len(members) <= max_group:          # rare vector = real co-grouping
            for a in members:
                related[a] |= (members - {a})
    return related, nbuilds

# ---- binary co-reference: ids referenced by the same function ----
def binary_corref(pool):
    data = BIN.read_bytes()
    words = [struct.unpack_from('<I', data, i*4)[0] for i in range(len(data)//4)]
    funcs = []           # list of sets of asset ids per function
    cur = set(); last = {}
    i = 0
    while i < len(words):
        w = words[i]; op = w >> 26; rt = (w>>16)&0x1f; rs = (w>>21)&0x1f; imm = w & 0xffff
        if op == 0x0f: last[rt] = imm
        elif op == 0x0d and rs in last:
            v = (last[rs]<<16)|imm
            if v in pool: cur.add(v)
        elif op == 0x09 and rs in last:
            lo = imm-0x10000 if imm & 0x8000 else imm; v = ((last[rs]<<16)+lo)&0xffffffff
            if v in pool: cur.add(v)
        if w == 0x03e00008:            # jr $ra -> function end (after delay slot)
            i += 1
            if cur: funcs.append(cur)
            cur = set(); last = {}
        i += 1
    if cur: funcs.append(cur)
    co = collections.defaultdict(set)
    for fs in funcs:
        if 1 < len(fs) <= 40:          # ignore huge tables (not a real co-use signal)
            for a in fs:
                co[a] |= (fs - {a})
    return co


def main():
    cats = {}
    with CATS.open(newline="") as f:
        for r in csv.DictReader(f):
            cats[int(r["id_hex"], 16)] = r
    pool = set(cats)
    co = binary_corref(pool)

    # relatedness graph
    related = collections.defaultdict(set)
    fam_members = collections.defaultdict(set)
    for i, r in cats.items():
        if r.get("func_family", "") != "":
            fam_members[r["func_family"]].add(i)
    for members in fam_members.values():
        for a in members:
            related[a] |= (members - {a})
    for i, r in cats.items():
        try:
            if int(r.get("nn_dist", "99")) <= 6:
                related[i].add(int(r["nn_id"], 16))
                related[int(r["nn_id"], 16)].add(i)
        except Exception:
            pass
    for a, others in co.items():
        related[a] |= others
    bp_related, nbuilds = build_presence_groups()
    for a, others in bp_related.items():
        related[a] |= others

    # load ledger candidates: id -> list of candidate strings
    rows = list(csv.DictReader(LEDGER.open(newline="")))
    cand = {}
    for r in rows:
        i = int(r["id_hex"], 16)
        cs = [c for c in [r.get("name", "")] + (r.get("alts", "") or "").split("|") if c]
        cand[i] = cs

    # token document frequency (over ids) and IDF
    def toks(s): return [t for t in s.replace("-", "_").split("_") if len(t) >= 2]
    df = collections.Counter()
    for i, cs in cand.items():
        seen = set()
        for c in cs:
            for t in toks(c): seen.add(t)
        for t in seen: df[t] += 1
    N = max(1, len(cand))
    def idf(t): return math.log((N + 1) / (df.get(t, 0) + 1)) + 0.1

    # which ids have each token (for corroboration lookups)
    id_tokens = {i: set(t for c in cs for t in toks(c)) for i, cs in cand.items()}

    LEVELS = set("MENU PHRO SCIE TMPL FINN MEGA BOIL SNOW FOOD HEAD BRG1 GLID CAVE "
                 "WEED EGGS GLEN CLOU SEVN SOAR CRYS CSTL WIZZ RUNN MOSS KLOG EVIL".split())

    def score(i, c):
        s = 0.0; hits = []
        my_levels = set((cats.get(i, {}).get("levels", "") or "").split(";"))
        nlev = len(my_levels)
        for t in set(toks(c)):
            base = idf(t) * len(t)
            corr = sum(1 for j in related[i] if t in id_tokens.get(j, ()))
            if corr: hits.append((t, corr))
            mult = 1 + BONUS * corr
            # level-code consistency (the DIE_SNOW_AMASS_GUN lesson):
            #   a level token only counts if the asset truly lives there, and
            #   is strong only when the asset is (near-)exclusive to that level.
            if t in LEVELS:
                if t not in my_levels:        mult = 0.0          # asset isn't in that level -> coincidence
                elif nlev <= 3:               mult += 6.0         # exclusive -> strong
                elif nlev >= 10:              mult = 0.2          # ubiquitous -> meaningless
            s += base * mult
        return s, hits

    best = {}
    for i, cs in cand.items():
        if not cs: continue
        scored = sorted(((score(i, c), c) for c in cs), key=lambda x: -x[0][0])
        best[i] = (scored[0][1], scored[0][0][0], scored[0][0][1])  # cand, score, corrob hits

    # write corrob_score back
    by_id = {int(r["id_hex"], 16): r for r in rows}
    for i, (c, sc, hits) in best.items():
        by_id[i]["corrob_score"] = f"{sc:.1f}"
        by_id[i]["name"] = c if by_id[i]["status"] not in ("verified", "manual") else by_id[i]["name"]
    fields = list(rows[0].keys())
    if "corrob_score" not in fields: fields.append("corrob_score")
    tmp = LEDGER.with_suffix(".tmp")
    with tmp.open("w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=fields); w.writeheader()
        for r in rows: w.writerow({k: r.get(k, "") for k in fields})
    tmp.replace(LEDGER)

    print(f"related-graph: {sum(len(v) for v in related.values())//2} edges; "
          f"binary co-ref covers {len(co)} ids")
    print("\n=== top candidates by corroboration (rare root shared with a related asset) ===")
    ranked = sorted(((sc, i, c, hits) for i, (c, sc, hits) in best.items()
                     if by_id[i]["status"] not in ("verified", "manual") and hits),
                    key=lambda x: -x[0])
    for sc, i, c, hits in ranked[:25]:
        levs = (cats[i].get("levels", "") or "").split(";")[:3]
        sh = ", ".join(f"{t}x{n}" for t, n in sorted(hits, key=lambda x: -x[1])[:4])
        print(f"  0x{i:08x} score={sc:6.1f} fl={cats[i].get('type','')[:3]}:{by_id[i]['floor']:>2} "
              f"{c:24s} shared[{sh}] {levs}")
    if not ranked:
        print("  (no candidate shares a token with a related asset's candidate yet)")


if __name__ == "__main__":
    main()
