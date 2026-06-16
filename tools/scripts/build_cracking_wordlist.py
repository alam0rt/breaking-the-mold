#!/usr/bin/env python3
"""Generate wordlist for klash_combine attack.

Combines:
  - Level codes (BLB)
  - Display name tokens (BLB)  
  - Character names + variants
  - Action verbs
  - Function-context-derived tokens (parsed from asset_usage_by_function.csv)
  - Common English words 3-9 chars
  - Numbers 0-100
  - Single letters
"""
import csv, re, os
from pathlib import Path

ROOT = Path("/home/sam/projects/btm")

words = set()

# Level codes from BLB
LEVELS = "MENU PHRO SCIE TMPL FINN MEGA BOIL SNOW FOOD HEAD BRG1 GLID CAVE WEED EGGS GLEN CLOU SEVN SOAR CRYS CSTL WIZZ RUNN MOSS KLOG EVIL".split()
words.update(LEVELS)

# Display name tokens from BLB
DISPLAY_TOKENS = ["AMAZING","BOIL","BOILER","BRAND","BRG1","CASTLE","CAVE",
    "CENTER","CLOU","CRYS","CSTL","DEATH","DOG","DRIVEY","EGGS","ELEVATED",
    "ENGINE","EVIL","FINN","FOOD","GARDEN","GATE","GLEN","GLENN","GLID",
    "GRAVEYARD","GUARD","HARD","HEAD","HOT","INCREDIBLE","JOE","KLOG","KLOGG",
    "LOS","MAGE","MEGA","MENU","MINES","MONK","MONKEY","MOSK","MOSS","MUERTOS",
    "OPTIONS","PHRO","RUNN","RUSHMORE","SCIE","SCIENCE","SEVN","SHARDS",
    "SHRINES","SHRINEY","SKULLMONKEY","SKULLMONKEYS","SNO","SNOW","SOAR",
    "STRUCTURE","TMPL","WEED","WEEDS","WIZZ","WORM","YNT","YNTIS",
    # Singularize
    "OPTION", "SHARD", "MINE", "WEED", "EGG", "SHRINE", "MONKEY"]
words.update(DISPLAY_TOKENS)

# Character / boss names + variants
CHARS = ["FINN","JOE","JOEHEAD","JOEHEADJOE","KLAYMEN","KLAY","HOBORG",
    "KLOGG","KLOG","GLENN","GLENNYNTIS","YNTIS","YNT","YNTI","OXTAY",
    "MONKEYMAGE","MAGE","MONKEY","MONK","MONKEYS","MAGICMONKEY",
    "PHARTHEAD","PHART","PLAYER","BOSS","ENEMY","BABY","BABIES",
    "SHRINEY","SHRINEYGUARD","GUARD","GUARDIAN","WIZZ","WIZARD","SAGE","SHRINEYSAGE",
    "RUNN","ROBOT","WORM","WORMWIZARD","WORMWORM","WORMS",
    "WILLIE","WILLIETROMBONE","TROMBONE","JAZZWILLIE","WILLIEJAZZ",
    "HAMSTER","HAMSTERSHIELD","SHIELD","HAMSTERWITHSHIELD",
    "CLAYBALL","BALL","CLAY",
    "SKULLMONKEY","SKULLMONKEYS","SKULL","SKULLMONK","MONK",
]
words.update(CHARS)

# Action verbs / states
ACTIONS = ["IDLE","STAND","STANDING","REST","RESTING","WAIT","WAITING","POSE",
    "WALK","WALKING","RUN","RUNNING","MOVE","MOVING","GO","GOING",
    "JUMP","JUMPING","JMP","LEAP","FALL","FALLING","DROP","DROPPING",
    "FLY","FLYING","FLOAT","FLOATING","HOVER","HOVERING","BOB","BOBBING",
    "ATTACK","ATK","ATTACKING","HIT","HITTING","STRIKE","PUNCH","KICK","BASH","SLASH",
    "DEATH","DIE","DYING","DEAD","KO","KILLED","DEFEAT","DEFEATED","DESTROY",
    "DAMAGE","DMG","HURT","HURTING","OUCH","PAIN",
    "SWIM","SWIMMING","WATER","SPLASH","WET",
    "SPAWN","SPAWNING","INIT","BIRTH","CREATE","NEW","MAKE","BORN",
    "PICKUP","PICK","GRAB","GRABBING","TAKE","GET","HOLD","HOLDING",
    "ROLL","ROLLING","ROLLED","SPIN","SPINNING","ROTATE","ROTATING",
    "EXPLODE","BLAST","BOOM","BURST","BURSTING","SHATTER","SHATTERING",
    "BREAK","BREAKING","SMASH","SMASHING","CRACK","CRACKING",
    "DEBRIS","PIECE","PIECES","FRAG","PARTS","BITS","CHUNK","CHUNKS",
    "PARTICLE","PCL","PART","FX","EFFECT","EFFECTS","SFX",
    "PROJECTILE","PROJ","BULLET","SHOT","SHOOT","FIRE","FIRING",
    "PORTAL","WARP","TELEPORT","GATE","DOORWAY",
    "CUTSCENE","CINEMA","MOVIE","SCENE","STORY","STORY",
    "VICTORY","WIN","WON","WINNING","TRIUMPH","SUCCESS",
    "INTRO","OUTRO","BEGIN","START","END","ENDING","FINAL","FINISH",
    "OPEN","CLOSE","OPENING","CLOSING","PRESS","RELEASE",
    "FLASH","FLASHING","BLINK","BLINKING","GLOW","GLOWING","SHINE","SHINING","TWINKLE",
    "BUTTON","BTN","B","ICON","ICN","IMAGE","PIC","SPRITE","SPR","ANIM","ANM","ANIMATION",
    "FRAME","FRAMES","LOOP","LOOPING","CYCLE","STATE","STATES",
    "SOUND","SND","VOICE","VOX","NOISE","MUSIC","MUS","SAMPLE","SMP",
    "FONT","FNT","TEXT","TXT","STR","STRING",
    "HUD","BAR","METER","GAUGE","INDICATOR",
    "MENU","MNU","MENUOPTION","MENUITEM",
    "MAP","MAPS","WORLD","STAGE","LEVEL","ZONE","AREA",
    "ROOM","ROOMS","DOOR","DOORS","WALL","WALLS","FLOOR","FLOORS",
    "TILE","TILES","TILEMAP",
    "SKY","SKYS","CLOUD","CLOUDS",
    "BG","BACK","BACKGROUND","FG","FORE","FOREGROUND","MID","MIDGROUND",
]
words.update(ACTIONS)

# Items / collectibles
ITEMS = ["COIN","COINS","GEM","GEMS","DIAMOND","JEWEL","STAR","STARS",
    "KEY","KEYS","LOCK","LOCKED","UNLOCK",
    "POWERUP","POWERUPS","POWER","UP","DOWN","BOOST",
    "SHIELD","ARMOR","HELMET","CAP","HAT",
    "HEALTH","HP","ENERGY","EN","MANA","MP","STAMINA","SP",
    "AMMO","BULLETS","WEAPON","SWORD","BOW","GUN",
    "POTION","FLASK","BOTTLE","JAR",
    "FOOD","FRUIT","APPLE","CHEESE","BREAD","CAKE","DONUT","PIZZA","HAMBURGER",
    "ITEM","ITEMS","COLLECTIBLE","COLLECT","PICKUP","BONUS","REWARD","TREASURE",
    "GOLD","SILVER","BRONZE","COPPER","BRASS",
    "NORM","NORMAL","BIG","SMALL","TINY","LARGE","HUGE","MASSIVE",
    "RED","GREEN","BLUE","YELLOW","PURPLE","ORANGE","PINK","CYAN","WHITE","BLACK",
]
words.update(ITEMS)

# Numbers
for i in range(0, 31):
    words.add(str(i))
    words.add(f"{i:02d}")

# Single letters and short codes
for c in "ABCDEFGHIJKLMNOPQRSTUVWXYZ":
    words.add(c)

# Common 2-3 letter codes
words.update(["FX","SFX","PFX","ANM","SPR","FNT","ICN","BTN","PIC","BG","FG",
              "L1","L2","L3","R1","R2","R3","S1","S2","S3","T1","T2","T3",
              "P1","P2","B1","B2","M1","M2","M3","M4","M5",
              "X","Y","Z","O","N","E","S","W","UP","DN","LT","RT"])

# Add English wordlist (3-10 char uppercase alphabetic)
for path in ["/tmp/words_short.txt", "/usr/share/dict/words"]:
    if os.path.exists(path):
        with open(path) as f:
            for line in f:
                w = line.strip().upper()
                if 3 <= len(w) <= 10 and w.isalpha():
                    words.add(w)
        break

# Words from function names parsed
fn_csv = ROOT / "docs/analysis/asset-identification/asset_usage_by_function.csv"
if fn_csv.exists():
    with open(fn_csv) as f:
        rdr = csv.DictReader(f)
        for r in rdr:
            fn_name = r.get("function", "")
            # Tokenize CamelCase
            tokens = re.findall(r"[A-Z][a-z]+|[A-Z]+(?=[A-Z]|$)|\d+", fn_name)
            for t in tokens:
                tu = t.upper()
                if 2 <= len(tu) <= 12:
                    words.add(tu)

# Save wordlist
out_path = ROOT / "tools/data/cracking_wordlist.txt"
out_path.parent.mkdir(parents=True, exist_ok=True)
sorted_words = sorted(words)
print(f"Total unique words: {len(sorted_words):,}")

# Limit to reasonable size for performance
# Drop very long English words that are unlikely to be used
filtered = [w for w in sorted_words if 1 <= len(w) <= 12]
print(f"After filter (1-12 char): {len(filtered):,}")

with open(out_path, "w") as f:
    for w in filtered:
        f.write(w + "\n")

print(f"Saved to: {out_path}")
