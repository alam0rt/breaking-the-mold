"""Test all plausible MENU vocabulary as candidates for the low-floor MENU sprites.

The 4 cracked sprites are: NO, YES, QUIT, PAUSED, CONTINUE, QUIT GAME.
Pattern: short single English words OR two-word phrases (case-insensitive,
non-alnum stripped).
"""
import sys; sys.path.insert(0, 'tools/scripts')
from calchash import calcHash

def rotr(x, n): return ((x >> n) | (x << (32-n))) & 0xFFFFFFFF
SEED = 0x28C0E011

# All MENU-related uncracked low-floor sprites
MENU_TARGETS = {
    0x28a0c119: ('fl=5', 'duckling MENU'),
    0x38a0c119: ('fl=6', 'MENU sister sprite (z=2000)'),
    0x30a0c119: ('fl=7', 'duckling 76-frame'),
    0x68c01218: ('fl=8', 'MENU (z=2000, paired with 0x38a0c119)'),
}

# Plausible single English words for menu items
words = """
NO YES OK QUIT EXIT BACK NEXT PREV PAUSE PAUSED RESUME CONTINUE
START NEW LOAD SAVE LOADER SAVER NEWGAME LOADGAME SAVEGAME
NEW_GAME LOAD_GAME SAVE_GAME 
RESET RESTART
OPTIONS SETTINGS CONFIG CONTROLS CONTROL SETUP HELP ABOUT CREDITS
SOUND MUSIC VOLUME VIDEO DISPLAY SCREEN
PRESS PUSH HIT TAP HOLD CLICK RELEASE SELECT CHOOSE PICK ENTER
GAME OVER ENDED PASSED CLEARED DONE OVER
WIN LOSE LOST WON LOSING WINNING SUCCESS FAIL FAILURE
LIFE LIVES TIME DEAD ALIVE RESPAWN REVIVE
LEVEL STAGE WORLD AREA ZONE ROOM SCENE MAP
ROUND BOUT MATCH FIGHT BATTLE
SCORE BONUS RANK RANKED
GO READY SET STEADY HUP
WAIT SOON LATER NOW
HIGH LOW MID UP DOWN LEFT RIGHT
TITLE LOGO BANNER MAIN MAIN_MENU MAINMENU MAIN_TITLE MAINTITLE
INTRO OUTRO SPLASH OPENING CLOSING ENDING BEGIN END
DEMO PLAY PLAYBACK PROMO PREVIEW TRAILER
EASY MEDIUM HARD NORMAL EXPERT INSANE
CONFIRM CANCEL NEVERMIND APPLY ACCEPT DECLINE
CHOOSE PICK PICKED YES NO MAYBE
RETRY AGAIN
LANGUAGE LANG ENG ENGLISH FRENCH FR DEUTSCH SPANISH IT
PLAYER PLAYERS HERO PROTAG
1UP 1_UP UP DOWN 2UP 3UP
PASSWORD PASS CODE
PRINCESS PRINCE KING KINGS QUEEN QUEENS PRINCE_X3
DUCKS THREE_DUCKS DUCKY DUCKY_DUCKS THREEDUCKS DUCKLINGS
ROYALS ROYAL CROWN CROWNS CROWNED CROWN_X3 TRIPLE_CROWN
KLAYMEN KLAYMAN KLAY GLENN GLEN_YNTI EARTHWORM JIM
BABYDUCK BABYDUCKS BABY_DUCKS THREEBABYDUCKS
""".upper().replace('_',' ').split()

# Add common compound forms
combos = []
roots = ['NEW','LOAD','SAVE','MAIN','START','GAME','OPTIONS','SOUND','MUSIC',
         'CONTROLS','CONTROL','LANGUAGE','HELP','BACK','NEXT','SELECT','EXIT',
         'CONTINUE','PAUSE','RESUME','PRESS','START','SCREEN','PASS','RETRY']
for r in roots:
    combos.append(r)
    combos.append(r + ' GAME')
    combos.append('GAME ' + r)
    combos.append(r + ' SCREEN')
    combos.append(r + ' MENU')
    combos.append('MENU ' + r)

# Add compound MENU-specific phrases
phrases = [
    'NEW GAME','LOAD GAME','SAVE GAME','MAIN MENU','PAUSE MENU','OPTIONS MENU',
    'GAME OVER','HIGH SCORE','HIGHSCORE','PRESS START','PRESS ANY KEY',
    'PUSH START','PUSH ANY KEY','SELECT GAME','START GAME','CONTINUE GAME',
    'RESUME GAME','QUIT GAME','EXIT GAME','HELP MENU','SOUND MENU',
    'MUSIC ON','MUSIC OFF','SOUND ON','SOUND OFF','SFX ON','SFX OFF',
    'PLAYER 1','PLAYER 2','P1','P2','1 PLAYER','2 PLAYER','1P','2P',
    'GAME PAUSED','PAUSE','PAUSED GAME','LIFE LOST','GAME WON','GAME LOST',
    'YOU WIN','YOU LOSE','YOU WON','YOU LOST','GAME COMPLETE',
    'CREDITS','THE END','GAME OVER','TRY AGAIN','GAME ENDED',
    'CONTINUE Y N','CONTINUE YES NO','CONTINUE',
    'DEMO MODE','DEMO ENDS','END OF DEMO','DEMO OVER','PLAY DEMO',
    'INTRO','OUTRO','TITLE','SPLASH SCREEN','TITLE SCREEN',
    'SAVE OK','LOAD OK','SAVING','LOADING',
    'MEMORY CARD','SAVE FILE','SAVE SLOT','LOAD SLOT',
    'PASSWORD ENTRY','ENTER PASSWORD','PASSWORD',
    'BACK TO MENU','BACK TO TITLE','RETURN TO MENU','TO MENU','TO TITLE',
]
combos += phrases
combos += ['SKULL MONKEYS','THE NEVERHOOD','NEVERHOOD','PRESENTS','BY','FROM',
           'STARRING','FEATURING','EXTRAS','BONUS','THANKS',
           'EARTHWORM JIM','EWJ','EWJ 3D','SECRET']

# Combine uppercase variants
all_cands = set()
for w in words + combos:
    all_cands.add(w.upper())

# Add 1-9 digit variants
for w in list(all_cands):
    for d in '123456789':
        all_cands.add(w + ' ' + d)
        all_cands.add(d + ' ' + w)
        all_cands.add(w + d)
        all_cands.add(d + w)

print(f'Testing {len(all_cands)} candidates...')
print()
hits_total = 0
for tid, (fl, label) in MENU_TARGETS.items():
    inv_target = rotr(tid ^ SEED, 27)
    hits = [c for c in all_cands if calcHash(c) == inv_target]
    if hits:
        print(f'>>> 0x{tid:08x} {fl} {label}')
        for h in hits:
            print(f'    {h!r}')
        hits_total += len(hits)
    else:
        pass

if hits_total == 0:
    print('No MENU vocabulary hits.')

# Same test for ALL 13 low-floor targets - any hit at all
print()
print('=== Same vocab against all 13 low-floor uncracked targets ===')
ALL_T = {
    0x28a0c119: ('wrap', 'duckling/fl5'),
    0x38a0c119: ('wrap', 'sister/fl6'),
    0x30a0c119: ('wrap', 'duckling76/fl7'),
    0x68c01218: ('wrap', 'MENU/fl8'),
    0x088c5011: ('wrap', 'FINN/fl7'),
    0x28c080df: ('wrap', 'DEMO/fl7'),
    0x21842018: ('wrap', 'PLAYERBASE/fl8'),
    0x10882010: ('wrap', 'EVIL_clayball/fl8'),
    0x2c182010: ('wrap', 'MEGA_boss/fl8'),
    0x0c01c014: ('wrap', 'GLEN/fl8'),
    0x04084011: ('wrap', 'global_throw_anim/fl8'),
    0x29808408: ('raw',  'global_audio_22lev/fl8'),
    0x1847c001: ('raw',  'type700/fl8'),
}
for tid, (ns, label) in ALL_T.items():
    target = rotr(tid ^ SEED, 27) if ns == 'wrap' else tid
    hits = [c for c in all_cands if calcHash(c) == target]
    # Also try opposite ns
    target2 = tid if ns == 'wrap' else rotr(tid ^ SEED, 27)
    hits2 = [c for c in all_cands if calcHash(c) == target2]
    if hits or hits2:
        print(f'  0x{tid:08x} {label}: ns={ns} hits={hits} other_ns={hits2}')
