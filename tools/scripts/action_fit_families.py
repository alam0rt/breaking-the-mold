#!/usr/bin/env python3
"""Expand the sprite-naming foothold: group uncracked sprites by the entity that
references them in code, then find `STEM + <known-action>` families WITHOUT needing
an FX sound for that entity. The action vocabulary (recovered from FX + boss-head +
player families) is the foothold.

For a code-entity group, bucket (sh, id ^ rotl(h(action), sh)) over all (sprite, action);
a bucket holding >=2 sprites via *different* actions is a family sharing one stem.
"""
from __future__ import annotations
import csv, sys, re
from collections import defaultdict
from pathlib import Path
sys.path.insert(0, "tools/scripts")

ROOT = Path(__file__).resolve().parents[2]
AID = ROOT / "docs/analysis/asset-identification"

def chs(s):
    h = sh = 0
    for ch in s:
        c = ord(ch)
        if 65 <= c <= 90: pass
        elif 97 <= c <= 122: c -= 32
        elif 48 <= c <= 57: c += 22
        else: continue
        sh = (sh + c - 64) & 31; h ^= 1 << sh
    return h, sh
def rotl(v, r): r &= 31; return ((v << r) | (v >> ((32 - r) & 31))) & 0xFFFFFFFF

# recovered action vocabulary (FX + boss-head + player) + common animation states
BASE_ACT = ["HIT","TURN","WALK","IDLE","DIE","DEATH","HURT","JUMP","LAND","FALL","STAND","RUN","FIRE",
 "SHOOT","ATTACK","BITE","EAT","SCREAM","ROAR","SPIT","CHARGE","STOMP","SPAWN","INTRO","FLY","FLAP",
 "GRUNT","SPRING","DUCK","BOUNCE","SLAM","SWAY","DASH","FART","BLINK","REST","GLIDE","SWIM","THROW",
 "GRAB","TAUNT","WIN","REACT","POOF","SLEEP","WAKE","CLIMB","DIVE","PECK","CRY","DIE_FALL","DUCK_DOWN",
 "RUN_FAST","HIT_HEAD","WAKE_UP","SQUISH","PIERCE","WALKING","RUNNING","JUMPING","FALLING","STANDING",
 "OPEN","CLOSE","START","END","MOVE","ACTIVE","HIDE","APPEAR","DROP","ROLL","SLIDE"]
NUMS = ["","_1","_2","_3","_4","_01","_02","_03","_04","1","2","3","4","_LEFT","_RIGHT","_UP","_DN",
        "_DOWN","_SOFT","_FALL","_FAST","_END","_HEAD"]
ACTIONS = []
for a in BASE_ACT:
    for n in NUMS:
        ACTIONS.append(a + n)
ACTH = [(a, chs(a)[0]) for a in ACTIONS]

def main():
    rows = list(csv.DictReader((AID / "cracked_names.csv").open()))
    spr = {int(r["id_hex"], 16) for r in rows
           if r["id_hex"].startswith("0x") and r["status"] == "uncracked" and r["type"] in ("sprite", "anim")}
    lv = {int(r["id_hex"], 16): r["levels"][:20] for r in rows if r["id_hex"].startswith("0x")}

    # group uncracked sprites by code-entity (prefix of their first referencing function)
    groups = defaultdict(set)
    cat = AID / "asset_access_catalog.csv"
    for r in csv.DictReader(cat.open()):
        idv = int(r["id_hex"], 16)
        if idv not in spr: continue
        fn = (r["top_functions"] or "").split(" | ")[0]
        if not fn: continue
        # entity key = function name up to a state/verb boundary
        m = re.match(r"([A-Za-z]+?)(?:State|Init|Tick|Callback|Event|Set|Check|Enter|Return|Update|Move|Spawn|Death|Damage|Handler|_)", fn)
        key = m.group(1) if m else fn
        if len(key) >= 4:
            groups[key].add(idv)

    results = []
    for ent, ids in sorted(groups.items(), key=lambda x: -len(x[1])):
        if len(ids) < 2: continue
        ids = sorted(ids)
        # bucket (sh, stem) -> {action: id}
        buckets = defaultdict(dict)
        for sh in range(32):
            for idv in ids:
                for a, ha in ACTH:
                    stem = idv ^ rotl(ha, sh)
                    b = buckets[(sh, stem)]
                    # keep first action per id to avoid one id filling a bucket twice
                    if idv not in b.values():
                        b.setdefault(a, idv)
        # best family: a (sh,stem) explaining the most DISTINCT sprite ids
        best = None
        for (sh, stem), amap in buckets.items():
            distinct_ids = set(amap.values())
            if len(distinct_ids) >= 2:
                if best is None or len(distinct_ids) > len(set(best[2].values())):
                    best = (sh, stem, dict(amap))
        if not best: continue
        sh, stem, amap = best
        # expand: predict all actions on this stem
        fam = {}
        for a, ha in ACTH:
            pid = stem ^ rotl(ha, sh)
            if pid in spr: fam[pid] = a
        if len(fam) >= 2:
            results.append((ent, stem, sh, fam, len(ids)))

    results.sort(key=lambda x: -len(x[3]))
    print(f"{'entity':22} {'stem':>10} sh  n  group  members")
    for ent, stem, sh, fam, gsize in results:
        if len(fam) < 2: continue
        members = " ".join(f"{a}=0x{pid:08x}" for pid, a in sorted(fam.items(), key=lambda x: x[1]))
        print(f"{ent:22} 0x{stem:08x} {sh:2} {len(fam):2}/{gsize:<3}  {members[:90]}")

    print(f"\n{len([r for r in results if len(r[3])>=3])} families with >=3 members (stronger).")

if __name__ == "__main__":
    main()
