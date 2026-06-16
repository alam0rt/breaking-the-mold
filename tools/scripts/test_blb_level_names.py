#!/usr/bin/env python3
"""Test BLB level display names + their tokens against the asset hash table.

Display names extracted from sles-01090.blb header:
  MENU → Options
  PHRO → Skullmonkey Gate
  SCIE → Science Center
  TMPL → Monkey Shrines
  FINN → The Amazing Drivey Finn (truncated)
  MEGA → Shriney Guard
  BOIL → Hard Boiler
  SNOW → Sno (or Sno-mo, etc.)
  FOOD → Skullmonkey Brand Ho... (Hot Dog?)
  HEAD → Joe-Head-Joe
  BRG1 → Elevated Structure o...
  GLID → Ynt Death Garden
  CAVE → Ynt Mines
  WEED → Ynt Weeds
  EGGS → Ynt Eggs
  GLEN → Glenn Yntis
  CLOU → Monk Rushmore
  SEVN → 1970's
  SOAR → Soar Head
  CRYS → Shards
  CSTL → Castle de los Muerto(s)
  WIZZ → Monkey Mage
  RUNN → The Incredible Drive(y Joe?)
  MOSS → Worm Graveyard
  KLOG → Klogg
  EVIL → Evil Engine #9
"""
import sys, csv, itertools
sys.path.insert(0, "tools/scripts")
from compound_hash_attack import asset_id

ROOT = "/home/sam/projects/btm"

ids_dict = {}
with open(f"{ROOT}/docs/reference/asset-ids-master.csv") as f:
    rdr = csv.DictReader(f)
    for r in rdr:
        ids_dict[int(r["id_hex"], 16)] = r
ids = set(ids_dict)
KNOWN = {
    asset_id("NO"), asset_id("YES"), asset_id("PAUSED"),
    asset_id("QUIT"), asset_id("CONTINUE"), asset_id("QUITGAME"),
}

# Full display names from BLB
LEVELS = [
    ("MENU", "Options"),
    ("PHRO", "Skullmonkey Gate"),
    ("SCIE", "Science Center"),
    ("TMPL", "Monkey Shrines"),
    ("FINN", "The Amazing Drivey Finn"),
    ("MEGA", "Shriney Guard"),
    ("BOIL", "Hard Boiler"),
    ("SNOW", "Sno"),
    ("FOOD", "Skullmonkey Brand Hot Dog"),  # guess
    ("HEAD", "Joe-Head-Joe"),
    ("BRG1", "Elevated Structure of Mosk"),  # guess
    ("GLID", "Ynt Death Garden"),
    ("CAVE", "Ynt Mines"),
    ("WEED", "Ynt Weeds"),
    ("EGGS", "Ynt Eggs"),
    ("GLEN", "Glenn Yntis"),
    ("CLOU", "Monk Rushmore"),
    ("SEVN", "1970's"),
    ("SOAR", "Soar Head"),
    ("CRYS", "Shards"),
    ("CSTL", "Castle de los Muertos"),
    ("WIZZ", "Monkey Mage"),
    ("RUNN", "The Incredible Drivey Joe"),  # guess
    ("MOSS", "Worm Graveyard"),
    ("KLOG", "Klogg"),
    ("EVIL", "Evil Engine #9"),
]

# Try direct match
print("=== Test 1: Full display names ===")
hits = []
for code, disp in LEVELS:
    h = asset_id(disp)
    is_known = h in KNOWN
    in_ids = h in ids
    marker = "✓" if in_ids else " "
    print(f"  {marker}  {code}: '{disp}' -> 0x{h:08x}{'  KNOWN' if is_known else (' MATCH' if in_ids else '')}")
    if in_ids and not is_known:
        hits.append((h, disp, "level_display_name"))

# Try with code prefix/suffix
print("\n=== Test 2: code + display name ===")
for code, disp in LEVELS:
    for variant in [code+disp, disp+code, code+"_"+disp, disp+"_"+code,
                    code+disp.upper(), code+"_"+disp.upper(),
                    disp.upper(), disp.lower(),
                    disp.replace(' ', ''), disp.replace(' ', '_'),
                    disp.replace('-', ''), disp.replace('-', '_'),
                    code+"END", code+"START", code+"INTRO", code+"OUTRO",
                    code+"BG", code+"BACK", code+"SKY", code+"FLOOR",
                    code+"BOSS", code+"LEVEL", code+"STAGE", code+"SCREEN",
                    code+"MAP", code+"PIC", code+"ICON", code+"NAME",
                    "LEVEL"+code, "STAGE"+code, "WORLD"+code,
                    code+"01", code+"02", code+"1", code+"2",
                    code+"_01", code+"_02", code+"_1", code+"_2",
                    code.lower(), code.title()]:
        h = asset_id(variant)
        if h in ids and h not in KNOWN:
            hits.append((h, variant, f"level={code} disp={disp}"))
            print(f"  HIT  0x{h:08x}  '{variant}'  level={code}")

# Tokenize display names and try compounds
print("\n=== Test 3: Tokenize and compound ===")
all_tokens = set()
for code, disp in LEVELS:
    # Tokenize
    import re
    tokens = re.findall(r"[A-Za-z]+", disp)
    for t in tokens:
        all_tokens.add(t.upper())
    all_tokens.add(code)

print(f"Total tokens: {len(all_tokens)}")
print(f"Tokens: {sorted(all_tokens)}")

# Test single tokens
print("\n=== Test 4: Single tokens ===")
for t in sorted(all_tokens):
    h = asset_id(t)
    if h in ids and h not in KNOWN:
        hits.append((h, t, "tokenized_word"))
        print(f"  HIT  0x{h:08x}  '{t}'")

# Test 2-token combinations
print("\n=== Test 5: 2-token combinations of display tokens ===")
tested_pairs = 0
for t1 in all_tokens:
    for t2 in all_tokens:
        for sep in ["", "_", "-"]:
            v = t1 + sep + t2
            h = asset_id(v)
            tested_pairs += 1
            if h in ids and h not in KNOWN:
                hits.append((h, v, "level_token_pair"))
                print(f"  HIT  0x{h:08x}  '{v}'")

print(f"  Tested {tested_pairs:,} pairs")

# Save
print(f"\n=== TOTAL: {len(hits)} hits ===")
seen = set()
for h, c, ctx in hits:
    if (h, c) in seen: continue
    seen.add((h, c))

if hits:
    with open(f"{ROOT}/docs/analysis/asset-identification/blb_level_name_hits.csv", "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["id_hex","candidate","context"])
        seen = set()
        for h, c, ctx in hits:
            if (h, c) in seen: continue
            seen.add((h, c))
            w.writerow([f"0x{h:08x}", c, ctx])
    print(f"\nSaved hits to: docs/analysis/asset-identification/blb_level_name_hits.csv")
