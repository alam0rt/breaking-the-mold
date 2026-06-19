#!/usr/bin/env python3
"""Skullmonkeys ToolX asset-id decoder / encoder.

The asset id namespacing is:   id = SEED ^ rotl(calcHash(name), R)
where calcHash is the Neverhood per-character single-bit-toggle hash.

Known (SEED, R) "prefix keys":
  RAW    (0x00000000,  0)  audio FX_* names (FX_ is literal text in the name)
  TEXT   (0x28C0E011, 27)  on-screen text glyphs
  PICKUP (0x88200080, 27)  power-up pickups
  BOSS   (0x08280150, 27)  boss/enemy sprites  (discovered 2026-06-17)

Usage:
  decoder.py hash  NAMESPACE NAME      -> id for NAME in NAMESPACE
  decoder.py probe NAMESPACE 0xID ENT  -> action word for an entity sprite id
"""
import sys
MASK=0xFFFFFFFF
KEYS={"RAW":(0x00000000,0),"TEXT":(0x28C0E011,27),"PICKUP":(0x88200080,27),"BOSS":(0x08280150,27)}
def rotl(x,r): r%=32; return ((x<<r)|(x>>(32-r)))&MASK
def rotr(x,r): return rotl(x,(32-(r%32))%32)
def cval(c):
    o=ord(c)
    if 65<=o<=90: return o-64
    if 97<=o<=122: return o-96
    if 48<=o<=57: return o-42
    return None
def calc(s):
    h=0;sv=0
    for c in s:
        v=cval(c)
        if v is None: continue
        sv=(sv+v)&31; h^=1<<sv
    return h
def make_id(ns,name):
    seed,R=KEYS[ns]; return seed^rotl(calc(name),R)
def decode_target(ns,idv):
    seed,R=KEYS[ns]; return rotr(idv^seed,R)   # == calc(full assetname)
if __name__=="__main__":
    cmd=sys.argv[1] if len(sys.argv)>1 else ""
    if cmd=="hash":
        ns,name=sys.argv[2],sys.argv[3]
        print("0x%08X"%make_id(ns,name))
    elif cmd=="probe":
        ns,idv,ent=sys.argv[2],int(sys.argv[3],16),sys.argv[4]
        target=decode_target(ns,idv)
        acts=["WALK","RUN","JUMP","IDLE","STAND","DIE","HURT","HIT","ATTACK","CAST","SHOOT","FIRE",
        "THROW","SPIT","FLY","FLAP","TURN","ROLL","FALL","LAND","SPIN","BOUNCE","SWING","STOMP","DASH",
        "DUCK","EAT","WING","STAFF","WIN","SHELL","SCREAM","TAUNT","BLINK","CHARGE","BEAM"]
        frames=[""]+[str(n) for n in range(13)]+["%02d"%n for n in range(13)]+["_%d"%n for n in range(10)]+["_%02d"%n for n in range(10)]
        hits=[ent+a+f for a in acts for f in frames if calc(ent+a+f)==target]
        print("target calc=0x%08X  matches: %s"%(target,hits or "(none — try other actions/frames)"))
    else:
        print(__doc__)
