#!/usr/bin/env python3
"""STRONGER CONSTRAINT: For ANY base asset, the difference between the FR and DE
forms must be a rotation of 0x01079563 XOR 0x0587801a = 0x04801579, by the
SAME base_shift as for FR-EN and DE-EN deltas. The two rotations are tied,
so calcHash(S_FR) XOR calcHash(S_DE) = 0x04801579 EXACTLY (no rotation).
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

import itertools
TARGET = 0x01079563 ^ 0x0587801a  # 0x04801579, popcount 10
print(f"FR-DE delta target: {TARGET:#010x} (popcount {bin(TARGET).count('1')})")

# Build hash buckets for short alpha and alphanumeric  
ALPHA = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
DIGIT = "0123456789"

print("\nHashing alpha L=1..6...")
h_to_strs = {}
for L in range(1, 7):
    n = 0
    for combo in itertools.product(ALPHA, repeat=L):
        s = "".join(combo)
        h = calc_hash(s)
        h_to_strs.setdefault(h, []).append(s)
        n += 1
    print(f"  L={L}: {n:,} strings, {len(h_to_strs):,} unique hashes")

print(f"\nSearching for pairs with XOR = {TARGET:#010x}...")
pairs = 0
plausible = []
priors = {"US", "FR", "DE", "EN", "GB", "UK", "JP", "GE", "USA", "FRA", "DEU",
          "GER", "ENG", "FRE", "FRC", "FRH", "ENGLISH", "FRENCH", "GERMAN",
          "AME", "AMR", "AMERICAN", "FRANCAIS", "DEUTSCH", "DEUTSCHE",
          "OUI", "JA", "NEIN", "NON", "OK",
          "PAUSE", "PAUSED", "ANGEHALTEN",
          "QUIT", "QUITTER", "BEEND", "BEENDEN",
          "CONTINUE", "WEITER", "FORTSETZEN",
          "GAMEOVER", "ENDE",
          "TITLE", "TITRE", "TITEL",
          "MENU", "MENUE",
          "OPTIONS", "OPTION", "OPTIONEN",
          "LOAD", "SAVE", "CHARGER", "LADEN", "SAUVER", "SPEICHERN",
          "PLAY", "JOUER", "SPIELEN",
          "EXIT", "SORTIE", "SORTIR",
          "BACK", "RETOUR", "ZURUECK", "ZURUCK",
          "START", "DEPART", "BEGINN",
          "WIN", "LOSE", "GAGNE", "PERDU", "GEWINN", "VERLOREN",
          # Also less common
          "PRESS", "DRUECK", "APPUYE", "APPUYER",
          "PUSH", "POUSSER", "DRUECKE",
          "SELECT", "WAEHLE", "SELECTIONNER",
          "SCORE", "PUNKTE",
          "TIME", "TEMPS", "ZEIT",
          "BEST", "MEILLEUR", "BESTE",
          "READY", "PRET", "BEREIT",
          "GO", "ALLEZ", "LOS",
          "FREE", "LIBRE", "FREI",
          "LIVES", "VIE", "VIES", "LEBEN",
          "LEVEL", "LVL", "NIVEAU", "EBENE",
          "WORLD", "MONDE", "WELT",
          "STAGE", "ETAPE", "STUFE",
          "BONUS", "BOM",
          "BOSS", "CHEF", "BOSS",
          "ENEMY", "ENNEMI", "FEIND",
          "FRIEND", "AMI", "FREUND",
          "PLAYER", "JOUEUR", "SPIELER",
          "ITEM", "OBJET", "OBJEKT",
          "POWER", "PUISSANCE", "MACHT",
          "HEALTH", "VITALITY", "GESUNDHEIT",
          "ARMOR", "ARMURE", "RUESTUNG",
          "GOLD", "OR",
          "MONEY", "ARGENT", "GELD",
          "KEY", "CLE", "SCHLUESSEL",
          "DOOR", "PORTE", "TUER",
          "MAP", "CARTE", "KARTE",
          # Numbers
          "ONE", "TWO", "THREE", "UN", "DEUX", "TROIS", "EINS", "ZWEI", "DREI",
          "FIRST", "SECOND", "PREMIER", "SECOND", "ERSTER", "ZWEITER",
          # Skullmonkeys
          "MONKEY", "MONK", "AFFE", "SINGE",
          "SHRINEY", "GUARD", "GARDE", "WACHE",
          # ToolX-style
          "TOOLX", "TLX",
          }
priors = {p.upper() for p in priors}

# Pivot on priors
print(f"\nPriors: {len(priors)}")
prior_hashes = {p: calc_hash(p) for p in priors}

# For each prior, check if prior^TARGET hash exists
hit_pairs = []
for p in priors:
    target_h = prior_hashes[p] ^ TARGET
    if target_h in h_to_strs:
        for partner in h_to_strs[target_h]:
            if partner != p:
                hit_pairs.append((p, partner))

print(f"\nPrior pairs (S_FR or S_DE side):  {len(hit_pairs)}")
for p, partner in hit_pairs[:60]:
    p_in = p in priors
    partner_in = partner in priors
    print(f"  {p:14s} {'*' if p_in else ' '}  {partner:14s} {'*' if partner_in else ' '}")
