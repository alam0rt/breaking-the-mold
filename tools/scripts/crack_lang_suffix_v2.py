#!/usr/bin/env python3
"""Direct test of plausible language-tag pairs against the FR/DE deltas.

calcHash(EN_SUF) ^ calcHash(FR_SUF) must be in rotations(0x01079563)
calcHash(EN_SUF) ^ calcHash(DE_SUF) must be in rotations(0x00340b0f)
"""

def calc_hash(s):
    h = 0; sh = 0
    for c in s.upper():
        if c.isalnum():
            co = ord(c) + 22 if c.isdigit() else ord(c)
            sh = (sh + co - 64) & 31
            h ^= 1 << sh
    return h

def rotations(mask):
    return {((mask << r) | (mask >> (32 - r))) & 0xFFFFFFFF for r in range(32)}

FR_BASE = 0x01079563
DE_BASE = 0x00340b0f
FR_R = rotations(FR_BASE)
DE_R = rotations(DE_BASE)

# Candidate language-related strings/tags
EN_CAND = [
    "", "E", "EN", "ENG", "ENGL", "ENGLISH", "ENGUS", "ENGUK",
    "US", "USA", "UK", "GB", "AM", "AMER", "AMERICAN",
    "PAL", "NTSC", "EUR", "EU", "EUROPE", "EUROPEAN",
    "AMERICAN", "AMERICA", "BRITISH", "BRITAIN",
    "EN_US", "EN_UK", "EN_GB",
    "EUS", "EFR", "EGE", "ENF", "ENG_US", "ENG_GB",
    # Prefix variants
    "ANGL", "ANGLE", "ENGENG",
]
FR_CAND = [
    "F", "FR", "FRA", "FRE", "FREN", "FRENC", "FRENCH",
    "FRANCAIS", "FRANCE", "FRANC", "FREE", "FRZ", "FRRU",
    "FRA_FR", "F_FR", "PFR", "FFR", "FR_FR", "FRH",
    "FRANC", "FRANCE", "FRANCAISE",
    "GAL", "GAUL", "GAULOIS",  # historical/joke
]
DE_CAND = [
    "D", "DE", "DEU", "DEUT", "DEUTS", "DEUTSCH",
    "GE", "GER", "GERM", "GERMAN", "GERMANY", "GERY",
    "AL", "ALEM", "ALLEM", "ALLEMAND",
    "DE_DE", "D_DE", "GERY", "GERH",
]

# For each EN candidate, check FR/DE matches
print(f"=== EN/FR pairs where calcHash(EN)^calcHash(FR) ∈ rot({FR_BASE:#010x}) ===")
fr_hits = []
for en in EN_CAND:
    for fr in FR_CAND:
        if en == fr: continue
        d = calc_hash(en) ^ calc_hash(fr)
        if d in FR_R:
            fr_hits.append((en, fr, d))
            r = next(r for r in range(32) if ((FR_BASE << r) | (FR_BASE >> (32-r))) & 0xFFFFFFFF == d)
            print(f"  EN={en!r:12s}  FR={fr!r:12s}  delta={d:#010x}  rot={r}")

print(f"\n=== EN/DE pairs where calcHash(EN)^calcHash(DE) ∈ rot({DE_BASE:#010x}) ===")
de_hits = []
for en in EN_CAND:
    for de in DE_CAND:
        if en == de: continue
        d = calc_hash(en) ^ calc_hash(de)
        if d in DE_R:
            de_hits.append((en, de, d))
            r = next(r for r in range(32) if ((DE_BASE << r) | (DE_BASE >> (32-r))) & 0xFFFFFFFF == d)
            print(f"  EN={en!r:12s}  DE={de!r:12s}  delta={d:#010x}  rot={r}")

if fr_hits and de_hits:
    print("\n=== Triples (same EN candidate hits both FR and DE) ===")
    fr_by_en = {}
    de_by_en = {}
    for en, fr, d in fr_hits:
        fr_by_en.setdefault(en, []).append((fr, d))
    for en, de, d in de_hits:
        de_by_en.setdefault(en, []).append((de, d))
    for en in fr_by_en:
        if en in de_by_en:
            for fr, fd in fr_by_en[en]:
                for de, dd in de_by_en[en]:
                    print(f"  EN={en!r:12s}  FR={fr!r:12s}  DE={de!r:12s}")

print(f"\nTotal FR hits: {len(fr_hits)}, DE hits: {len(de_hits)}")
