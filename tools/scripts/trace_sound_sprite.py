#!/usr/bin/env python3
"""Correlate verified FX sounds with sprites by CODE CO-OCCURRENCE (behavior, not names).

If a function plays a verified FX sound (e.g. FX_BOSS_HEAD_HIT) and also sets a sprite
(SetEntitySpriteId / InitEntitySprite), that sprite is that sound's animation. This labels
sprites using the VERIFIED sound as the anchor, independent of the (guessed) function names.
"""
from __future__ import annotations
import csv, re, sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
AID = ROOT / "docs/analysis/asset-identification"
SRC = ROOT / "export/SLES_010.90.c"

FUNC_DEF = re.compile(r"^[A-Za-z_][\w \*]*\b([A-Za-z_]\w+)\s*\(")
CTRL = {"if","for","while","switch","return","do","else","case"}
HEX = re.compile(r"0x[0-9a-fA-F]{6,8}")
PLAY = re.compile(r"PlaySoundEffect\s*\(\s*(0x[0-9a-fA-F]+)")
SETSPR = re.compile(r"(?:SetEntitySpriteId|InitEntitySprite|AllocSpriteRenderContext)\s*\([^;]*?(0x[0-9a-fA-F]{6,8})")

def functions():
    pending=None; cur="<global>"; buf=[]; start=0
    with SRC.open(errors="replace") as f:
        lines=f.readlines()
    # collect (funcname, body-lines)
    out=[]; cur="<global>"; body=[]
    pending=None
    for ln in lines:
        st=ln.strip()
        if st=="{" and pending:
            if body: out.append((cur, body))
            cur=pending; pending=None; body=[]
        else:
            m=FUNC_DEF.match(ln)
            if m and m.group(1) not in CTRL and not ln.rstrip().endswith(";") and "=" not in ln.split("(")[0]:
                pending=m.group(1)
        body.append(ln)
    out.append((cur, body))
    return out

def main():
    rows=list(csv.DictReader((AID/"cracked_names.csv").open()))
    fxname={int(r["id_hex"],16):r["name"] for r in rows if r["name"].startswith("FX_")}
    sprset={int(r["id_hex"],16) for r in rows if r["id_hex"].startswith("0x") and r["type"] in("sprite","anim")}
    sprstatus={int(r["id_hex"],16):r["status"] for r in rows if r["id_hex"].startswith("0x")}

    # per function: sounds played, sprites set
    corr=[]   # (func, fxid, fxname, [sprite ids set])
    for fn, body in functions():
        text="".join(body)
        # any verified sound id present as a literal (played via any helper fn)
        vsounds=[h for h in (int(s,16) for s in HEX.findall(text)) if h in fxname]
        vsounds=list(dict.fromkeys(vsounds))
        if not vsounds: continue
        sprites=[int(s,16) for s in SETSPR.findall(text) if int(s,16) in sprset]
        sprites=list(dict.fromkeys(sprites))
        if sprites:
            for v in vsounds:
                corr.append((fn, v, fxname[v], sprites))

    # write catalog CSV + md
    import csv as _csv
    out=AID/"sound_sprite_map.csv"
    with out.open("w",newline="") as f:
        w=_csv.writer(f); w.writerow(["sound_name","sound_id","function","sprite_id","sprite_status"])
        for fn, fxid, fxnm, sprites in corr:
            for s in sprites:
                w.writerow([fxnm,f"0x{fxid:08x}",fn,f"0x{s:08x}",sprstatus.get(s,"?")])
    print(f"wrote {out.name}\n")
    print("=== functions playing a verified FX sound + sprites they set ===\n")
    # group by entity (FX prefix) for readability
    by_ent=defaultdict(list)
    for fn, fxid, fxnm, sprites in corr:
        ent="_".join(fxnm.split("_")[1:3]) if fxnm.startswith("FX_") else fxnm
        by_ent[ent].append((fxnm, fn, sprites))
    for ent in sorted(by_ent):
        print(f"## {ent}")
        for fxnm, fn, sprites in by_ent[ent]:
            ss=" ".join(f"0x{s:08x}{'' if sprstatus.get(s)=='uncracked' else '(cracked)'}" for s in sprites[:8])
            print(f"  [{fxnm}]  in {fn}()  sets: {ss}")
        print()

if __name__ == "__main__":
    main()
