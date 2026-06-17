"""Broad WRAP-namespace attack on ALL uncracked sprite IDs.

The WRAP namespace formula:  id = 0x28C0E011 ^ rotl(calcHash(name), 27)
Confirmed used by the 6 menu-text anchors: NO/YES/PAUSED/QUIT/CONTINUE/QUITGAME.

Strategy: build a large UI/menu-text vocabulary (NOT generic English/grammar words),
test every candidate's WRAP hash against the full uncracked sprite ID set, and
report any single-candidate hits as high-confidence WRAP-namespace finds.
"""
import sys, csv, itertools
sys.path.insert(0, 'tools/scripts')
from calchash import calcHash

def rotl(v, r):
    r &= 31
    return ((v << r) | (v >> ((32 - r) & 31))) & 0xFFFFFFFF

SEED = 0x28C0E011

def wrap(s):
    return SEED ^ rotl(calcHash(s), 27)

# Load all uncracked sprite IDs
targets = {}
with open('docs/analysis/asset-identification/cracked_names.csv') as f:
    for r in csv.DictReader(f):
        if r['status'] != 'uncracked':
            continue
        if r['type'] != 'sprite':
            continue
        targets[int(r['id_hex'], 16)] = (r['levels'], int(r['floor']))

print(f'Loaded {len(targets)} uncracked sprite targets')

# === Build a broad UI/menu/display-text vocabulary ===
# Categories matching things displayed as on-screen text in a 90s console game

# Single words (the 6 anchors were 1 word, 2-8 chars)
DISPLAY_TEXT_WORDS = [
    # Basic UI
    'YES','NO','OK','OKAY','CANCEL','EXIT','QUIT','PAUSE','PAUSED','BACK',
    'GO','START','STOP','BEGIN','END','DONE','RESET','RETRY','RESUME',
    'SAVE','LOAD','MENU','MAIN','OPTIONS','SETTINGS','CONFIG','HELP','INFO',
    'SELECT','SELECTED','SELECTING','PRESS','PRESSED','PUSH','PUSHED','HIT',
    'NEW','OLD','EMPTY','FULL','FILES','FILE','SLOT','SLOTS','ITEM','ITEMS',
    'LIVES','LIFE','SCORE','HISCORE','BEST','TOP','TIME','TIMER','BONUS',
    'LEVEL','STAGE','ZONE','ROOM','AREA','MAP','WORLD','PLANET',
    'CONTINUE','CONTINUES','GAMEOVER','GAME','PLAY','PLAYER','PLAYING','PAUSE','PASS',
    'PASSWORD','PASS','UNLOCK','LOCK','LOCKED','UNLOCKED','CODE','CODES',
    'CREDITS','CREDIT','THANKS','THE','END','ENDING','OUTRO','INTRO','TITLE',
    'WELCOME','HELLO','GOODBYE','BYE','BACK','RETURN',
    'WIN','LOSE','LOST','WON','WINNER','LOSER','FAIL','FAILED','SUCCESS',
    'CLEAR','CLEARED','COMPLETE','COMPLETED','FINISH','FINISHED',
    'TRY','TRYING','TRIED','AGAIN','MORE','LESS','BIG','SMALL','HUGE','TINY',
    'GOOD','BAD','BEST','WORST','GREAT','NICE','COOL','OK','OOPS','UH',
    'WOW','YAY','YIKES','OUCH','HEY','HI','HELLO',
    'READY','SET','GO','WAIT','WAITING','LOADING','LOADED','LOAD',
    'SAVED','SAVING','PLEASE','THANK','SORRY','HEAL','HEALED',
    'DEMO','DEMOS','DEMOMODE','PLAYDEMO','REPLAY','REPLAYING','REPLAYED',
    'PRESSSTART','STARTGAME','MAINMENU','NEWGAME','EXITGAME','QUITGAME','SAVEGAME','LOADGAME',
    'NEW','OLD','RECORD','RECORDS','SCORE','SCORES','HIGHSCORE',
    'EASY','HARD','MEDIUM','NORMAL','HARDER','EASIER','DIFFICULTY',
    'SHORT','LONG','MORE','LESS','EXTRA','EXTRAS','EXTRA_LIFE','EXTRALIFE',
    'YEAH','NOPE','MAYBE','MAYBE_NOT','PROBABLY','SURE','UNSURE',
    'TUTORIAL','TUTORIALS','GUIDE','GUIDES','HINT','HINTS','TIP','TIPS',
    'WARN','WARNING','CAUTION','DANGER','BEWARE','NOTICE','ALERT',
    # Skullmonkeys specifics
    'SKULL','SKULLS','MONKEY','MONKEYS','SKULLMONKEY','SKULLMONKEYS','SKM',
    'CLAY','CLAYBALL','CLAYBALLS','CLAYMAN','CLAYMEN','KLAYMEN','KLAY',
    'WILLIE','SUPERWILLIE','PHOENIX','GLIDEY','GLIDE','GLIDER',
    'FARTHEAD','PHARTHEAD','PHART','PHARTH','HAMSTER','HAMSTERS','SLAPPY','SHIELD','SHIELDS',
    'SWIRLY','SWIRLYQ','UNIVERSE','ENEMA','NUKE','NUKES',
    'BURPER','BURP','BURPS','BURPING',
    'JOE','JOEHEAD','JOEHEADJOE','GLENN','GLEN','YNTIS','WIZZ','WIZARD','KLOGG','KLOG',
    'FINN','MEGA','BOIL','SNOW','FOOD','BRG','BRG1','CAVE','WEED','EGGS','CLOU','SEVN','SOAR',
    'CRYS','CSTL','RUNN','MOSS','EVIL','PHRO','SCIE','TMPL','GLID','HEAD',
    'TITLE','TITLESCREEN','LOGO','LOGOS','GAMETITLE',
    'OPTIONS','SETUP','CONFIG','SETTING','AUDIO','VIDEO','SOUND','MUSIC','SFX','VOLUME','VOL',
    'CONTROLS','CONTROLLER','PAD','BUTTON','BUTTONS','JOYPAD','CONTROL',
    'LANGUAGE','LANG','LANGS','ENGLISH','FRENCH','GERMAN','SPANISH','ITALIAN','JAPANESE',
    'EN','FR','DE','SP','IT','JP','US','UK',
    'MENUSELECT','MENU_SELECT','PRESS_START','PRESSSTART','PUSH_START','PUSHSTART',
    'PLAY_DEMO','PLAYDEMO','WATCH_DEMO','WATCHDEMO','VIEW_DEMO','VIEWDEMO',
    'EXTRAS','EXTRA','BONUS','BONUSES','SECRET','SECRETS','UNLOCKABLE','UNLOCKABLES',
    # collectible / pickup names
    'COIN','COINS','TOKEN','TOKENS','GEM','GEMS','STAR','STARS','RING','RINGS',
    'HEART','HEARTS','LIFE','LIVES','HEAL','HEALTH','HP',
    'POWER','POWERUP','POWERUPS','POWER_UP',
    'BIG','SUPER','MEGA','HYPER','ULTRA','GIANT','HUGE',
    # tutorial sign content
    'COLLECT','COLLECTING','COLLECTED','PICKUP','PICK','GRAB','TAKE','GET','HAVE',
    'ONEHUNDRED','HUNDRED','THIRTY','TWENTY','TEN','THREE','TWO','ONE','FOUR','FIVE',
    'C100','C50','C25','C10','C5','C3','C2','C1',
    'FREE','FREELIFE','FREEPLAY','GIFT','REWARD','PRIZE','PRIZES',
    'BONUSLEVEL','BONUSSTAGE','BONUSAREA','SECRETLEVEL','SECRETSTAGE',
    'NUKEALL','NUKEALL','KILLALL','KILL','DIE','DEAD','DEATH',
]

# Two-word combinations (CONTINUE+GAME=QUITGAME pattern)
TWO_WORD_BASES = [
    # base1 + base2 combinations seen in anchors:
    # CONTINUE has CONTINUE alone (verified)
    # QUITGAME = QUIT+GAME (verified)
    ('QUIT','GAME'),('NEW','GAME'),('SAVE','GAME'),('LOAD','GAME'),
    ('END','GAME'),('START','GAME'),('STOP','GAME'),('RESET','GAME'),
    ('GAME','OVER'),('GAME','PAUSE'),('GAME','MENU'),('GAME','MODE'),
    ('PRESS','START'),('PRESS','BUTTON'),('PRESS','A'),('PRESS','X'),('PRESS','SELECT'),
    ('PUSH','START'),('PUSH','BUTTON'),
    ('MAIN','MENU'),('PAUSE','MENU'),('OPTIONS','MENU'),('TITLE','MENU'),
    ('PLAY','DEMO'),('WATCH','DEMO'),('VIEW','DEMO'),('STOP','DEMO'),
    ('HIGH','SCORE'),('TOP','SCORE'),('BEST','SCORE'),('YOUR','SCORE'),
    ('GAME','OVER'),('YOU','WIN'),('YOU','LOSE'),('YOU','DIED'),('YOU','LOST'),
    ('LEVEL','CLEAR'),('STAGE','CLEAR'),('LEVEL','COMPLETE'),('STAGE','COMPLETE'),
    ('AREA','CLEAR'),('ZONE','CLEAR'),
    ('CLEAR','BONUS'),('TIME','BONUS'),('LIFE','BONUS'),('STAGE','BONUS'),
    ('NEXT','LEVEL'),('NEXT','STAGE'),('NEXT','AREA'),('NEXT','ZONE'),
    ('PASSWORD','ENTRY'),('PASSWORD','INPUT'),('PASSWORD','ENTER'),
    ('PASSWORD','WRONG'),('PASSWORD','OK'),('PASSWORD','GOOD'),
    ('WRONG','PASSWORD'),('BAD','PASSWORD'),('GOOD','PASSWORD'),
    ('FILE','SELECT'),('FILE','LOAD'),('FILE','SAVE'),('FILE','EMPTY'),
    ('SLOT','EMPTY'),('SLOT','SELECT'),('NEW','FILE'),('OLD','FILE'),
    ('GAME','OVER'),('TRY','AGAIN'),('PRESS','RETRY'),('LOAD','RETRY'),
    ('LIFE','LOST'),('LIVES','LEFT'),('EXTRA','LIFE'),('FREE','LIFE'),
    ('PRESS','R1'),('PRESS','R2'),('PRESS','L1'),('PRESS','L2'),
    ('PRESS','CIRCLE'),('PRESS','TRIANGLE'),('PRESS','SQUARE'),('PRESS','X'),
    ('NEO','PETS'),('SUPER','WILLIE'),('CLAY','BALL'),
    ('GLAYBALL','TEXT'),('CLAYBALL','TEXT'),
    ('HAMSTER','SHIELD'),('PHOENIX','HAND'),('SUPER','WILLIE'),('FART','HEAD'),
    ('UNIVERSE','ENEMA'),('SLAPPY','HAMSTER'),('SWIRLY','Q'),
    ('CLAY','BALL'),('CLAY','BALLS'),
]

# Build all candidates
candidates = set()
for w in DISPLAY_TEXT_WORDS:
    candidates.add(w.upper())
for a, b in TWO_WORD_BASES:
    candidates.add((a + b).upper())
    candidates.add((b + a).upper())  # also reverse

print(f'Testing {len(candidates)} candidate names...')

# Test against ALL uncracked sprite targets
hits = []
for n in candidates:
    w = wrap(n)
    if w in targets:
        lv, fl = targets[w]
        hits.append((n, w, lv, fl))

if hits:
    print(f'\nFound {len(hits)} WRAP hits:')
    for n, w, lv, fl in sorted(hits, key=lambda x: x[1]):
        lv_short = lv if len(lv) < 40 else lv[:40] + '...'
        print(f'  0x{w:08x}  pop={fl:2d}  {n:<25s}  levels={lv_short}')
else:
    print('\nNo WRAP matches found in uncracked sprite targets.')
