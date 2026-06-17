"""WRAP attack on text-rendering sprites (instructional/menu text/etc.)

These targets are sprites that DISPLAY TEXT on screen, so they may use the same
WRAP namespace as NO/YES/PAUSED/QUIT/CONTINUE/QUITGAME anchors.
"""
import sys
sys.path.insert(0, 'tools/scripts')
from calchash import calcHash

def rotl(v, r):
    r &= 31
    return ((v << r) | (v >> ((32 - r) & 31))) & 0xFFFFFFFF

def wrap(s):
    return 0x28C0E011 ^ rotl(calcHash(s), 27)

# Targets that DISPLAY TEXT - candidates for WRAP namespace
TARGETS = {
    0x80729081: 'Tutorial sign multi-frame: CLAY BALL/SWIRLY Q/UNIVERSE ENEMA/SUPER WILLIE/SLAPPY HAMSTER',
    0x88329285: 'Shadow of tutorial sign (paired with 0x80729081)',
    0x28c080df: 'DEMO text stamp',
    0x8ab92024: 'LOADING screen text + Klaymen',
    0x00e2f188: 'Number glyph 0-9 (all levels)',
    # All in cracked_names with MENU presence and many frames
    0x33808e1b: 'Menu cursor anim frame (MENU, 36 frames)',
    0x63848e59: 'Menu cursor (MENU)',
    0x4a3ea110: 'Menu door reveal (MENU)',
    0x0305a4f5: 'MENU sprite, 42 frames',
    0x10094096: 'MENU sprite',
    0x3099991b: 'MENU sprite',
    0x30a0c119: 'MENU sprite (76 frames)',
    0x38a0c119: 'MENU sprite (96 frames)',
    0x39900619: 'MENU sprite',
    0xe289c059: 'MENU sprite',
    0xec95689b: 'MENU sprite',
    0x40b18011: 'MENU stage4 sprite',
    0x40b28011: 'MENU stage4 sprite',
    0x40b48011: 'MENU stage4 sprite',
    # Other text-bearing
    0x3080820d: 'InitMenuStage1 sprite (token:2)',
    0x3080840d: 'InitMenuStage1 sprite',
    0x30808e0d: 'InitMenuStage1 sprite',
}

# === Candidate name dictionary - tutorial / hint / collectible info text ===
CANDIDATES = []

# Anchor-style: just display words (like NO, YES)
DISPLAY_WORDS = [
    'OK','CANCEL','SAVE','LOAD','EXIT','BACK','MENU','HELP','INFO','START','BEGIN',
    'PLAY','TITLE','OPTIONS','PASSWORD','CREDITS','THANKS','ENDING','GAMEOVER',
    'DEMO','DEMOMODE','PRESS','PRESSSTART','PUSHSTART','HEY','HI','WAIT',
    'LOADING','LOAD','LOADTEXT','PLEASEWAIT','READY','SET','GO',
    'WELL','DONE','WELLDONE','BONUS','BIGBONUS','CHEAT','UNLOCK','LOCKED','PAUSED',
    'GAME','GAMES','NEW','NEWGAME','CONTINUE','SAVED','SAVING','LOADED',
    'YEAH','NAH','NOPE','SURE','MAYBE','RETRY','RESET','QUITGAME',
]
CANDIDATES.extend(DISPLAY_WORDS)

# Tutorial-sign vocabulary (matches the sign content)
TUTORIAL_WORDS = [
    'CLAYBALL','CLAYBALLS','CLAY','SWIRLY','SWIRLYQ','SWIRLY_Q','SWIRLEY','SWIRLEYQ',
    'UNIVERSE','UNIVERSEENEMA','ENEMA','UNIVERSE_ENEMA',
    'SUPER','SUPERWILLIE','WILLIE','SUPER_WILLIE',
    'SLAPPY','SLAPPYHAMSTER','HAMSTER','SLAPPY_HAMSTER',
    'PHOENIX','PHOENIXHAND','PHOENIX_HAND','FARTHEAD','PHARTHEAD','PHART','GLIDEY','GLIDE',
    'COLLECT','COLLECT100','PICKUP','TAKE','GRAB','GET','GETIT',
    'BONUSLEVEL','BONUS_LEVEL','BONUS_LVL','FREELIFE','FREE_LIFE','EXTRALIFE','EXTRA_LIFE',
    'NUKE','PRESS','PRESS_R1','PRESS_R2','PRESSR1','PRESSR2','PRESSSELECT',
    'POWERUP','POWER_UP','POWER','SHIELD','PROTECT','PROTECTS','SHIELDS',
    'HINT','HINTS','HELP','HELPS','TUTORIAL','TUTORIALS','GUIDE','GUIDES','INSTRUCT','INSTRUCTIONS',
    'INSTRUCTION','SIGN','SIGNS','BOARD','POST','SIGNPOST','TOTEM','NOTE','NOTES','SCROLL','BANNER',
    'INFO_BOARD','INFOBOARD','INFO_SIGN','INFOSIGN','HINT_BOARD','HINTBOARD','HINT_SIGN','HINTSIGN',
    'TIP','TIPS','MESSAGE','MSG','MESSAGES',
    'WARN','WARNING','CAUTION','BEWARE',
]
CANDIDATES.extend(TUTORIAL_WORDS)

# Cursor/Pointer/UI elements
UI_ELEMENTS = [
    'CURSOR','POINTER','HAND','ARROW','SELECTOR','HIGHLIGHTER','MARKER','PICK','SELECT',
    'CURSOR_ANIM','CURSORANIM','POINTER_ANIM','POINTERANIM','ARROW_ANIM','ARROWANIM',
    'MENU_CURSOR','MENUCURSOR','MENUARROW','MENU_ARROW','MENUPOINTER','MENU_POINTER',
    'MENU_HAND','MENUHAND','MENU_PICK','MENUPICK','MENU_SELECT','MENUSELECT',
    'MENU_SKULL','MENUSKULL','SKULL_CURSOR','SKULLCURSOR','SKULL_ARROW','SKULLARROW',
    'BUTTON','BUTTONS','BTN','BTNS','MENUBUTTON','MENU_BUTTON',
    'DOOR','DOORS','MENU_DOOR','MENUDOOR','DOOR_OPEN','DOOROPEN','DOOR_REVEAL','DOORREVEAL',
    'GATE','PORTAL','REVEAL','TRANSITION','TRANS','WIPE','FADE','CURTAIN','CURTAINS',
    'OPENING','CLOSING','OPENS','CLOSES','OPENDOOR','CLOSEDOOR',
    'TITLEDOOR','TITLE_DOOR','SHRINEDOOR','SHRINE_DOOR','BIGDOOR','BIG_DOOR',
    'PANEL','FRAME','BAR','BOX','BORDER','WINDOW','DIALOG','MODAL','POPUP','OVERLAY',
    'BG','BACKGROUND','BACKDROP','TITLE','LOGO','TITLELOGO','TITLE_LOGO','MENULOGO','MENU_LOGO',
    'SKULL','SKULLS','SKULLLOGO','SKULL_LOGO','BURP','BURP_LOGO','BURPLOGO',
]
CANDIDATES.extend(UI_ELEMENTS)

# Font / digit / number glyphs
FONT_GLYPHS = [
    'FONT','FONTS','BIGFONT','BIG_FONT','SMALLFONT','SMALL_FONT','TINYFONT','TINY_FONT','HUGE','MENU_FONT','MENUFONT',
    'GLYPH','GLYPHS','CHAR','CHARS','LETTER','LETTERS','TEXT','TEXTS','STRING','STRINGS',
    'DIGIT','DIGITS','NUM','NUMS','NUMBER','NUMBERS','NUMERAL','NUMERALS','NUMERIC','SCORE','SCOREFONT',
    'SCORE_FONT','SCORENUM','SCORE_NUM','SCOREDIGIT','SCORE_DIGIT','LIFEDIGIT','LIFE_DIGIT',
    'COUNTERFONT','COUNTER_FONT','COUNTERDIGIT','COUNTER','HUDFONT','HUD_FONT','HUDNUM','HUD_NUM','HUDDIGIT','HUD_DIGIT',
    'NUMBERFONT','NUMBER_FONT','NUMERIC_FONT','NUMERICFONT','DIGITFONT','DIGIT_FONT','DIGITSFONT','DIGITS_FONT',
    'NUMFONT','NUM_FONT','PWDFONT','PASSWORD_FONT','PASSWORDFONT','PWD_FONT',
    'PWD_DIGITS','PASSWORD_DIGITS','PWD_DIGIT','PASSWORD_DIGIT','PWDDIGIT','PWDDIGITS',
    '0123456789','S_NUMBERS','S_DIGITS','SPR_NUM','SPR_DIGITS','SPR_DIGIT','SPRDIGIT','SPRNUM',
    'NUMERIC_GLYPHS','NUMERICGLYPHS','NUMBER_GLYPHS','NUMBERGLYPHS',
    'CLOCKDIGIT','CLOCK_DIGIT','CLOCKFONT','CLOCK_FONT','CLOCKNUM','CLOCK_NUM',
    'TIMERDIGIT','TIMER_DIGIT','TIMERFONT','TIMER_FONT','TIMER',
    'NUMBER','TIMERNUM','LIFEDIGIT','LIVES','LIVE','LIFE','LIVESNUM','LIVES_NUM',
]
CANDIDATES.extend(FONT_GLYPHS)

# Dedupe and convert to uppercase
seen = set()
final = []
for c in CANDIDATES:
    cu = c.upper()
    if cu in seen:
        continue
    seen.add(cu)
    final.append(cu)

print('Testing %d UNIQUE candidates against %d WRAP+RAW targets:' % (len(final), len(TARGETS)))
print()
hits = []
for n in final:
    w = wrap(n)
    r = calcHash(n)
    if w in TARGETS:
        hits.append(('wrap', n, w, TARGETS[w]))
    if r in TARGETS:
        hits.append(('raw', n, r, TARGETS[r]))

for ns, n, h, tag in hits:
    print(f'  HIT ({ns}): {n:<25s} -> 0x{h:08x}  ({tag})')

if not hits:
    print('  No matches found.')
else:
    print()
    print(f'Found {len(hits)} hits.')
