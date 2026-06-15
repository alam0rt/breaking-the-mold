"""Try literal game-text strings against the target ID set."""
import sys
sys.path.insert(0, 'tools/scripts')
from compound_hash_attack import calc_hash_and_shift, rotl  # type: ignore

SEED = 0x28C0E011
OUT_ROT = 27


def asset_id(name: str) -> int:
    h, _ = calc_hash_and_shift(name)
    return SEED ^ rotl(h, OUT_ROT) & 0xFFFFFFFF


import re
def load_targets():
    targets = set()
    with open("/tmp/asset_targets.txt") as f:
        for line in f:
            s = line.strip()
            if not s or s.startswith("#"):
                continue
            if s.startswith("0x") or s.startswith("0X"):
                targets.add(int(s, 16))
            else:
                targets.add(int(s, 10))
    return targets


# Common game-UI text strings
CANDIDATES = []

singles = [
    # menu / score
    "SCORE", "BONUS", "EXTRA", "EXTRALIFE", "LIFE", "LIVES",
    "STAGE", "STAGECLEAR", "CLEAR", "GAMEOVER", "GAME OVER", "GAME",
    "OVER", "PRESS", "PRESSSTART", "START", "PRESSSTARTBUTTON",
    "PASSWORD", "OPTIONS", "OPTION", "MUSIC", "SOUND", "MUSICTEST",
    "SOUNDTEST", "CREDITS", "CREDIT", "SETTING", "SETTINGS", "EXIT",
    "BACK", "RETURN", "CONFIRM", "CANCEL", "CONFIRMATION",
    "PLAYER1", "PLAYER2", "PLAYERONE", "PLAYERTWO", "1P", "2P", "P1", "P2",
    "1UP", "2UP", "PERFECT", "EXCELLENT", "GOOD", "BAD", "OK", "OKAY",
    "RETRY", "TRYAGAIN", "AGAIN", "QUIT", "ABORT", "BACKUP", "SAVE",
    "LOAD", "FILE", "MEMORYCARD", "CARD",
    # Skullmonkeys-specific
    "SKULLMONKEYS", "MONKEY", "MONKEYS", "SKULL", "SKULLS", "KLAYMEN",
    "KLAY", "KLOG", "KLOGG", "JOE", "JOEHEAD", "JOEHEADJOE", "MEGA",
    "EVIL", "NEVERHOOD", "BIRD", "BIRDHEAD", "GUARD", "SHRINEY",
    "SHRINEYGUARD", "WIZZ", "GLEN", "GLENN", "GLIDER", "SLIDER",
    # technical
    "MENU", "MAIN", "MAINMENU", "TITLE", "INTRO", "OUTRO", "LOGO",
    "DEMO", "PROLOGUE", "EPILOGUE", "ENDING", "ENDING1", "ENDING2",
    "CONTINUE", "CONTINUEYESNO", "PAUSE", "PAUSED", "PAUSEMENU",
    # game elements
    "CHECKPOINT", "BONUSSTAGE", "BONUSLEVEL", "BONUSROOM",
    "TUTORIAL", "INTROSCENE", "OPENING", "CLOSING", "TRANSITION",
    "STAGEINTRO", "LEVELINTRO", "AREA", "AREAINTRO",
    "BOSS", "BOSSFIGHT", "BOSSINTRO",
    # specific words from manual
    "HOCK", "HOCKING", "LOOGEY", "PLOOGER", "PROOGER", "GOOGER",
    "PROOG", "POSEY", "PHIRO", "PHIROW", "PHIROW8", "TEMPLE", "RUINS",
    # numbers as words
    "ZERO", "ONE", "TWO", "THREE", "FOUR", "FIVE", "SIX", "SEVEN",
    "EIGHT", "NINE", "TEN",
    # generic font/dialog
    "FONT", "FONT0", "FONT1", "FONT2", "TEXT", "DIALOG", "DIALOGUE",
    "MESSAGE", "MSG", "ALPHABET", "ALPHA", "GLYPH", "GLYPHS", "CHARS",
    "CHARSET", "CHARACTERS", "DIGIT", "DIGITS", "NUM", "NUMS",
    "NUMBER", "NUMBERS", "BG", "BACKGROUND", "FOREGROUND",
    "ICON", "ICONS", "CURSOR", "CURSORS", "POINTER", "HAND", "ARROW",
    # generic actions
    "WALK", "RUN", "JUMP", "FALL", "DIE", "DEATH", "STAND", "IDLE",
    "ATTACK", "ATK", "HURT", "HIT", "SHOOT", "FIRE", "SPIT", "BOUNCE",
    # specific objects
    "CLAYBALL", "CLAY", "BALL", "RAINBOW", "STAR", "STARS", "POINT",
    "SAVE", "POWERUP", "POWER", "ITEM", "PICKUP", "BONUS", "TREASURE",
    "RUBY", "GEM", "GEMS", "DIAMOND", "DIAMONDS", "COIN", "COINS",
    "KEY", "KEYS", "EXIT", "EXITSIGN", "DOORWAY", "DOOR", "ENTRANCE",
    # versions
    "VERSION", "VERSIONNTSC", "VERSIONPAL", "VERSION1", "VERSION2",
]

for s in singles:
    CANDIDATES.append(s.upper())

# Multi-word
multi = [
    "SKULL MONKEY", "SKULL MONKEYS", "SKULL MONKEY HUNT", "MONKEY HEAD",
    "JOE HEAD JOE", "JOE HEAD", "HEAD JOE", "KLAYMEN GO",
    "WORLD WARP", "GAME OVER", "STAGE CLEAR", "PRESS START",
    "PRESS START BUTTON", "OPTIONS MENU", "MAIN MENU", "SOUND TEST",
    "MUSIC TEST", "CONTINUE GAME", "QUIT GAME", "PAUSED GAME",
    "PAUSE MENU", "PAUSED MENU", "SHRINEY GUARD", "MENU CURSOR",
    "SCREAMER MONKEY", "BIRD HEAD", "JOE BIRDHEAD",
    "MONKEY MAGE", "MONKEY MAGE BOSS", "GLENN YNTIS",
    "BONUS POINTS", "BONUS LIFE", "EXTRA LIFE", "FINAL SCORE",
    "FINAL STAGE", "FINAL BOSS", "WORLD MAP", "LEVEL SELECT",
    "ITEM BAG", "POWER UP", "POWER METER", "HEALTH METER",
]

for s in multi:
    CANDIDATES.append(s.upper().replace(" ", ""))
    CANDIDATES.append(s.upper().replace(" ", "_"))


targets = load_targets()
hits = []
for c in CANDIDATES:
    a = asset_id(c)
    if a in targets:
        hits.append((a, c))

print(f"tested {len(CANDIDATES)} candidates; {len(hits)} hits in target set")
for a, c in sorted(hits):
    print(f"  0x{a:08x}  {c!r}")
