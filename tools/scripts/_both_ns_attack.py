"""Test the SAME UI text candidates against ALL uncracked sprites in BOTH namespaces.

This proves whether any of these UI/menu words match any uncracked sprite ID
under either RAW or WRAP namespace.
"""
import sys, csv
sys.path.insert(0, 'tools/scripts')
from calchash import calcHash

def rotl(v, r):
    r &= 31
    return ((v << r) | (v >> ((32 - r) & 31))) & 0xFFFFFFFF

def wrap(s):
    return 0x28C0E011 ^ rotl(calcHash(s), 27)

# Load ALL uncracked targets (both sprite and audio)
targets = {}
with open('docs/analysis/asset-identification/cracked_names.csv') as f:
    for r in csv.DictReader(f):
        if r['status'] != 'uncracked':
            continue
        targets[int(r['id_hex'], 16)] = (r['type'], r['levels'], int(r['floor']))

# Also include known anchors for sanity check
verified = {}
with open('docs/analysis/asset-identification/cracked_names.csv') as f:
    for r in csv.DictReader(f):
        if r['status'] in ('verified', 'verified-anchor'):
            verified[int(r['id_hex'], 16)] = r['name']

print(f'Loaded {len(targets)} uncracked targets, {len(verified)} verified anchors')

# Comprehensive UI text candidates  
CANDIDATES = [
    # === verified anchors (sanity check) ===
    'NO','YES','PAUSED','QUIT','CONTINUE','QUITGAME',
    
    # === ALL EXISTING FX_* known names + variants ===
    'FX_KLAY_JUMP','FX_KLAY_LAND','FX_KLAY_FART','FX_KLAY_HURT',
    'FX_KLAY_DUCK_DOWN','FX_KLAY_DIE_FALL','FX_KLAY_HIT_HEAD',
    'FX_KLAY_FOOTSTEP_QUIET','FX_KLAY_FOOTSTEP_QUIET_SOFT',
    'FX_KLAY_FOOTSTEP_LEFT_SOFT','FX_KLAY_FOOTSTEP_RIGHT_SOFT',
    'FX_KLAY_RUN_FAST_SOFT','FX_KLAY_LAND_SOFT',
    'FX_MENU_PASSWORD','FX_MENU_SELECT',
    'FX_PICKUP_PHOENIX','FX_PICKUP_GROW','FX_PICKUP_ONE_UP',
    'FX_PICKUP_1970','FX_PICKUP_1970_03',
    'FX_SKULL_FOOTSTEP_LEFT','FX_SKULL_FOOTSTEP_RIGHT','FX_SKULL_SPRING_01','FX_SKULL_SPRING_02',
    'FX_SKULL_UP','FX_SKULL_FIRE_02','FX_SKULL_FLY_01','FX_SKULL_FLY_02',
    'FX_BIRD_FLY_01','FX_BIRD_SQUISH_01',
    'FX_BOSS_HEAD_HIT','FX_BOSS_HEAD_TURN','FX_BOSS_HEAD_WALK','FX_BOSS_HEAD_IDLE_01',
    'FX_RAT_DASH_END','FX_GUM_PIERCE_DN','FX_FINN_DIE_4','FX_PUFF_FALL_3',
    
    # UI text words
    'YES','NO','OK','OKAY','CANCEL','EXIT','QUIT','PAUSE','PAUSED','BACK','GO','START','STOP',
    'BEGIN','END','DONE','RESET','RETRY','RESUME','SAVE','LOAD','MENU','MAIN','OPTIONS','SETTINGS',
    'PRESS','PUSH','SELECT','SELECTED','NEW','OLD','EMPTY','FULL','HELP','INFO',
    'CONTINUE','CONTINUES','GAMEOVER','GAME','PLAY','PLAYER','PASSWORD','UNLOCK','CODE',
    'CREDITS','THANKS','ENDING','OUTRO','INTRO','TITLE','WELCOME','HELLO','GOODBYE',
    'WIN','LOSE','LOST','WON','FAIL','SUCCESS','CLEAR','COMPLETE','FINISH',
    'TRY','AGAIN','MORE','LESS','BIG','SMALL','HUGE','TINY','GOOD','BAD','BEST','WORST',
    'WOW','OOPS','UH','HEY','HI','READY','SET','WAIT','LOADING','LOAD','SAVED','SAVING',
    'DEMO','REPLAY','MAINMENU','NEWGAME','EXITGAME','SAVEGAME','LOADGAME','PRESSSTART',
    'RECORD','RECORDS','SCORE','SCORES','HIGHSCORE','EASY','HARD','NORMAL','DIFFICULTY',
    'EXTRA','EXTRALIFE','TUTORIAL','GUIDE','HINT','TIP','WARN','WARNING','CAUTION','DANGER',
    'SKULL','MONKEY','SKULLMONKEY','SKULLMONKEYS','CLAY','CLAYBALL','CLAYMAN','KLAYMEN','KLAY',
    'WILLIE','SUPERWILLIE','PHOENIX','GLIDEY','GLIDE','GLIDER','FARTHEAD','PHARTHEAD',
    'PHART','HAMSTER','SLAPPY','SHIELD','SWIRLY','SWIRLYQ','UNIVERSE','ENEMA',
    'BURPER','BURP','JOE','JOEHEAD','GLENN','GLEN','YNTIS','WIZZ','WIZARD','KLOGG','KLOG',
    'FINN','MEGA','BOIL','SNOW','FOOD','BRG','CAVE','WEED','EGGS','CLOU','SEVN','SOAR',
    'CRYS','CSTL','RUNN','MOSS','EVIL','PHRO','SCIE','TMPL','GLID','HEAD',
    'OPTIONS','SETUP','CONFIG','AUDIO','VIDEO','SOUND','MUSIC','SFX','VOLUME',
    'CONTROLS','CONTROLLER','PAD','BUTTON','JOYPAD','CONTROL',
    'LANGUAGE','LANG','ENGLISH','FRENCH','GERMAN','SPANISH','ITALIAN','JAPANESE',
    'EN','FR','DE','SP','IT','JP','US','UK','PLAYDEMO','WATCHDEMO',
    'EXTRAS','BONUS','BONUSES','SECRET','SECRETS','UNLOCKABLE',
    'COIN','COINS','TOKEN','TOKENS','GEM','GEMS','STAR','STARS','RING','RINGS',
    'HEART','HEARTS','LIFE','LIVES','HEAL','HEALTH','HP',
    'POWER','POWERUP','POWERUPS','POWER_UP','SUPER','HYPER','ULTRA','GIANT',
    'COLLECT','COLLECTING','PICKUP','PICK','GRAB','TAKE','GET',
    'CURSOR','POINTER','HAND','ARROW','SELECTOR','HIGHLIGHTER','MARKER',
    'DOOR','DOORS','GATE','PORTAL','REVEAL','TRANSITION','WIPE','FADE','CURTAIN',
    'PANEL','FRAME','BAR','BOX','BORDER','WINDOW','DIALOG','MODAL','POPUP',
    'BG','BACKGROUND','LOGO','LOGOS','GAMETITLE','TITLELOGO',
    'FONT','FONTS','GLYPH','DIGIT','DIGITS','NUM','NUMS','NUMBER','NUMBERS',
    'CHECKPOINT','CHECKPOINTS','SAVEPOINT','SPAWN','SPAWNPOINT','RESPAWN',
    'SIGN','SIGNS','POST','BOARD','TOTEM','NOTE','SCROLL','BANNER',
    'POWERUPSIGN','CHECKPOINTSIGN','POWERUP_SIGN','CHECKPOINT_SIGN','TUTORIAL_SIGN','TUTORIALSIGN',
    'HINT_SIGN','HINTSIGN','HINT_TOTEM','HINTTOTEM','POWERUP_TOTEM','POWERUPTOTEM',
    'INSTRUCT','INSTRUCTION','INSTRUCTIONS','TUTORIAL_TEXT','TUTORIALTEXT',
]

# Test in both namespaces
hits = []
for n in CANDIDATES:
    n_upper = n.upper()
    r = calcHash(n)
    w = wrap(n)
    if r in targets:
        t, lv, fl = targets[r]
        hits.append(('RAW', n, r, t, lv, fl))
    if r in verified:
        hits.append(('RAW-anchor', n, r, 'verified', verified[r], 0))
    if w in targets:
        t, lv, fl = targets[w]
        hits.append(('WRAP', n, w, t, lv, fl))
    if w in verified:
        hits.append(('WRAP-anchor', n, w, 'verified', verified[w], 0))

print(f'\nTested {len(CANDIDATES)} candidates against {len(targets)} uncracked + {len(verified)} verified')
print(f'Total hits: {len(hits)}')
print()
for ns, n, h, t, lv, fl in sorted(hits, key=lambda x: x[2]):
    print(f'  {ns:<12s}  0x{h:08x}  {n:<25s}  type={t}  pop={fl:2d}')
