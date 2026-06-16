#!/usr/bin/env python3
"""Comprehensive single-target brute (both hash models) with compound hashing.

Usage: python3 tools/scripts/brute_one.py 0x8ab92024 [--theme loading]
Reports every candidate string that hashes (RAW or WRAP) to the target id.
"""
from __future__ import annotations
import argparse, itertools, sys, time

def chs(s):
    h = sh = 0
    for ch in s:
        c = ord(ch)
        if 65 <= c <= 90: pass
        elif 97 <= c <= 122: c -= 32
        elif 48 <= c <= 57: c += 22
        else: continue
        sh = (sh + c - 64) & 31
        h ^= 1 << sh
    return h, sh

def rotl(v, r):
    r &= 31
    return ((v << r) | (v >> ((32 - r) & 31))) & 0xFFFFFFFF

SEED = 0x28C0E011

# ---- vocab slots (loading-screen themed, generous) ----
LEAD = ["", "S_", "SP_", "SPR_", "SPRT_", "SEQ_", "ANM_", "A_", "UI_", "HUD_", "BG_", "FG_",
        "MENU_", "GAME_", "SCR_", "SCREEN_", "GFX_", "OBJ_", "SM_", "SYS_", "TXT_", "TEXT_",
        "LOAD_", "LOADING_", "LVL_", "STAGE_", "INTER_", "TRANS_"]

CORE = ["LOADING", "LOAD", "LOADER", "LOADSCREEN", "LOADINGSCREEN", "NOWLOADING", "PLEASEWAIT",
        "WAIT", "WAITING", "STANDBY", "ONEMOMENT", "HOLDON", "LOADIN", "LOADN",
        "SCREEN", "STAGE", "LEVEL", "LVL", "STAGELOAD", "LEVELLOAD", "STAGESCREEN", "LEVELSCREEN",
        "INTERLEVEL", "INTERSTAGE", "INTERMISSION", "INTRO", "TRANSITION", "TRANS", "ENTER",
        "ENTERING", "NEXT", "NEXTLEVEL", "NEXTSTAGE", "GOTO", "WARP",
        "KLAYMEN", "KLAY", "WILLIE", "PLAYER", "HERO", "MONKEY", "CLAYMAN", "SKULLMONKEY",
        "BUSY", "WORKING", "PROCESSING", "INIT", "STARTUP", "BOOT", "PRELOAD", "STREAM",
        "DISK", "DISC", "CD", "DATA", "FILE", "READING", "READ", "BLB", "ASSET", "PAGE",
        "TITLE", "LOGO", "SPLASH", "PRESENT", "WAITSCREEN", "LOADANIM", "LOADBAR", "PROGRESS"]

MID = ["", "_", "SCREEN", "_SCREEN", "TEXT", "_TEXT", "ANIM", "_ANIM", "BG", "_BG", "KLAY",
       "_KLAY", "KLAYMEN", "_KLAYMEN", "WAIT", "_WAIT", "LOAD", "_LOAD", "LOADING", "_LOADING",
       "STAGE", "_STAGE", "LEVEL", "_LEVEL", "MONKEY", "_MONKEY", "RUN", "_RUN", "WALK", "_WALK",
       "WHEEL", "_WHEEL", "SPIN", "_SPIN", "DOTS", "_DOTS", "BAR", "_BAR", "ICON", "_ICON"]

TAIL = ["", "0", "1", "2", "00", "01", "02", "_0", "_1", "_2", "_00", "_01", "_02",
        "A", "B", "_A", "_B", "SCREEN", "_SCREEN", "TEXT", "_TEXT", "ANIM", "_ANIM", "SEQ",
        "_SEQ", "SPR", "_SPR", "SPRITE", "_SPRITE", "BG", "_BG", "IMG", "_IMG", "PAGE",
        ".SPR", ".SEQ", ".TIM", ".CEL", ".ANM", "S", "ER", "ING", "SCR"]


def precompute(words):
    return [(w, *chs(w)) for w in words]

def combine(a, b):
    ha, sha = a; hb, shb = b
    return (ha ^ rotl(hb, sha)) & 0xFFFFFFFF, (sha + shb) & 31


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("target")
    args = ap.parse_args()
    T = int(args.target, 16)
    Twrap = T  # for wrap we compare SEED^rotl(h,27)==T  ->  precompute target-side
    print(f"target = 0x{T:08x}  popcount={bin(T).count('1')}")

    lead = precompute(LEAD); core = precompute(CORE); mid = precompute(MID); tail = precompute(TAIL)
    # heads = LEAD x CORE
    heads = []
    for ln, lh, lsh in lead:
        for cn, ch_, csh in core:
            h, sh = combine((lh, lsh), (ch_, csh))
            heads.append((ln + cn, h, sh))
    # tails = MID x TAIL
    tails = []
    for mn, mh, msh in mid:
        for tn, th, tsh in tail:
            h, sh = combine((mh, msh), (th, tsh))
            tails.append((mn + tn, h, sh))
    total = len(heads) * len(tails)
    print(f"heads={len(heads)} tails={len(tails)} -> {total:,} names x2 models")

    hits = []
    t0 = time.time()
    rl = rotl
    for i, (hn, hh, hsh) in enumerate(heads):
        for tn, th, _ in tails:
            full = (hh ^ rl(th, hsh)) & 0xFFFFFFFF
            if full == T:
                hits.append(("RAW", hn + tn))
            elif (SEED ^ rl(full, 27)) & 0xFFFFFFFF == T:
                hits.append(("WRAP", hn + tn))
        if (i + 1) % 1000 == 0:
            print(f"  {i+1}/{len(heads)} heads  hits={len(hits)}  ({time.time()-t0:.0f}s)", flush=True)

    print(f"\n=== {len(hits)} hits in {time.time()-t0:.0f}s ===")
    for m, n in sorted(set(hits)):
        print(f"  {m}: {n}")


if __name__ == "__main__":
    main()
