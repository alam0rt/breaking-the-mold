#!/usr/bin/env python3
"""Direct equality search: calcHash(S_FR) XOR calcHash(S_EN) == 0x01079563
(not a rotation - the rotation comes from the base name, the suffix delta itself
is the unrotated mask).

Same for DE: calcHash(S_DE) XOR calcHash(S_EN) == 0x00340b0f
"""

def calc_hash(s):
    h = 0; sh = 0
    for c in s.upper():
        if c.isalnum():
            co = ord(c) + 22 if c.isdigit() else ord(c)
            sh = (sh + co - 64) & 31
            h ^= 1 << sh
    return h

FR_DELTA = 0x01079563
DE_DELTA = 0x00340b0f

# Generate every alphanumeric string up to length 7 and bucket by hash
import itertools
ALPHA = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
DIGIT = "0123456789"

print("Building hash map for length 1-6 alphanumeric strings...")
h_to_strs = {}
total = 0
for L in range(1, 7):
    for combo in itertools.product(ALPHA + DIGIT, repeat=L):
        s = "".join(combo)
        h = calc_hash(s)
        h_to_strs.setdefault(h, []).append(s)
        total += 1
print(f"  {total:,} strings, {len(h_to_strs):,} unique hashes")

# Also include empty string (no suffix)
h_to_strs[0] = h_to_strs.get(0, []) + [""]

# Find pairs (a, b) where calcHash(a) ^ calcHash(b) == FR_DELTA
print(f"\n=== EN/FR pairs (calcHash(EN) ^ calcHash(FR) == {FR_DELTA:#010x}) ===")
print("Looking for plausible language tags only...")

LANG_HINTS = {
    # English-like
    "E", "EN", "ENG", "ENGL", "ENGLISH", "ENGUS", "USA", "US", "UK", "GB",
    "AME", "AMER", "AMERICAN", "BRT", "BRIT", "BRITISH", "EUR", "EURO",
    "EUROPE", "EUS", "EUK", "PAL", "NTSC", "INTL", "INTERNATIONAL",
    "ANGL", "ANGLAIS",
    # French-like
    "F", "FR", "FRA", "FRE", "FREN", "FRENCH", "FRANCAIS", "FRANCAISE",
    "FRANCE", "FRC", "FRH", "GAL", "GAUL",
    # German-like
    "D", "DE", "DEU", "DEUT", "DEUTS", "DEUTSCH", "DEUTSCHLAND",
    "GE", "GER", "GERM", "GERMAN", "GERMANY", "GERY", "ALLEMAND", "ALLEM", "ALEMAN",
    # Generic suffix variants
    "X", "T", "T2", "L", "L1", "L2", "X1", "X2",
    "0", "1", "2", "3", "0", "00", "01", "02", "03",
    "JP", "JA", "JPN", "JAP", "JAPAN", "JAPANESE",
    # Short word translations (matters as TEXT sprites are localized)
    "QUIT", "QUITTER", "BEEND", "BEENDEN",
    "NO", "OUI", "JA", "NEIN", "NON", "YES",
    "OK", "OKAY", "OKE",
    "GAME", "JEU", "SPIEL",
    "MENU",
    "PAUSE", "ANGEHALTEN",
    "LOAD", "CHARGER", "LADEN",
    "SAVE", "SAUVER", "SPEICHERN",
    "PLAY", "JOUER", "SPIELEN",
    "BACK", "RETOUR", "ZURUCK",
    "EXIT", "SORTIR", "SORT",
    "TEXT", "TXT",
    "OPTIONS", "OPTION",
    "TITLE", "TITRE", "TITEL",
}

print(f"Vocabulary: {len(LANG_HINTS)} candidates")

# Direct test
direct_fr = []
direct_de = []
LANG_HINTS_LIST = sorted(LANG_HINTS)
for a in LANG_HINTS_LIST:
    for b in LANG_HINTS_LIST:
        if a == b: continue
        d = calc_hash(a) ^ calc_hash(b)
        if d == FR_DELTA:
            direct_fr.append((a, b))
        if d == DE_DELTA:
            direct_de.append((a, b))

print(f"\nDirect FR matches: {len(direct_fr)}")
for a, b in direct_fr:
    print(f"  EN={a!r:12s}  FR={b!r:12s}")
print(f"\nDirect DE matches: {len(direct_de)}")
for a, b in direct_de:
    print(f"  EN={a!r:12s}  DE={b!r:12s}")

# Brute force: find ALL (a, b) where calcHash(a) ^ calcHash(b) == FR_DELTA  
# and at least one is plausible
print(f"\n=== Comprehensive sweep: {FR_DELTA:#010x} (FR), {DE_DELTA:#010x} (DE) ===")
fr_count = 0
de_count = 0
fr_short = []
de_short = []
for h_a, strs_a in h_to_strs.items():
    h_b_fr = h_a ^ FR_DELTA
    h_b_de = h_a ^ DE_DELTA
    if h_b_fr in h_to_strs:
        # Pick shortest representatives
        shortest_a = min(strs_a, key=len)
        shortest_b = min(h_to_strs[h_b_fr], key=len)
        fr_count += len(strs_a) * len(h_to_strs[h_b_fr])
        if len(shortest_a) <= 4 and len(shortest_b) <= 4:
            fr_short.append((shortest_a, shortest_b))
    if h_b_de in h_to_strs:
        shortest_a = min(strs_a, key=len)
        shortest_b = min(h_to_strs[h_b_de], key=len)
        de_count += len(strs_a) * len(h_to_strs[h_b_de])
        if len(shortest_a) <= 4 and len(shortest_b) <= 4:
            de_short.append((shortest_a, shortest_b))

print(f"\nFR sweep: {fr_count:,} total pairs")
print(f"  Short pairs (both ≤4 chars):")
for a, b in fr_short[:30]:
    print(f"    {a:6s}  {b:6s}")

print(f"\nDE sweep: {de_count:,} total pairs")
print(f"  Short pairs (both ≤4 chars):")
for a, b in de_short[:30]:
    print(f"    {a:6s}  {b:6s}")
