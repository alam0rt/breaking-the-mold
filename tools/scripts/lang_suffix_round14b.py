#!/usr/bin/env python3
"""Faster: pivot on KNOWN priors. For each prior P, compute calcHash(P), then
for each rotation r in orbit, compute target = calcHash(P) ^ rotl(0x01079563, r),
and look up which strings hash to that target.

This way the search is len(priors) × 32 × O(1) lookups, then we collect partner strings.
"""

def calc_hash(s):
    h = 0; sh = 0
    for c in s.upper():
        if 'A' <= c <= 'Z':
            sh = (sh + ord(c) - 64) & 31
            h ^= 1 << sh
        elif '0' <= c <= '9':
            sh = (sh + ord(c) + 22 - 64) & 31
            h ^= 1 << sh
    return h

def rotl(x, n):
    return ((x << n) | (x >> (32 - n))) & 0xFFFFFFFF

FR_DELTA_ID = 0x01079563
DE_DELTA_ID = 0x0587801a

FR_ORBIT = [rotl(FR_DELTA_ID, r) for r in range(32)]
DE_ORBIT = [rotl(DE_DELTA_ID, r) for r in range(32)]

# Build dictionary of all alpha and short alphanum strings
import itertools
ALPHA = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
DIGIT = "0123456789"

print("Building hash → strings map...")
h_to_strs = {}
counts = []
for L in range(1, 6):
    n = 0
    for combo in itertools.product(ALPHA, repeat=L):
        s = "".join(combo)
        h_to_strs.setdefault(calc_hash(s), []).append(s)
        n += 1
    counts.append(n)
    print(f"  alpha L={L}: {n:,}")
print(f"  total {sum(counts):,} alpha strings, {len(h_to_strs):,} unique hashes")

# Priors - language tags and translation candidates
PRIORS = [
    "", "US", "FR", "DE", "EN", "GB", "UK", "JP", "JA", "GE",
    "USA", "FRA", "DEU", "GER", "ENG", "FRE", "FRC", "FRH",
    "AME", "AMR", "ENGLISH", "FRENCH", "GERMAN",
    "UE", "EU", "INTL", "INT", "PAL", "NTSC",
    "FRA1", "DEU1", "ENG1", "USA1",
    "L1", "L2", "L3", "L4", "L0",
    "T1", "T2", "T3", "T0",
    "0", "1", "2", "3", "00", "01", "02", "03",
    "A", "B", "C", "D", "E", "F", "G",
    # Translation words (full UPPERCASE)
    "NO", "YES", "OUI", "JA", "NEIN", "NON", "OK",
    "QUIT", "QUITTER", "BEEND", "BEENDEN", "EXIT", "AUS",
    "RETURN", "BACK", "RETOUR", "ZURUCK",
    "PAUSE", "PAUSED", "PLAY", "JOUER", "SPIELEN",
    "GAME", "JEU", "SPIEL", "OVER", "FIN", "ENDE",
    "CONTINUE", "RESUME", "RESUMER", "FORTSETZEN",
    "OPTIONS", "OPTION", "MENU", "MENUE",
    "TITLE", "TITRE", "TITEL",
    "SCORE", "BEST", "TIME", "TEMPS", "ZEIT",
    "LIVES", "VIE", "VIES", "LEBEN",
    "LEVEL", "NIVEAU", "EBENE", "STUFE",
    "PRESS", "DRUECK", "APPUYE", "START", "DEPART",
    "LOAD", "SAVE", "CHARGER", "SAUVER", "LADEN", "SPEICHERN",
    "GAMEOVER", "FINPARTIE", "ENDE",
    # Skullmonkeys-themed
    "MONKEY", "MONK", "MONKE", "AFFE", "SINGE",
    "TITLE", "BOSS", "ENEMY", "FOE", "ENNEMI", "FEIND",
    "SPRITE", "BANK", "SHEET", "ATLAS",
    "TEXT", "TXT",
    # 2-letter combos that might form internal codes
    "EU", "AS", "AM", "WO",
    # 1-char codes
    "U", "F", "D", "E", "G", "J", "I", "S",
]

PRIORS = list(set(PRIORS))
print(f"\n{len(PRIORS)} priors")

# Compute hash for each prior (treat empty as 0)
prior_hashes = {p: calc_hash(p) for p in PRIORS}

print("\n=== FR pairs (lookup via priors) ===")
fr_results = []
for prior in PRIORS:
    pa = prior_hashes[prior]
    for delta in FR_ORBIT:
        target = pa ^ delta
        if target in h_to_strs:
            for partner in h_to_strs[target]:
                if partner != prior:
                    fr_results.append((prior, partner, delta))

print(f"  {len(fr_results):,} pairs found via priors")
# Show interesting ones (partner is short or also a prior)
print("\n  Pairs where partner is ≤4 chars:")
shown = 0
for prior, partner, delta in fr_results:
    if len(partner) <= 4 and shown < 80:
        partner_in = "*" if partner in PRIORS else " "
        print(f"    {prior!r:14s} -> {partner_in}{partner!r:8s}  delta={delta:#010x}")
        shown += 1

print("\n=== DE pairs (lookup via priors) ===")
de_results = []
for prior in PRIORS:
    pa = prior_hashes[prior]
    for delta in DE_ORBIT:
        target = pa ^ delta
        if target in h_to_strs:
            for partner in h_to_strs[target]:
                if partner != prior:
                    de_results.append((prior, partner, delta))

print(f"  {len(de_results):,} pairs found via priors")
print("\n  Pairs where partner is ≤4 chars:")
shown = 0
for prior, partner, delta in de_results:
    if len(partner) <= 4 and shown < 80:
        partner_in = "*" if partner in PRIORS else " "
        print(f"    {prior!r:14s} -> {partner_in}{partner!r:8s}  delta={delta:#010x}")
        shown += 1

# Even better: find prior pairs where SAME pair gives both FR and DE deltas
# i.e., prior_X has corresponding FR partner Y and DE partner Z s.t. all 3 form a triple
print("\n=== EN/FR/DE TRIPLES (prior + partners give correct rotation alignment) ===")
# DE rotation = FR rotation + 15 (or whatever - per the doc, normalized)
# We need delta_DE rotation = delta_FR rotation + 15 (alignment)
# Effectively, for a given (prior_EN, base_rot), partner_FR uses rotl(FR_DELTA, r) and
# partner_DE uses rotl(DE_DELTA, r+15)... no wait, that's for the SAME asset's base rotation.
# For our pure-suffix case (no base), the rotations of prior_FR and prior_DE relative to each other
# should be consistent with the global mask alignment.

triples = []
for prior in PRIORS:
    pa = prior_hashes[prior]
    for r in range(32):
        fr_target = pa ^ rotl(FR_DELTA_ID, r)
        de_target = pa ^ rotl(DE_DELTA_ID, r)  # same r for canonical mask alignment
        if fr_target in h_to_strs and de_target in h_to_strs:
            for fr_p in h_to_strs[fr_target]:
                if fr_p == prior: continue
                for de_p in h_to_strs[de_target]:
                    if de_p == prior or de_p == fr_p: continue
                    triples.append((prior, fr_p, de_p, r))

print(f"  {len(triples):,} triples found")
print("\n  Triples where ALL three are short (≤5):")
shown = 0
for en, fr, de, r in triples:
    if len(en) <= 5 and len(fr) <= 5 and len(de) <= 5 and shown < 100:
        print(f"    EN={en!r:6s}  FR={fr!r:6s}  DE={de!r:6s}  rot={r}")
        shown += 1
