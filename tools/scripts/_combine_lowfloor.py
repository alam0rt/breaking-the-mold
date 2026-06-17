"""Build a tight WRAP-targeted wordlist combining:
- All single English words L=3..7 (likely menu items)
- Skullmonkeys-specific tokens
- Common UI/game-state words

Then run kcrack combine depth=3 against the 13 low-floor inverse-WRAP targets.
"""
import os, subprocess

MENU_WORDS = """
NEW LOAD SAVE GAME OVER START PRESS PAUSE PAUSED RESUME CONTINUE QUIT EXIT
BACK NEXT PREV HOME OPTIONS SETTINGS CONFIG SOUND MUSIC VOLUME CONTROLS
HELP ABOUT CREDITS LANGUAGE SELECT CHOOSE PICK ENTER OK CANCEL APPLY YES
NO MAYBE TRUE FALSE ON OFF UP DOWN LEFT RIGHT GO STOP TITLE LOGO INTRO
OUTRO SPLASH BANNER FRAME BORDER PLATE BOX TAB BAR BUTTON BTN ICON SIGN
MARK STAMP TAG OVERLAY UPPER LOWER TOP BOTTOM
""".split()

GAME_WORDS = """
KLAY KLAYMEN KLAYMAN HERO PLAYER MAIN PROTAG SKULL MONKEY MONK BABY
JOE EVO PUFF GUM GLEN GLENN YNTI YNTIS RAT RATS BIRD WORM HAMSTER
PHRO PHROG FROG BAT WIZZ NEVERHOOD SKULLMONKEYS WILLIE PHOENIX
BOSS HEAD MEGA TURN WALK FIRE FLY LAND JUMP RUN HOP DIE DEAD HIT HURT
BITE EAT CHEW SCREAM YELL GRUNT FLAP DASH LAUNCH BLOW BLAST SPLAT
EXPLODE SMASH CRASH BUMP DROP THROW HURL TOSS PITCH KICK PUNCH GRAB
HOLD CARRY LIFT PULL PUSH SHOVE BURP FART POOP PEE SPIT
PICKUP CLAYBALL CLAY BALL BONUS SHIELD GROW POWER HEALTH LIFE LIVES
SPAWN RESPAWN ALIVE REVIVE
EARTH EARTHWORM JIM EWJ FARTHEAD GLIDEY UNIVERSE ENEMA SUPER ICONIC
SOFT QUIET LOUD HIGH LOW BIG SMALL TINY HUGE MEGA MINI BIG MEDIUM
ANIM SPRITE SOUND MUSIC FX SFX SND
IDLE REST BASE STAND READY WAIT STILL POSE FRAME FRONT BACK SIDE
LOAD SAVE NEW GAME PAUSE QUIT EXIT
PASSWORD CODE PASS ENTRY KEY KEYS
ONE TWO THREE FOUR FIVE SIX SEVEN EIGHT NINE TEN ZERO 
DEMO PROMO TEASE INTRO OUTRO SPLASH WELCOME BANNER LOGO SIGN
""".split()

LEVEL_NAMES = ['BOIL','BRG1','CAVE','CLOU','CRYS','CSTL','EGGS','EVIL','FINN','FOOD',
               'GLEN','GLID','HEAD','MEGA','MENU','MOSS','PHRO','RUNN','SCIE','SEVN',
               'SNOW','SOAR','TMPL','WEED','WIZZ','KLOG']
NUMS = ['','1','2','3','4','5','6','7','8','9','01','02','03','04','05','11','12']
DIRS = ['LEFT','RIGHT','UP','DOWN','TOP','BOT','BOTTOM','FRONT','BACK','SIDE']

# Build wordlist
words = set()
words.update(MENU_WORDS)
words.update(GAME_WORDS)
words.update(LEVEL_NAMES)
words.update(NUMS)
words.update(DIRS)
words = sorted(set(w.upper() for w in words if w))
words = [w for w in words if 1 <= len(w) <= 12]
print(f'Wordlist: {len(words)} entries')
with open('/tmp/wl_lowfloor.txt', 'w') as f:
    for w in words:
        f.write(w + '\n')

# Run kcrack combine depth=3
result = subprocess.run(
    ['./tools/kcrack', 'combine', '/tmp/wl_lowfloor.txt', '/tmp/targets_low_brute.txt',
     '--raw', '--depth', '3'],
    cwd='/home/sam/projects/btm', capture_output=True, text=True, timeout=900
)
print('=== STDERR ===')
print(result.stderr[:1000])
print('=== HITS ===')
print(result.stdout[:5000])
print()
nlines = result.stdout.count('\n')
print(f'Total: {nlines} hit lines')

# Save to file
with open('/tmp/combine_lowfloor_d3.txt', 'w') as f:
    f.write(result.stdout)
print('-> /tmp/combine_lowfloor_d3.txt')
