#!/usr/bin/env python3
"""Map gameplay sprite families using the FX (sound) action vocabulary.

Discovery (2026-06-17): a character's sprite animation family is `STEM + <ACTION>`,
where the ACTION suffixes are the SAME words as its FX sound names. So:
  1. group FX names into (entity, action)
  2. for each entity, find a transform (seed,rot) that lands >=2 of its action-cores
     on real sprite IDs  (sprite family <-> sound family share action-suffix spacing)
  3. solve the sprite STEM from action-only suffixes, then predict the rest of the family
  4. validate via code identity (the sprite IDs' referencing functions / level)

Output: docs/analysis/asset-identification/fx_sprite_families.csv (+ .md)
"""
from __future__ import annotations
import csv, sys
from collections import defaultdict, Counter
from pathlib import Path
sys.path.insert(0, "tools/scripts")
from calchash import calcHash

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

# known entity prefixes (longest match wins) — from verified FX names
ENTITIES = ["BOSS_HEAD", "KLAY", "SKULL", "FINN", "RAT", "BIRD", "GUM", "PUFF", "PICKUP", "MENU"]
# extra action words to try when predicting (superset of observed FX actions)
EXTRA_ACT = ["HIT","TURN","WALK","IDLE","DIE","DEATH","HURT","JUMP","LAND","FALL","STAND","RUN","FIRE",
 "SHOOT","ATTACK","BITE","EAT","SCREAM","ROAR","SPIT","CHARGE","STOMP","SPAWN","INTRO","FLY","FLAP",
 "GRUNT","SPRING","DUCK","BOUNCE","SLAM","SWAY","DASH","FART","BLINK","REST","GLIDE","SWIM","THROW",
 "GRAB","TAUNT","VICTORY","REACT","POOF","SLEEP","WAKE","CLIMB","DIVE","PECK","CRY"]
SUFNUM = ["","_1","_2","_3","_4","_01","_02","_03","_04","1","2","3","_DN","_UP","_LEFT","_RIGHT",
 "_DOWN","_SOFT","_FALL","_FAST","_HEAD","_END","_QUIET","_QUIET_SOFT"]

def load():
    rows = list(csv.DictReader((AID / "cracked_names.csv").open()))
    fx = [r["name"] for r in rows if r["name"].startswith("FX_")]
    spr = {int(r["id_hex"], 16): r["levels"] for r in rows
           if r["id_hex"].startswith("0x") and r["status"] == "uncracked" and r["type"] in ("sprite", "anim")}
    # id -> referencing functions (for validation)
    idfn = defaultdict(list)
    cat = AID / "asset_access_catalog.csv"
    if cat.exists():
        for r in csv.DictReader(cat.open()):
            idfn[int(r["id_hex"], 16)] = r["top_functions"]
    return fx, spr, idfn

def split_entity(core):  # core = name without FX_
    for e in sorted(ENTITIES, key=len, reverse=True):
        if core == e: return e, ""
        if core.startswith(e + "_"): return e, core[len(e)+1:]
    return None, None

def main():
    fx, spr, idfn = load()
    sprset = set(spr)
    # group FX -> entity -> set(actions)
    ent_actions = defaultdict(set)
    for name in fx:
        e, a = split_entity(name[3:])
        if e and a: ent_actions[e].add(a)
    print(f"FX entities w/ actions: { {e:len(a) for e,a in ent_actions.items()} }")

    out_rows = []
    md = ["# FX-anchored sprite families (STEM + action)\n",
          "\nEach entity's sprite family solved from its FX action vocabulary. `stem` is the opaque "
          "sprite entity-hash; members are `stem + action` landing on real sprite IDs. Validate via "
          "the referencing functions.\n"]

    for ent, actions in sorted(ent_actions.items(), key=lambda x: -len(x[1])):
        actions = sorted(actions)
        if len(actions) < 2: continue
        # transform sweep: cores = full FX core "ENT_ACTION"; find (rot,seed) hitting >=2 actions
        cores = [(a, *chs(ent + "_" + a)) for a in actions]
        buckets = defaultdict(dict)   # (rot,seed) -> {action: sprite_id}
        for rot in range(32):
            for a, hc, _ in cores:
                rc = rotl(hc, rot)
                for sid in sprset:
                    buckets[(rot, sid ^ rc)][a] = sid
        # best bucket: most distinct actions hitting distinct ids
        best = None
        for (rot, seed), amap in buckets.items():
            if len(amap) >= 2 and len(set(amap.values())) >= 2:
                if best is None or len(amap) > len(best[2]):
                    best = (rot, seed, dict(amap))
        if not best: continue
        rot, seed, amap = best
        # solve sprite STEM from action-only suffixes (the matched (action -> id) pairs)
        anchors = list(amap.items())
        stem_hash = None; sh_stem = None
        for sh in range(32):
            stems = {sid ^ rotl(chs(a)[0], sh) for a, sid in anchors}
            if len(stems) == 1:
                stem_hash = next(iter(stems)); sh_stem = sh; break
        if stem_hash is None: continue
        # predict whole family: stem + (every action) + numeric variants
        fam = {}
        for a in set(actions) | set(EXTRA_ACT):
            for n in SUFNUM:
                suf = a + n
                pid = stem_hash ^ rotl(chs(suf)[0], sh_stem)
                if pid in sprset:
                    fam[pid] = suf
        # validation signature
        fns = Counter()
        for pid in fam:
            for f in (idfn.get(pid, "") or "").split(" | "):
                if f: fns[f.split("_")[0][:18]] += 1
        sig = ";".join(f"{k}({v})" for k, v in fns.most_common(3))
        print(f"\n[{ent}] stem=0x{stem_hash:08x} sh={sh_stem}  {len(fam)} sprites  validate:{sig}")
        for pid, suf in sorted(fam.items(), key=lambda x: x[1]):
            print(f"    {ent}<stem>+{suf:12} -> 0x{pid:08x}")
            out_rows.append({"entity": ent, "stem_hash": f"0x{stem_hash:08x}", "sh": sh_stem,
                             "action": suf, "sprite_id": f"0x{pid:08x}", "ref_funcs": (idfn.get(pid,"") or "")[:50]})
        md.append(f"\n## {ent}  (stem `0x{stem_hash:08x}`, sh {sh_stem}) — {len(fam)} sprites; validate: {sig}\n\n")
        md.append("| action | sprite_id | ref functions |\n|---|---|---|\n")
        for pid, suf in sorted(fam.items(), key=lambda x: x[1]):
            md.append(f"| {suf} | 0x{pid:08x} | {(idfn.get(pid,'') or '')[:50]} |\n")

    out = AID / "fx_sprite_families.csv"
    if out_rows:
        with out.open("w", newline="") as f:
            w = csv.DictWriter(f, fieldnames=list(out_rows[0].keys())); w.writeheader(); w.writerows(out_rows)
    (AID / "fx_sprite_families.md").write_text("".join(md))
    print(f"\n{len(out_rows)} family-sprite rows -> {out.relative_to(ROOT)} (+ .md)")

if __name__ == "__main__":
    main()
