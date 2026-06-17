"""WRAP-namespace attack for menu/UI sprites.

The WRAP namespace formula:
    id = 0x28C0E011 ^ rotl(calcHash(name), 27)

Used by: NO/YES/PAUSED/QUIT/CONTINUE/QUITGAME (verified text anchors)

Visual identifications from sprite_identification_template.csv:
  - 0x00e2f188 Number glyph frames 0-9 (all 26 levels - WRAP likely)
  - 0x33808e1b Menu cursor animation frame (MENU only - WRAP likely)
  - 0x63848e59 Menu cursor (MENU only - WRAP likely)
  - 0x4a3ea110 Menu door opens to reveal monkey (MENU - WRAP likely)
  - 0x8ab92024 LOADING screen text + Klaymen (all 26 levels - WRAP likely)
  - 0x61981a0c Plain monkey enemy (gameplay - RAW more likely)
  - 0xc8f90114 GLEN boss (gameplay - RAW more likely)
  - 0x0b2084d0 Klaymen running variant (all 21 - mixed)
  - 0x88b9833c Klaymen leaping (all 21 - mixed)
"""
import sys
sys.path.insert(0, 'tools/scripts')
from calchash import calcHash

def rotl(v, r):
    r &= 31
    return ((v << r) | (v >> ((32 - r) & 31))) & 0xFFFFFFFF

def wrap(name):
    return 0x28C0E011 ^ rotl(calcHash(name), 27)

# Targets that are visually-confirmed and live in WRAP/UI scope
TARGETS = {
    0x00e2f188: 'NUM/DIGIT glyphs 0-9 (all levels)',
    0x33808e1b: 'Menu cursor anim frame (MENU only)',
    0x63848e59: 'Menu cursor (MENU only)',
    0x4a3ea110: 'Menu door reveal (MENU only)',
    0x8ab92024: 'LOADING text + Klaymen (all levels)',
    0x28c080df: 'misc UI sprite (all 26 levels - guess)',
    # MENU-only sprites from cracked_names that are uncracked
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
}

# Candidate name dictionary - UI/menu/text
CANDIDATES = [
    # Numeric/digit
    'NUM','NUMS','NUMBER','NUMBERS','DIGITS','DIGIT','NUMERIC','NUMBERFONT','NUMBER_FONT',
    'NUMERIC_FONT','NUMERICFONT','DIGITFONT','DIGIT_FONT','DIGITS_FONT','NUMFONT','NUM_FONT',
    'FONT','FONTS','BIGFONT','BIG_FONT','SMALLFONT','SMALL_FONT','TINYFONT','TINY_FONT',
    'GLYPH','GLYPHS','CHARS','CHARSET','CHAR_SET','LETTERS','SCOREFONT','SCORE_FONT',
    'SCORENUM','SCORE_NUM','SCOREDIGIT','SCORE_DIGIT','LIFEDIGIT','LIFE_DIGIT',
    'COUNTERFONT','COUNTER_FONT','COUNTERDIGIT','COUNTER','HUDFONT','HUD_FONT','HUDNUM',
    'NUMERAL','NUMERALS','DIGIT_GLYPH','DIGITGLYPH','GLYPH_DIGIT',
    'TXT_NUMBERS','TEXT_NUMBERS','TXT_DIGITS','TEXT_DIGITS','TXT_NUM','TEXT_NUM',
    'PWD_DIGITS','PASSWORD_DIGITS','PWD_DIGIT','PASSWORD_DIGIT','PWDFONT','PWD_FONT',
    'PASSWORDDIGIT','PASSWORDDIGITS','PASSWORDFONT','PASSWORD_FONT',
    '0123456789','S_NUMBERS','S_DIGITS','SPR_NUM','SPR_DIGITS','SPR_DIGIT','SPRDIGIT','SPRNUM',
    # Cursor/pointer
    'CURSOR','CURSORS','CURSOR_ANIM','CURSORANIM','CURSOR_ICON','CURSORICON',
    'MENU_CURSOR','MENUCURSOR','MENUARROW','MENU_ARROW','MENUPOINTER','MENU_POINTER',
    'POINTER','POINTERS','POINTER_ANIM','POINTERANIM','POINTER_ICON','POINTERICON',
    'ARROW','ARROWS','ARROW_ANIM','ARROWANIM','MENU_HAND','MENUHAND','HAND',
    'HIGHLIGHT','HIGHLIGHTER','FOCUS','SELECTOR','SELECT','SEL','SELECTION',
    'CHOICE','CHOOSER','PICK','PICKER','MARKER','TARGET','BLINKER',
    'CURSOR_SKULL','SKULLCURSOR','SKULL_CURSOR','SKULLICON','SKULL_ICON','SKULL',
    'SKULLPOINT','SKULL_POINT','MENU_SKULL','MENUSKULL','SKULL_ARROW','SKULLARROW',
    # Door/Door animation
    'DOOR','DOORS','DOOR_OPEN','DOOROPEN','DOOR_ANIM','DOORANIM',
    'MENU_DOOR','MENUDOOR','SHRINE_DOOR','SHRINEDOOR','BIGDOOR','BIG_DOOR',
    'TITLE_DOOR','TITLEDOOR','REVEAL','REVEAL_DOOR','REVEALDOOR','SHOW_DOOR','SHOWDOOR',
    'TRANSITION','TRANS','TRANSITION_DOOR','TRANSDOOR','WIPE','FADE','FADE_DOOR',
    'OPENING','OPENING_DOOR','OPENINGDOOR','CLOSING','CLOSING_DOOR','GATE','PORTAL',
    'CURTAIN','CURTAINS','VEIL','REVEAL_VEIL',
    # Loading screen
    'LOADING','LOADTEXT','LOAD_TEXT','LOADING_TEXT','LOADING_SCREEN','LOADSCREEN','LOAD_SCREEN',
    'LOAD','LOADER','LOADBAR','LOAD_BAR','LOAD_KLAY','LOAD_KLAYMEN','LOADING_KLAY','LOADING_KLAYMEN',
    'PLEASEWAIT','PLEASE_WAIT','WAIT','WAITING','BUSY','BUSY_SCREEN','BUSYSCREEN',
    'SAVE_LOAD','SAVELOAD','LOAD_SAVE','LOADSAVE','SAVE','SAVING','SAVE_SCREEN',
    'TXT_LOADING','TEXT_LOADING','S_LOADING','SPR_LOADING','SPR_LOAD',
    # Menu buttons / text labels
    'BUTTON','BUTTONS','MENU_BUTTON','MENUBUTTON','BTN','BTNS',
    'PASSWORD','PASS','PWD','OPTIONS','OPTION','SETTINGS','CONFIG',
    'EXIT','EXITGAME','EXIT_GAME','STARTGAME','START_GAME','START','BEGIN',
    'PRESS','PRESSSTART','PRESS_START','TITLE','TITLESCREEN','TITLE_SCREEN',
    'MAINMENU','MAIN_MENU','MAIN','SUBMENU','SUB_MENU','PAUSEMENU','PAUSE_MENU','PAUSE',
    'SCORE','HISCORE','HIGHSCORE','HIGH_SCORE','PLAYER_SCORE',
    'SAVE','LOAD','DELETE','CREATE','NEW','NEW_GAME','NEWGAME','CONTINUE_GAME','CONTINUEGAME',
    'MOVIE','MOVIES','MUSIC','SOUND','SFX','VOLUME','LANG','LANGUAGE',
    'INTRO','OUTRO','TITLE_INTRO','GAMEOVER','GAME_OVER','GAME','LEVELSELECT','LEVEL_SELECT',
    # Skullmonkey specific UI
    'SKULLMONKEYS','SKULLMONKEY','SKULLMNKY','LOGO','LOGO_ANIM','LOGOANIM','TITLE_LOGO','TITLELOGO',
    'SKULLLOGO','SKULL_LOGO','BURP_LOGO','LOGO_INTRO','LOGOINTRO',
    'CREDITS','CREDIT','THANKS','THE_END','THEEND','END','ENDING','ENDSCREEN',
    'COMPANY','COPYRIGHT','TM','REGISTERED',
    # Common UI primitives
    'BG','BACKGROUND','BACKDROP','PANEL','FRAME','BAR','LINE','BOX','BORDER',
    'WINDOW','DIALOG','MODAL','POPUP','POP_UP','OVERLAY','POPOVER',
    'CHECKBOX','CHECK','RADIO','BULLET','STAR','HEART','TICK','MARK',
    # Klaymen running text variants
    'KLAY_RUN','KLAYMEN_RUN','KLAYMENRUN','KLAY_RUNNING','KLAYMEN_RUNNING','RUN_KLAY',
    'KLAY_FAST','KLAYMEN_FAST','RUNNING','RUNNINGKLAY','RUNANIM','RUN_ANIM',
    'CHARGE','CHARGEKLAY','CHARGE_KLAY','BURST','BURSTKLAY','BURST_KLAY',
]

print('Testing %d candidates against %d WRAP targets:' % (len(CANDIDATES), len(TARGETS)))
print()
hits = []
for n in CANDIDATES:
    w = wrap(n)
    if w in TARGETS:
        hits.append((n, w, TARGETS[w]))
        print('  HIT (wrap): %-25s -> 0x%08x  (%s)' % (n, w, TARGETS[w]))

if not hits:
    print('  No matches found.')
else:
    print()
    print('Found %d hits.' % len(hits))
