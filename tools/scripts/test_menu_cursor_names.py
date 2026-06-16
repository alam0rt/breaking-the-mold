#!/usr/bin/env python3
"""Test names for menu cursor highlight (idle) sprites.

From src/menu.c we know:
  0x39900619 = "primary idle button-highlight sprite"
  0x33808E1B = "alternate idle button-highlight sprite (post-flash resting)"

These are MENU button highlights/cursors. Try targeted names.
"""
import sys, csv
sys.path.insert(0, "tools/scripts")
from compound_hash_attack import asset_id

ROOT = "/home/sam/projects/btm"

ids_dict = {}
with open(f"{ROOT}/docs/reference/asset-ids-master.csv") as f:
    rdr = csv.DictReader(f)
    for r in rdr:
        ids_dict[int(r["id_hex"], 16)] = r
ids = set(ids_dict)

# Build all candidate variations
SUBJECTS = ["BUTTON","HIGHLIGHT","HILITE","HILT","CURSOR","BTN","SELECTOR",
            "MENU","MENUBUTTON","MENU_BUTTON","MENUBTN",
            "ITEM","MENUITEM","MENU_ITEM","SELECT","CHOICE","OPTION","OPT",
            "ARROW","POINTER","PTR",
            "FLASH","BLINK","GLOW","SHINE",
            "REST","RESTING","POSTFLASH","POST_FLASH","RESET","RESETTING",
            "SELECTED","UNSELECTED","ACTIVE","INACTIVE","FOCUS","FOCUSED",
            "PRIMARY","SECONDARY","ALT","ALTERNATE","NORMAL","NORM",
            "PASSWORD","OPTIONS","PAUSE","PAUSEMENU","TITLE","TITLEMENU",
            "IDLE","IDLING","STAND","STANDING","WAIT","WAITING","REST"]

ACTIONS = ["IDLE","ACTIVE","INACTIVE","NORMAL","REST","WAIT","STAND",
           "FLASH","BLINK","GLOW","SHINE","SHIMMER","TWINKLE","SPARKLE",
           "ON","OFF","UP","DOWN","HOVER","PRESS","PRESSED","UNPRESSED",
           "FOCUS","UNFOCUS","SELECT","UNSELECT","HOVER","UNHOVER",
           "FAST","SLOW","DELAY","WAIT","HOLD","HOVER",
           "PRIMARY","SECONDARY","SECOND","FIRST","INITIAL","PRIMORDIAL",
           "ANIM","ANIMATION","FRAME","FRAMES","LOOP","LOOPING","CYCLE","CYCLING",
           "RAW","BASE","STATE","STATES",
           "1","2","3","ALT","ALT1","ALT2","COPY","DUP",
           "POSTFLASH","POSTBLINK","POSTGLOW","RESTPOSE","REST_POSE",
           "PAUSED","WAITING","HOLDING","SLEEPING"]

# All combinations + various separators
print("Generating candidates...")
cands = set()
for s in SUBJECTS:
    cands.add(s)
    for a in ACTIONS:
        for c in [s+a, s+"_"+a, a+s, a+"_"+s,
                  s+a+"ANIM", s+"_"+a+"_ANIM", a+s+"ANIM", a+"_"+s+"_ANIM",
                  s+a+"SPR", s+"_"+a+"_SPR", s+a+"FX", s+"_"+a+"_FX",
                  s+a+"ICON", s+"_"+a+"_ICON",
                  s+a+"FRAME", s+"_"+a+"_FRAME",
                  s+a+"LOOP", s+"_"+a+"_LOOP",
                  s+a+"BLINK", s+"_"+a+"_BLINK",
                  s+a+"FLASH", s+"_"+a+"_FLASH",
                  s+a+"1", s+"_"+a+"_1", s+a+"01", s+"_"+a+"_01",
                  s+a+"2", s+"_"+a+"_2", s+a+"02", s+"_"+a+"_02",
                  s+"01", s+"_01", s+"00", s+"_00", s+"02", s+"_02",
                  s+"A", s+"B", s+"C", s+"D",
                  ]:
            cands.add(c)

print(f"  {len(cands):,} candidates")

# Specific targets (menu cursor variants from attack_priorities)
TARGETS = [
    (0x33808e1b, "Menu cursor IDLE alt highlight (from menu.c)"),
    (0x39900619, "Menu cursor IDLE primary highlight"),
    (0x63848e59, "Menu cursor variant (cohort C0256/259)"),
    (0x68c01218, "Menu sprite font (floor 8)"),
    (0x28a0c119, "MENU sprite (floor 5) — exhaustive feasible"),
    (0x38a0c119, "MENU sprite (floor 6)"),
    (0x30a0c119, "MENU sprite (floor 7)"),
    (0x28c080df, "MENU sprite (floor 7)"),
]

print("\n=== Testing targeted ===")
hits = []
for tid, role in TARGETS:
    found = []
    for c in cands:
        if asset_id(c) == tid:
            found.append(c)
    print(f"  0x{tid:08x}  {role:60s}  hits={len(found)}")
    for f in found:
        print(f"    *** {f}")
        hits.append((tid, f, role))

# Also test ALL ids
print("\n=== Testing ALL menu cursor candidates against full ID pool ===")
for c in cands:
    h = asset_id(c)
    if h in ids and ids_dict[h]['kinds'].startswith('sprite') and \
       h not in {0x29C0E211, 0x2AD0F011, 0x0AD0F813, 0x68C0F413, 0x69C04050, 0x69C8F473}:
        # Filter to single-MENU level sprites only
        sources = ids_dict[h]['sources']
        # Limit to those highly likely to be menu (popcount 5-15)
        popc = int(ids_dict[h]['popcount'])
        if 5 <= popc <= 15:
            hits.append((h, c, "global"))
            print(f"  HIT  0x{h:08x}  {c:35s}  popc={popc}")

print(f"\n=== TOTAL: {len(hits)} hits ===")
seen = set()
for tid, name, role in hits:
    key = (tid, name)
    if key in seen: continue
    seen.add(key)
