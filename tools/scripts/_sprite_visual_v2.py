"""Brute search for various visual-identified sprite names."""
import sys
sys.path.insert(0, 'tools/scripts')
from calchash import calcHash

# === Targets with visual descriptions ===
TARGETS = {
    0x80729081: 'tutorial/hint sign board (multi-frame: clay ball, swirly q, etc)',
    0x88329285: 'small/secondary checkpoint sprite (child of 0x80729081)',
    0x8ab92024: 'LOADING screen text + Klaymen',
    0x33808e1b: 'menu cursor animation frame',
    0x63848e59: 'menu cursor',
    0x4a3ea110: 'menu door opening to reveal monkey',
    0x4aee0118: 'shriney guard slam animation',
    0x4baf8200: 'shriney guard yell animation',
    0x61981a0c: 'plain monkey enemy',
    0x75189608: 'plain monkey enemy variant',
    0xc8f90114: 'glen boss / large enemy',
    0xc9387d8c: '1970 hamster icon sprite',
}

# Comprehensive word/concept dictionary
WORDS = [
    # UI/Menu primitives
    'CURSOR','POINTER','ARROW','SELECT','SELECTOR','MENU','OPTION','OPTIONS','OPT',
    'CHOICE','PICK','PIK','MARKER','HIGHLIGHT','FOCUS','SELECTION','UI',
    'HAND','POINT','POINTING','POINTINGHAND','BIGHAND','BIG_HAND','HAND_POINT',
    'HAND_RIGHT','POINTRIGHT','POINT_RIGHT','MENU_CURSOR','MENUCURSOR',
    'BUTTON_CURSOR','BTN_CURSOR','MENU_HIGHLIGHT','MENU_FOCUS','MENU_POINTER',
    'MENU_HAND','MENUHAND','HAND_MENU',
    # Loading
    'LOADING','LOAD','LOADER','LOADBAR','LOAD_BAR','LOADSCREEN','LOAD_SCREEN',
    'LOADINGSCREEN','LOADING_SCREEN','PLEASEWAIT','PLEASE_WAIT','WAIT','WAITING',
    'BUSY','LOADKLAY','LOADING_KLAY','LOAD_KLAY','LOADING_KLAYMEN','LOAD_KLAYMEN',
    'TXT_LOADING','TEXT_LOADING','LOADING_TEXT','LOADTEXT',
    'INTRO','OUTRO','TITLE','SPLASH','SPLASHSCREEN','SPLASH_SCREEN',
    'GAME_OVER','GAMEOVER','GAME','TRANSITION','TRANS','TRANS_SCREEN',
    # Door/Reveal
    'DOOR','DOORS','MENU_DOOR','MENUDOOR','DOOR_OPEN','DOOROPEN','DOOR_OPENING','DOOROPENING',
    'OPENING','OPENING_DOOR','OPEN_DOOR','SHRINE_DOOR','SHRINEDOOR','GATE','PORTAL',
    'STARTDOOR','START_DOOR','LEVEL_DOOR','LEVELDOOR',
    'REVEAL','REVEALDOOR','REVEAL_DOOR',
    # Shriney Guard
    'SHRINEY','SHRINEYGUARD','SHRINEY_GUARD','SHRINEGUARD','SHRINE_GUARD',
    'GUARD','GUARDS','GUARDIAN','GUARDIANS','BOSSGUARD','BOSS_GUARD',
    'GUARD_SLAM','GUARDSLAM','GUARD_YELL','GUARDYELL','GUARD_ATTACK','GUARDATTACK',
    'SHRINEY_SLAM','SHRINEYSLAM','SHRINEY_YELL','SHRINEYYELL','SHRINEY_HIT','SHRINEYHIT',
    'SHRINEY_ATTACK','SHRINEY_BIG','SHRINEY_IDLE','SHRINEY_HURT',
    # Monkey enemy
    'MONK','MONKS','MONKEY','MONKEYS','MONKE','SCREAMER','SCREAMER_MONKEY','SCREAMERMONKEY',
    'PLAIN_MONKEY','PLAINMONKEY','REGULAR_MONKEY','ENEMY','ENEMIES','MOOK','MOOKS',
    'MOB','MOBS','GRUNT','GRUNTS','HENCH','HENCHMAN','HENCHMEN','MINION',
    'MONKEY_WALK','MONKEYWALK','MONKEY_RUN','MONKEYRUN','MONKEY_IDLE',
    # Glenn Yntis boss
    'GLENN','GLENNYNTIS','GLENN_YNTIS','GLEN','GLENS','YNTIS','GLENN_BOSS','GLEN_BOSS',
    'GLENNBOSS','BIGGLENN','BIG_GLENN','GIANT_GLENN','SHRUNKEN','PUPPET','PUPPETBOSS',
    'GLENN_PUPPET','PUPPETER','PUPPETEER',
    # 1970 Hamster
    '1970','HAMSTER','HAMSTERS','HAM','HAMS','HAMICON','HAM_ICON','HAMSTER_ICON',
    'HAMSTERICON','SLAPPY','SLAPPYHAMSTER','SLAPPY_HAMSTER','SLAPPYHAM','HAM_SLAPPY',
    'HAMSTERSPRITE','HAMSTER_SPRITE','SLAPPY_SHIELD','SHIELD','HALO',
    'POWER_HAMSTER','POWERHAMSTER','HAMSTER_POWER','HAMSTERPOWER',
    'GUINEAPIG','GUINEA_PIG','PIG','MOUSE',
    # Generic asset name prefixes/suffixes
    'SPR','SPRT','SPRITE','SEQ','ANM','ANIM','OBJ','OBJECT','BG','FG','HUD',
]

print('Testing %d words against %d targets...' % (len(WORDS), len(TARGETS)))
print()
for n in WORDS:
    h = calcHash(n)
    if h in TARGETS:
        print('  HIT: %s -> 0x%08x  (%s)' % (n, h, TARGETS[h]))
print('done')
