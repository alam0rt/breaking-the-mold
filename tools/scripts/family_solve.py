#!/usr/bin/env python3
"""Joint family solver: crack a shared name ROOT from sibling ids + guessed suffixes.

If siblings are ROOT+suffix_i, then calcHash(name_i)=calcHash(ROOT)^rotl(calcHash(suffix_i),S)
where S=stepsum(ROOT). Given the sibling ids and a small SUFFIX vocabulary, brute S and the
suffix assignment; a consistent solution across all siblings pins ROOT's hash+length. With
>=2 siblings the answer is collision-proof. Recover ROOT by dictionary/short-brute on its hash.
"""
import sys
SEED=0x28C0E011
def rotl(v,r): r&=31; return ((v<<r)|(v>>((32-r)&31)))&0xFFFFFFFF
def rotr(v,r): return rotl(v,(32-(r&31))&31)
def calc2(s):
    h=0;sh=0
    for ch in s:
        o=ord(ch)
        if 'a'<=ch<='z':o-=32
        elif '0'<=ch<='9':o+=22
        if not(('A'<=ch<='Z')or('0'<=ch<='9')):continue
        sh=(sh+(o-64))&31;h^=1<<sh
    return h,sh
def pc(v): return bin(v).count('1')

def solve(ids, suffix_vocab, root_dict=None):
    H=[rotr(i^SEED,27) for i in ids]          # calcHash of each full name
    vh={w:calc2(w)[0] for w in suffix_vocab}   # suffix calcHash (from pos 0)
    sols=[]
    for S in range(32):
        for s0,h0 in vh.items():
            root_h=H[0]^rotl(h0,S)
            assign=[s0]; ok=True
            for i in range(1,len(H)):
                req=rotr(H[i]^root_h,S)
                m=[w for w,hh in vh.items() if hh==req]
                if not m: ok=False; break
                assign.append(m[0])
            if ok:
                roots=[]
                if root_dict:
                    roots=[w for w in root_dict if calc2(w)==(root_h,S)]
                sols.append((S,root_h,assign,roots))
    return sols

if __name__=="__main__":
    # DEMO on a known family: QUIT (root QUIT, suffix "") and QUITGAME (root QUIT, suffix "GAME")
    print("=== DEMO: known family QUIT / QUITGAME ===")
    ids=[0x68c0f413, 0x69c8f473]
    vocab=["","GAME","RUN","JUMP","OVER","MENU","NEW","LOAD","SAVE","START","BACK"]
    rootdict=["QUIT","EXIT","STOP","MENU","KLAY","KLAYMEN","PLAYER"]
    for S,rh,asg,roots in solve(ids,vocab,rootdict):
        print(f"  stepsum(root)={S:2d} root_hash=0x{rh:08x} suffixes={asg} root_recovered={roots}")
