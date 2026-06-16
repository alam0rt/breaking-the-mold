#!/usr/bin/env python3
"""Meet-in-the-middle search for suffix pairs (A, B) such that
calcHash(A) ^ calcHash(B) is a rotation of 0x01079563 (popcount 12).

Constrained to plausible language suffix patterns.
"""
import itertools
from collections import defaultdict

def calc_hash(s):
    h = 0; sh = 0
    for c in s.upper():
        if c.isalnum():
            co = ord(c) + 22 if c.isdigit() else ord(c)
            sh = (sh + co - 64) & 31
            h ^= 1 << sh
    return h

def rotations_of(mask):
    return {((mask << r) | (mask >> (32 - r))) & 0xFFFFFFFF for r in range(32)}

FR_TARGETS = rotations_of(0x01079563)
DE_TARGETS = rotations_of(0x00340b0f)

# Build candidate suffix strings — language identifiers, region codes
LANG_SUFFIXES = [
    # English-side
    "ENG", "ENGL", "ENGLISH", "ENGLAND", "AMERICAN", "BRITISH",
    "USA", "US", "UK", "GBR", "GB", "AME", "AMER",
    "EN", "E",
    # French-side
    "FR", "FRE", "FRA", "FREN", "FRENCH", "FRANCAIS", "FRANCAISE",
    "FRANCE", "FRC", "FRH", "FRC", "F",
    "ANGLAIS", "EUROPE", "EUR",
    # German-side
    "DE", "DEU", "GER", "GERM", "GERMAN", "GERMANY", "DEUTSCH", "DEUTSCHLAND",
    "DEUT", "DEU", "GERY", "GERH", "G", "D",
    "ALLEMAND", "ALLEM", "ALEMAN",
    # Mixed/numeric
    "001", "002", "003", "01", "02", "03", "1", "2", "3",
    "USVERSION", "USAVERSION", "FRVERSION", "DEVERSION",
    "USTEXT", "FRTEXT", "DETEXT",
    "USFONT", "FRFONT", "DEFONT",
    # JP indicators (Japan was a separate SKU)
    "JP", "JPN", "JAP", "JAPAN", "JAPANESE", "NIHON", "NIPPON",
    # ASCII alphabet pairs as defaults
]

# Plus many TEXT word translations
TEXT_LITERALS = [
    "QUIT", "QUITTER", "QUITTEZ", "BEENDEN", "BEEND",
    "CONTINUE", "CONTINUER", "FORTSETZEN",
    "OPTIONS", "OPTIONEN",
    "PAUSE", "PAUSED", "ANGEHALTEN",
    "PLAY", "JOUER", "SPIELEN", "SPIEL",
    "SAVE", "SAUVER", "SAUVE", "SPEICHERN",
    "LOAD", "CHARGER", "LADEN",
    "BACK", "RETOUR", "ZURUCK",
    "EXIT", "SORTIR", "BEENDEN",
    "YES", "OUI", "JA",
    "NO", "NON", "NEIN",
    "GAME", "JEU", "SPIEL",
    "OK", "OKAY",
    "MENU", "MENU", "MENU",
    "START", "DEMARRER", "STARTEN",
    "TITLE", "TITRE", "TITEL",
    "NEW", "NOUVEAU", "NEU",
    "SETTINGS", "REGLAGES", "EINSTELLUNGEN",
]

SUFFIX_CANDS = list(set(LANG_SUFFIXES + TEXT_LITERALS))

# Build hash map
h_to_strs = defaultdict(list)
for s in SUFFIX_CANDS:
    h_to_strs[calc_hash(s)].append(s)

print(f"Vocabulary: {len(SUFFIX_CANDS)} candidates, {len(h_to_strs)} unique hashes")

# Find pairs (a, b) where calcHash(a) ^ calcHash(b) ∈ FR_TARGETS
print(f"\n=== EN/FR pairs (delta ∈ rot({0x01079563:#010x})) ===")
hits_fr = []
for h_a, strs_a in h_to_strs.items():
    for tgt in FR_TARGETS:
        h_b = h_a ^ tgt
        if h_b in h_to_strs:
            for a in strs_a:
                for b in h_to_strs[h_b]:
                    if a >= b: continue
                    hits_fr.append((a, b, tgt))
for a, b, tgt in hits_fr:
    print(f"  {a:14s}  {b:14s}  delta={tgt:#010x}")

print(f"\n=== EN/DE pairs (delta ∈ rot({0x00340b0f:#010x})) ===")
hits_de = []
for h_a, strs_a in h_to_strs.items():
    for tgt in DE_TARGETS:
        h_b = h_a ^ tgt
        if h_b in h_to_strs:
            for a in strs_a:
                for b in h_to_strs[h_b]:
                    if a >= b: continue
                    hits_de.append((a, b, tgt))
for a, b, tgt in hits_de:
    print(f"  {a:14s}  {b:14s}  delta={tgt:#010x}")

print(f"\nTotal: FR={len(hits_fr)} DE={len(hits_de)}")
