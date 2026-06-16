#!/usr/bin/env python3
"""Test pause-menu / UI text strings against the asset table.

The 6 known anchors are ALL pause menu words: NO, YES, PAUSED, QUIT,
CONTINUE, QUITGAME. Hypothesis: many of the asset IDs are TEXT STRINGS
that get rendered through the font system, not sprite assets.

Try every plausible UI text string from a PSX game.
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

# UI text - core anchors known
KNOWN = {
    asset_id("NO"), asset_id("YES"), asset_id("PAUSED"),
    asset_id("QUIT"), asset_id("CONTINUE"), asset_id("QUITGAME"),
}

# Comprehensive UI text vocabulary
UI_WORDS = [
    # Title / main menu
    "TITLE", "TITLESCREEN", "TITLEMENU", "MAINMENU", "MAIN", "MENU",
    "GAME", "PLAY", "PLAYGAME", "NEWGAME", "STARTGAME", "START", "BEGIN",
    "EXIT", "EXITGAME", "QUIT", "QUITGAME", "CONTINUE", "CONTINUEGAME",
    "BACK", "RETURN", "DONE", "OK", "OKAY", "CANCEL", "CONFIRM",
    "SELECT", "CHOOSE", "PICK", "ENTER", "NEXT", "PREVIOUS", "PREV",
    "RESUME", "RESTART", "RETRY", "TRYAGAIN", "GIVEUP",
    "OPTIONS", "OPTION", "SETTINGS", "CONFIG", "CONFIGURE", "CONTROLS",
    "HELP", "ABOUT", "INFO", "INFORMATION", "CREDITS", "STAFF",
    
    # Save / Load
    "SAVE", "SAVEGAME", "SAVING", "SAVED",
    "LOAD", "LOADGAME", "LOADING", "LOADED",
    "PASSWORD", "PASSWORDS", "CODE", "CODES",
    "MEMORYCARD", "CARD", "MEMCARD", "MEMORY",
    "ERASE", "DELETE", "OVERWRITE", "REPLACE",
    "EMPTY", "FULL", "FREE", "USED",
    "BLOCK", "BLOCKS", "SLOT", "SLOTS",
    
    # Pause menu
    "PAUSE", "PAUSED", "PAUSEMENU", "PAUSING",
    "STOP", "STOPPED", "WAIT", "WAITING",
    
    # Confirmation
    "YES", "NO", "MAYBE", "SURE", "AREYOUSURE", "QUITYESNO",
    
    # Game state
    "GAMEOVER", "GAMECLEAR", "GAMECOMPLETE", "GAMEEND", "GAMEEND",
    "LEVELCLEAR", "LEVELCOMPLETE", "STAGECLEAR", "STAGECOMPLETE",
    "WIN", "WINNER", "WINNING", "VICTORY",
    "LOSE", "LOSER", "LOST", "DEFEAT",
    "READY", "GO", "FIGHT", "BEGIN",
    "DEAD", "DIED", "DYING", "DEATH",
    "ALIVE", "LIVES", "LIFE", "EXTRALIFE", "ONEUP", "1UP", "TWOUP", "2UP",
    "TIME", "TIMEUP", "TIMER", "TIMING",
    
    # Score / Stats
    "SCORE", "SCORES", "POINTS", "POINT", "PTS",
    "BONUS", "EXTRA", "BIG",
    "HIGH", "HIGHSCORE", "HIGHSCORES", "BESTTIME", "BEST",
    "RECORD", "RECORDS", "STATS", "STATISTICS",
    "TOTAL", "FINAL",
    "COMBO", "MULTIPLIER",
    "RANK", "RANKING", "PLACE",
    "GOLD", "SILVER", "BRONZE", "PERFECT",
    
    # Levels
    "LEVEL", "STAGE", "WORLD", "ZONE", "AREA",
    "MISSION", "QUEST", "TASK", "OBJECTIVE",
    "CHAPTER", "EPISODE", "SCENE",
    "ROOM", "ROOMS", "MAP", "WORLD",
    "BOSS", "MINIBOSS", "SUBBOSS",
    
    # Items / collectibles  
    "ITEM", "ITEMS", "COIN", "COINS", "GEM", "GEMS",
    "KEY", "KEYS", "STAR", "STARS",
    "POWERUP", "POWERUPS", "SHIELD", "ARMOR",
    "HEALTH", "HP", "ENERGY", "POWER", "MANA",
    "AMMO", "BULLETS", "WEAPON",
    "INVENTORY", "BAG", "POCKET",
    
    # HUD
    "HUD", "BAR", "METER", "GAUGE",
    "ICON", "ICONS", "LOGO",
    
    # Direction / controls
    "UP", "DOWN", "LEFT", "RIGHT",
    "PUSH", "PULL", "MOVE", "JUMP", "ATTACK",
    "PRESS", "HOLD", "RELEASE",
    "BUTTON", "STICK", "DPAD", "PAD",
    "X", "O", "TRIANGLE", "SQUARE", "CROSS", "CIRCLE",
    "L1", "L2", "R1", "R2", "SELECT", "START",
    
    # Common phrases
    "PRESSSTART", "PRESSBUTTON", "PRESSANY", "PRESSANYBUTTON",
    "TAPSTART", "TAPBUTTON",
    "PLEASEWAIT", "WAITING",
    "ARE", "YOU", "SURE", "WANT", "TO",
    "REALLY", "REALLYQUIT",
    
    # Numbers & text
    "ZERO", "ONE", "TWO", "THREE", "FOUR", "FIVE", "SIX", "SEVEN", "EIGHT", "NINE", "TEN",
    "FIRST", "SECOND", "THIRD",
    "X1", "X2", "X3", "X10", "X100",
    
    # SkullMonkeys specific
    "SKULLMONKEYS", "NEVERHOOD", "KLAYMEN", "KLOG", "JOE", "JOEHEAD",
    "WILLIE", "WILLIETROMBONE", "GLENNYNTIS", "GLENN", "YNTIS",
    "ROBOT", "ROBOTS", "MONKEY", "MONKEYS",
    "RUNNTIME", "RUNNTHRU", "RUNNTHROUGH",
    "EYE", "EYEBOSS",
    "MONKEYMAGE", "PHARTHEAD", "WEEDOUT", "WEEDS",
    "CONTROLS", "CONTROLLER",
    
    # Tutorial / popup
    "PRESS", "TAP", "HOLD", "WAIT",
    "JUMP", "ATTACK", "HIT", "DUCK", "CROUCH",
    "TUTORIAL", "PROLOGUE", "EPILOGUE",
    
    # Sound related (might match)
    "VOLUME", "SOUND", "MUSIC", "VOICE", "SFX",
    "STEREO", "MONO", "LANGUAGE", "SUBTITLES",
    
    # Other
    "EMPTY", "FULL", "OFF", "ON", "AUTO", "MANUAL",
    "EASY", "NORMAL", "HARD", "EXPERT", "DIFFICULTY",
    "COMPLETE", "INCOMPLETE", "DONE", "NOTDONE", "FINISHED", "UNFINISHED",
]

# Lowercase / mixed case variations
all_cands = set()
for w in UI_WORDS:
    all_cands.add(w)
    all_cands.add(w.lower())
    all_cands.add(w.capitalize())
    # Try with prefixes/suffixes
    for prefix in ["", "TXT_", "STR_", "MSG_", "TEXT_", "MENU_", "UI_", "LBL_", "LABEL_"]:
        for suffix in ["", "_TXT", "_STR", "_MSG", "_TEXT", "_LBL", "_LABEL"]:
            all_cands.add(prefix+w+suffix)

# Common pause menu compound phrases
PHRASES = [
    "QUITTOTITLE", "QUITTOMAIN", "QUITTODESKTOP", "QUITGAMEYESNO",
    "RESUMEPLAY", "RESUMEGAME", "RESTARTLEVEL", "RESTARTSTAGE",
    "MAINMENU", "TITLESCREEN", "GOTOTITLE", "BACKTOTITLE",
    "BACKTOMENU", "BACKTOGAME", "RETURNTOMENU", "RETURNTOGAME",
    "GAMEOVER", "TIMEUP", "BONUSROUND", "BONUSGAME",
    "TIPS", "TIP", "HINT", "HINTS", "HELP", "INFO",
    "GETREADY", "GETSET", "READYSTART", "READYGO",
    "PRESSANY", "PRESSANYKEY", "PRESSSTARTBUTTON", "PRESSXTOSTART",
    "ENTERPASSWORD", "ENTERCODE", "ENTERPSWD",
    "PASSWORDOK", "PASSWORDOKAY", "PASSWORDFAIL", "WRONGPASSWORD",
    "PASSWORDINVALID", "PASSWORDVALID",
    "INPUTPASSWORD",
    # Common Skullmonkeys-likely phrases
    "PLAYAGAIN", "TRYAGAIN", "RETRY",
    "1PLAYER", "2PLAYER", "ONEPLAYER", "TWOPLAYER",
    "MUSICONOFF", "SOUNDONOFF",
    "BEGINGAME", "BEGINPLAY", "STARTPLAY", "STARTNEW",
    "LOADSAVE", "LOADGAME",
    "SAVEGAME", "SAVEAS",
    # Pause menu specific
    "PAUSEHEADER", "PAUSETITLE", "PAUSEFOOTER",
    "MAINPAUSE", "PAUSEMAIN",
    "QUITPROMPT", "QUITQUESTION", "QUITCONFIRM",
    "AREYOUSURE", "AREYOUSUREYOUWANT", 
    "RUSURE", "RUSUREYESNO",
]

for p in PHRASES:
    all_cands.add(p)
    all_cands.add(p.lower())

# Test
print(f"Testing {len(all_cands):,} UI text candidates...")
hits = []
new_hits = []
for c in all_cands:
    h = asset_id(c)
    if h in ids:
        info = ids_dict[h]
        is_known = h in KNOWN
        marker = "(known)" if is_known else "*** NEW ***"
        print(f"  {marker:12s}  0x{h:08x}  {c:30s}  popc={info['popcount']}  kinds={info['kinds']}")
        hits.append((h, c, is_known))
        if not is_known:
            new_hits.append((h, c))

print(f"\n=== TOTAL: {len(hits)} hits ({len(new_hits)} NEW) ===")
for h, c in new_hits:
    info = ids_dict[h]
    print(f"  0x{h:08x}  {c:30s}  popc={info['popcount']}  kinds={info['kinds']}")
    print(f"          sources: {info['sources']}")

# Save
if new_hits:
    with open(f"{ROOT}/docs/analysis/asset-identification/ui_text_hits.csv", "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["id_hex","candidate","popcount","kinds","sources"])
        for h, c in new_hits:
            info = ids_dict[h]
            w.writerow([f"0x{h:08x}", c, info['popcount'], info['kinds'], info['sources']])
    print(f"\nSaved to: docs/analysis/asset-identification/ui_text_hits.csv")
