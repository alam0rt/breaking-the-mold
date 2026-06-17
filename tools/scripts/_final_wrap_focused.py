"""Final focused WRAP attack on sprites with visible English text content.

Specifically targets:
  0x81100030 — Controller mapping screen ("JUMP / SHOOT / FAST RUN / NOT USED")
  0x28c080df — "DEMO" text stamp
  0x8ab92024 — "LOADING" screen text
  0x80729081 — Tutorial sign (CLAY BALL etc.)
"""
import sys, csv
sys.path.insert(0, 'tools/scripts')
from calchash import calcHash

def rotl(v, r):
    r &= 31
    return ((v << r) | (v >> ((32 - r) & 31))) & 0xFFFFFFFF

def wrap(s):
    return 0x28C0E011 ^ rotl(calcHash(s), 27)

TARGETS = {
    0x81100030: 'Controller mapping screen ("3 / NOT USED / JUMP / SHOOT / FAST RUN")',
    0x28c080df: '"DEMO" text stamp',
    0x8ab92024: '"LOADING" screen text + Klaymen',
    0x80729081: 'Tutorial sign (CLAY BALL/SWIRLY Q/UNIVERSE ENEMA/...)',
    0x00e2f188: 'Number glyph 0-9',
}

# Controller mapping vocabulary
CONTROLLER_WORDS = [
    'CONTROLS','CONTROL','CONTROLLER','CONTROLLERS','CTL','CTRL',
    'BUTTON','BUTTONS','BTN','BTNS','KEYS','KEY','KEYMAP','KEYMAPPING',
    'MAPPING','MAP','MAPS','BINDING','BINDINGS','BIND',
    'DEFAULT','DEFAULTS','SETUP','CONFIG','CONFIGS','CONFIGURATION',
    'JUMP','SHOOT','RUN','ATTACK','FIRE','FAST','SLOW','FASTRUN','FAST_RUN','RUNFAST',
    'NOT_USED','NOTUSED','UNUSED','NONE','NULL','EMPTY','BLANK','NA',
    'CONTROLSCREEN','CONTROL_SCREEN','CONTROLSSCREEN','CONTROLS_SCREEN',
    'BUTTONMAP','BUTTON_MAP','BUTTONMAPPING','BUTTON_MAPPING',
    'GAMEPAD','PAD','PADS','JOYPAD','JOY','JOYSTICK','STICK','DPAD','DPAD_UP',
    'TRIGGER','TRIGGERS','SHOULDER','L1','L2','R1','R2','LR1','LR2',
    'CIRCLE','TRIANGLE','SQUARE','CROSS','START','SELECT','DPAD',
    'PRESS','PRESSED','PUSH','PUSHED','HOLD','HELD','RELEASE','RELEASED',
    'CTRLMAP','CTRL_MAP','PADMAP','PAD_MAP',
    # Action-related menu text
    'ACTION','ACTIONS','MOVES','MOVE','DODGE','BLOCK','GUARD','PARRY',
    'POWER','SPECIAL','SUPER','HYPER','CHARGE','BURST','SLAM',
    # Common menu words from research
    'OPTIONS','OPTION','OPTS','OPT','MENU','MENUS','SETTINGS','SETTING','SETUP',
    'AUDIO','VIDEO','SOUND','SOUNDS','MUSIC','SFX','FX','VOL','VOLUME',
    'LANGUAGE','LANG','LANGUAGES','REGION','REGIONS',
    'BRIGHTNESS','CONTRAST','COLOR','COLORS','GAMMA','TINT',
    'INVERT','SENS','SENSITIVITY','MOUSE','CURSOR',
]

# Load entity action verbs from decompiled source (game vocabulary)
ENGINE_VOCAB = [
    'IDLE','STAND','STANDING','WALK','WALKING','RUN','RUNNING','JUMP','JUMPING',
    'FALL','FALLING','LAND','LANDING','DIE','DEATH','DEAD','HURT','HIT','BLINK',
    'SHOOT','FIRE','FIRING','ATTACK','SLAM','BOUNCE','OPEN','CLOSE','TURN','EAT',
    'SLEEP','ROLL','SLIDE','FLY','FLYING','GLIDE','SPIN','SPIT','SCREAM','TAUNT',
    'VICTORY','DANCE','LEAP','CYCLE','SWING','CHARGE','DODGE','BLOCK','CLIMB','SWIM',
    'DUCK','CRAWL','REST','WAIT','SPAWN','GROW','SHRINK','BASE','MAIN','FULL','NORMAL',
    'INITIAL','START','MIDDLE','FINAL','LAST','FIRST',
    'ENABLE','ENABLED','DISABLE','DISABLED','ACTIVE','INACTIVE','ON','OFF',
    'TRUE','FALSE','GOOD','BAD','BEST','WORST',
    'SHIELD','BOOST','POWER','SUPER','MEGA','HYPER','ULTRA','MAX','MIN',
    'ARMOR','HEALTH','ENERGY','MANA','STAMINA','SPECIAL','SKILL',
]

# Specific from game manual/title screen analysis
SKULLMONKEYS_MENU = [
    'SKULLMONKEYS','SKULL_MONKEYS','SKULLMONKEY','SKULL_MONKEY',
    'SKULLMENU','SKULL_MENU','MONKEYMENU','MONKEY_MENU',
    'NEWGAME','NEW_GAME','LOADGAME','LOAD_GAME','SAVEGAME','SAVE_GAME','OPTIONSMENU','OPTIONS_MENU',
    'MAIN_MENU','MAINMENU','GAMEMENU','GAME_MENU','TITLEMENU','TITLE_MENU',
    'PASSWORD','PASSWORDS','PWD','PASS','CODE','CODES','PASSCODE','PASS_CODE',
    'CREDITS','CREDIT','CONGRATS','CONGRATULATIONS','THANKS','THANK_YOU','THANKYOU',
    'INTRO','INTRO_MOVIE','INTROMOVIE','OUTRO','OUTRO_MOVIE','OUTROMOVIE',
    'TITLE','TITLE_SCREEN','TITLESCREEN','LOGO','LOGOS','GAMETITLE','GAME_TITLE',
]

# Targets the tutorial sign content
TUTORIAL_CONTENT = [
    'CLAY','CLAYBALL','CLAYBALLS','SWIRLY','SWIRLEY','SWIRLYQ','SWIRLY_Q',
    'UNIVERSE','UNIVERSEENEMA','UNIVERSE_ENEMA','ENEMA',
    'SUPER','SUPERWILLIE','SUPER_WILLIE','WILLIE',
    'SLAPPY','SLAPPYHAMSTER','SLAPPY_HAMSTER','HAMSTER',
    'PHOENIX','PHOENIXHAND','PHOENIX_HAND','PHART','PHARTHEAD','PHART_HEAD','FARTHEAD','FART_HEAD',
    'GLIDEY','GLIDE','GLIDER',
    'TUTORIAL','TUTORIALSIGN','TUTORIAL_SIGN','HINTBOARD','HINT_BOARD',
    'POWERUPSIGN','POWERUP_SIGN','SIGN','SIGNS','POST','TOTEM','BOARD','POPUP',
    'INSTRUCT','INSTRUCTION','INSTRUCTIONS','HINTSIGN','HINT_SIGN','HINT',
    'COLLECTSIGN','COLLECT_SIGN','PICKUPSIGN','PICKUP_SIGN',
    'CHECKPOINT','CHECKPOINTSIGN','CHECKPOINT_SIGN',
    'INTRO','INTROSIGN','INTRO_SIGN','LOAD','LOADSIGN','LOAD_SIGN',
    'INFO','INFOBOARD','INFO_BOARD','INFOSIGN','INFO_SIGN','INFOPANEL','INFO_PANEL',
    'WHATIS','WHAT_IS','THISIS','THIS_IS','THIS','THAT','HEY','LOOK','SEE',
    'EXAMINE','EXPLAIN','LEARN','LEARNTHIS','LEARN_THIS',
    'TUT','TUTS','TUT01','TUT_01','TUT1','TUTORIAL01','TUTORIAL_01','TUTORIAL1',
    'SCROLL','SCROLLS','PARCHMENT','PAPER','PAPERS','NOTES','NOTE',
    'BANNER','BANNERS','PLAQUE','PLAQUES','PLATE','PLATES',
    'NEWMOVE','NEW_MOVE','MOVEINFO','MOVE_INFO','NEWITEM','NEW_ITEM','ITEMINFO','ITEM_INFO',
    'GAMETIP','GAME_TIP','GAMETIPS','GAME_TIPS','PROTIP','PRO_TIP','PROTIPS','PRO_TIPS',
]

LOADING_WORDS = [
    'LOADING','LOAD','LOADER','LOADTEXT','LOAD_TEXT','LOADINGTEXT','LOADING_TEXT',
    'LOADSCREEN','LOAD_SCREEN','LOADINGSCREEN','LOADING_SCREEN',
    'PLEASE','PLEASEWAIT','PLEASE_WAIT','WAIT','WAITING','BUSY','BUSY_SCREEN',
    'NEXT','NEXTLEVEL','NEXT_LEVEL','NEXTSTAGE','NEXT_STAGE','NEXTAREA','NEXT_AREA','NEXTROOM','NEXT_ROOM',
    'SAVING','SAVE','SAVESCREEN','SAVE_SCREEN','SAVED','SAVE_TEXT','SAVETEXT',
    'KLAY','KLAYMEN','KLAYLOADING','KLAY_LOADING','KLAYMENLOADING','KLAYMEN_LOADING',
    'LOADKLAY','LOAD_KLAY','LOADKLAYMEN','LOAD_KLAYMEN',
    'LOADANIM','LOAD_ANIM','LOADINGANIM','LOADING_ANIM',
]

DEMO_WORDS = [
    'DEMO','DEMOS','DEMOMODE','DEMO_MODE','DEMOPLAYBACK','DEMO_PLAYBACK',
    'PLAYDEMO','PLAY_DEMO','RECORDDEMO','RECORD_DEMO','REPLAYDEMO','REPLAY_DEMO',
    'DEMOTEXT','DEMO_TEXT','DEMOSTAMP','DEMO_STAMP','DEMOBADGE','DEMO_BADGE',
    'WATERMARK','WATER_MARK','STAMP','STAMPS','BADGE','BADGES','OVERLAY','LABEL','LABELS',
    'PRESS','PRESSSTART','PRESS_START','STARTGAME','START_GAME','BEGIN',
    'WAIT','IDLE','ATTRACT','ATTRACTMODE','ATTRACT_MODE','TITLE_ATTRACT','TITLEATTRACT',
    'PROMOTIONAL','PROMO','SHOWCASE','PREVIEW','SAMPLE','SAMPLER','TRIAL',
]

# Build combined candidate set
all_candidates = set()
for lst in [CONTROLLER_WORDS, ENGINE_VOCAB, SKULLMONKEYS_MENU, TUTORIAL_CONTENT, LOADING_WORDS, DEMO_WORDS]:
    for w in lst:
        all_candidates.add(w.upper())

# Also try with common prefixes/suffixes
extended = set(all_candidates)
for w in list(all_candidates):
    # Suffix variants
    for suf in ['_TEXT','_SCREEN','_LABEL','_BANNER','_SIGN','_MENU','_UI','_BG','_HUD','_ANIM','_FRAME','_BOX']:
        extended.add(w + suf)
        extended.add(w + suf.replace('_',''))
    # Prefix variants
    for pre in ['UI_','MENU_','HUD_','BG_','TEXT_','LABEL_','SIGN_','BANNER_','TITLE_']:
        extended.add(pre + w)
        extended.add(pre.replace('_','') + w)

print(f'Testing {len(extended)} unique candidates against {len(TARGETS)} targets (WRAP + RAW)...')
print()

hits = []
for n in extended:
    w = wrap(n)
    r = calcHash(n)
    if w in TARGETS:
        hits.append(('WRAP', n, w, TARGETS[w]))
    if r in TARGETS:
        hits.append(('RAW', n, r, TARGETS[r]))

if hits:
    for ns, n, h, tag in hits:
        print(f'  HIT ({ns}): {n:<35s} -> 0x{h:08x}  ({tag})')
    print(f'\nFound {len(hits)} hits.')
else:
    print('  No matches found.')

# Also try against ALL uncracked sprites to find collateral hits
print()
print('Scanning candidates against all uncracked sprites (looking for any WRAP hits)...')
import csv as csv_mod
unknowns = {}
with open('docs/analysis/asset-identification/cracked_names.csv') as f:
    for r in csv_mod.DictReader(f):
        if r['status'] == 'uncracked' and r['type'] == 'sprite':
            unknowns[int(r['id_hex'], 16)] = r['levels']

wrap_hits = []
for n in extended:
    w = wrap(n)
    if w in unknowns:
        wrap_hits.append((n, w, unknowns[w]))

if wrap_hits:
    for n, h, lv in wrap_hits[:30]:
        lv_short = lv[:50] + ('...' if len(lv) > 50 else '')
        print(f'  WRAP HIT: 0x{h:08x}  {n:<25s}  levels={lv_short}')
    print(f'\nFound {len(wrap_hits)} WRAP hits in uncracked sprite set.')
else:
    print('  No WRAP hits in uncracked sprite set.')
