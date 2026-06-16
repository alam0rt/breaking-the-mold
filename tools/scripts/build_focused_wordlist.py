#!/usr/bin/env python3
"""Generate FOCUSED wordlist (game vocab only, no English flood)."""
import csv, re
from pathlib import Path

ROOT = Path("/home/sam/projects/btm")

words = set()

# Level codes
words.update("MENU PHRO SCIE TMPL FINN MEGA BOIL SNOW FOOD HEAD BRG1 GLID CAVE WEED EGGS GLEN CLOU SEVN SOAR CRYS CSTL WIZZ RUNN MOSS KLOG EVIL".split())

# Display name tokens
words.update(["AMAZING","BOIL","BOILER","BRAND","CASTLE","CAVE","CENTER","CRYS",
    "DEATH","DOG","DRIVEY","EGGS","ELEVATED","ENGINE","EVIL","FINN","FOOD",
    "GARDEN","GATE","GLENN","GRAVEYARD","GUARD","HARD","HEAD","HOT",
    "INCREDIBLE","JOE","KLOG","KLOGG","LOS","MAGE","MENU","MINES","MONK",
    "MONKEY","MOSK","MOSS","MUERTOS","OPTIONS","PHRO","RUNN","RUSHMORE",
    "SCIENCE","SEVN","SHARDS","SHRINES","SHRINEY","SKULLMONKEY",
    "SKULLMONKEYS","SNO","SNOW","SOAR","STRUCTURE","TMPL","WEED","WEEDS",
    "WIZZ","WORM","YNT","YNTIS"])

# Character names
words.update(["FINN","JOE","JOEHEAD","KLAYMEN","KLAY","HOBORG","KLOGG",
    "GLENN","YNTIS","MAGE","MONKEY","MONK","PHARTHEAD","PHART","PLAYER",
    "BOSS","ENEMY","SHRINEY","GUARD","WIZZ","RUNN","ROBOT","WORM",
    "WILLIE","TROMBONE","HAMSTER","SHIELD","CLAYBALL","BALL","SKULL",
    "SKULLMONKEY","SAGE","WORMWIZARD"])

# Actions / states
words.update(["IDLE","STAND","REST","WAIT","WALK","RUN","MOVE",
    "JUMP","FALL","DROP","FLY","FLOAT","HOVER","SWIM","SPLASH",
    "ATTACK","ATK","HIT","STRIKE","PUNCH","KICK","BASH","SLASH",
    "DEATH","DIE","KILL","DEFEAT","DESTROY","BREAK","SMASH",
    "DAMAGE","DMG","HURT","OUCH","PAIN","HEAL","HEALED",
    "SPAWN","INIT","BIRTH","CREATE","NEW","BORN",
    "PICKUP","PICK","GRAB","TAKE","GET","HOLD",
    "ROLL","ROLLING","SPIN","ROTATE",
    "EXPLODE","BLAST","BOOM","BURST","SHATTER","CRACK",
    "DEBRIS","PIECE","FRAG","PART","CHUNK","BIT",
    "PARTICLE","FX","EFFECT","SFX","PFX",
    "PROJECTILE","BULLET","SHOT","SHOOT","FIRE",
    "PORTAL","WARP","TELEPORT","GATE","DOOR",
    "VICTORY","WIN","WON","TRIUMPH","SUCCESS",
    "INTRO","OUTRO","BEGIN","START","END","FINAL","FINISH",
    "OPEN","CLOSE","PRESS","RELEASE","TOGGLE",
    "FLASH","BLINK","GLOW","SHINE","TWINKLE","SPARKLE",
    "BUTTON","BTN","ICON","ICN","IMAGE","PIC","SPRITE","SPR","ANIM","ANM",
    "FRAME","LOOP","CYCLE","STATE",
    "SOUND","SND","VOICE","VOX","NOISE","MUSIC","MUS","SAMPLE",
    "FONT","FNT","TEXT","TXT","STR","STRING","LBL","LABEL",
    "HUD","BAR","METER","GAUGE","INDICATOR",
    "OPTION","ITEM","SELECT","CHOOSE","MENU","MNU",
    "MAP","MAPS","WORLD","STAGE","LEVEL","ZONE","AREA",
    "ROOM","DOOR","WALL","FLOOR","CEILING","TILE","TILES",
    "SKY","SKYS","CLOUD","CLOUDS","STAR","STARS",
    "BG","BACK","BACKGROUND","FG","FORE","FOREGROUND","MID",
    "READY","DONE","ACTIVE","INACTIVE","ENABLED","DISABLED"])

# Items
words.update(["COIN","COINS","GEM","GEMS","DIAMOND","JEWEL","STAR","STARS",
    "KEY","KEYS","LOCK","UNLOCK","POWERUP","POWER","BOOST",
    "SHIELD","ARMOR","HELMET","CAP","HAT",
    "HEALTH","HP","ENERGY","MANA","MP","STAMINA",
    "AMMO","BULLETS","WEAPON","SWORD","BOW","GUN",
    "POTION","FLASK","BOTTLE","JAR",
    "FOOD","FRUIT","APPLE","CHEESE","BREAD","CAKE","DONUT","PIZZA",
    "ITEM","COLLECTIBLE","COLLECT","BONUS","REWARD","TREASURE",
    "GOLD","SILVER","BRONZE","COPPER",
    "NORM","NORMAL","BIG","SMALL","TINY","LARGE","HUGE","MASSIVE","ALT",
    "RED","GREEN","BLUE","YELLOW","PURPLE","ORANGE","PINK","CYAN","WHITE","BLACK","GREY","GRAY",
    "FIRE","ICE","WATER","AIR","EARTH","WIND","LIGHTNING","ELECTRIC"])

# Numbers and short codes
for i in range(0, 21):
    words.add(str(i))
    words.add(f"{i:02d}")
words.update(["X","Y","Z","O","N","E","S","W","UP","DN","LT","RT","NORTH","SOUTH","EAST","WEST"])
words.update(["L1","L2","L3","R1","R2","R3","S1","S2","S3","P1","P2"])

# Single letters
for c in "ABCDEFGHIJKLMNOPQRSTUVWXYZ":
    words.add(c)

# Function name tokens
fn_csv = ROOT / "docs/analysis/asset-identification/asset_usage_by_function.csv"
if fn_csv.exists():
    with open(fn_csv) as f:
        rdr = csv.DictReader(f)
        for r in rdr:
            fn_name = r.get("function", "")
            tokens = re.findall(r"[A-Z][a-z]+|[A-Z]+(?=[A-Z]|$)|\d+", fn_name)
            for t in tokens:
                tu = t.upper()
                if 2 <= len(tu) <= 12:
                    words.add(tu)

# UI text words
words.update(["NO","YES","PAUSED","QUIT","CONTINUE","QUITGAME","PASSWORD","LOAD","SAVE",
    "TITLE","TITLESCREEN","MAIN","CREDITS","STAFF","HELP","INFO","BACK","RETURN",
    "OK","CANCEL","CONFIRM","SURE","REALLY","ARE","YOU","WANT","TO","EXIT",
    "PLAY","NEW","SELECT","CHOOSE","ENTER","ENTRY","CODE","INPUT",
    "GAMEOVER","CLEAR","COMPLETE","OVER","TIMEUP","TIMEOUT","FAILURE","SUCCESS",
    "READY","GO","FIGHT","BEGIN","WAIT","STOP","RESUME","RETRY","RESTART",
    "OPTIONS","SETTINGS","CONFIG","CONTROLS","CONTROLS","SOUND","VOLUME","MUSIC","SFX",
    "VIBRATE","RUMBLE","SHAKE","CONTROLLER",
    "EASY","NORMAL","HARD","EXPERT","DIFFICULTY",
    "1UP","ONEUP","2UP","TWOUP","LIVES","LIFE","SCORE","BONUS","HIGH",
    "STAGECLEAR","LEVELCLEAR","WORLDCLEAR","BOSSCLEAR",
    "MENUUP","MENUDOWN","MENULEFT","MENURIGHT","MENUSELECT","MENUBACK"])

# Save
out = ROOT / "tools/data/game_focused_wordlist.txt"
out.parent.mkdir(parents=True, exist_ok=True)
sw = sorted(words)
with open(out, "w") as f:
    for w in sw:
        f.write(w + "\n")
print(f"Total unique words: {len(sw):,}")
print(f"Saved to {out}")
