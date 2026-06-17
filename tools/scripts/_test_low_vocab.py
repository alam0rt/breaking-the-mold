"""Test plausible vocabulary against low-floor uncracked sprite/audio targets."""
import sys; sys.path.insert(0, 'tools/scripts')
from calchash import calcHash

def rotr(x, n): return ((x >> n) | (x << (32-n))) & 0xFFFFFFFF
SEED = 0x28C0E011

# All 13 low-floor uncracked targets
family = {
    0x28a0c119: ('floor=5','sprite','duckling visual MENU'),
    0x38a0c119: ('floor=6','sprite','MENU sister'),
    0x30a0c119: ('floor=7','sprite','duckling 76-frame MENU'),
    0x68c01218: ('floor=8','sprite','MENU'),
    0x088c5011: ('floor=7','sprite','FINN'),
    0x28c080df: ('floor=7','sprite','DEMO text stamp - global'),
    0x21842018: ('floor=8','sprite','Klaymen base FINN'),
    0x10882010: ('floor=8','sprite','EVIL'),
    0x2c182010: ('floor=8','sprite','MEGA boss body'),
    0x0c01c014: ('floor=8','sprite','GLEN'),
    0x04084011: ('floor=8','sprite','global'),
    0x29808408: ('floor=8','audio',  'global audio'),
    0x1847c001: ('floor=8','type700','unknown ns'),
}

def to_raw(fid, ns):
    if ns == 'audio':
        return fid, 'raw'
    if ns == 'type700':
        return fid, 'raw_or_wrap'
    return rotr(fid ^ SEED, 27), 'wrap_inverse'

# Massive single-word vocabulary
words = """
OK YES NO START READY GO OFF ON HOLD WAIT PLAY PAUSE PAUSED QUIT EXIT
BACK NEXT PREV MORE MENU TITLE INTRO OUTRO GAME OVER DEMO BEGIN END
OPEN CLOSE LOAD SAVE OPTION OPTIONS CONTROL CONTROLS CONFIG SETUP HELP
SOUND MUSIC VIDEO VOLUME PRESS PUSH HIT TAP SELECT CHOOSE PICK ENTER
KING KINGS QUEEN PRINCE PRINS REGAL ROYAL NOBLE LORD LADY HERO HEROS
HEROES CROWN CROWNS THRONE KNIGHT KNIGHTS RULER BARON DUKE EARL SIRE
DUCK DUCKS DUCKY DUCKIE DUCKLING DUCKLINGS CHICK CHICKS BIRD BIRDS FLOCK
TRIO BABY BABIES TOTS LITTLE TINY THREE TRIPLE TWINS TWIN
KLAY KLAYMEN KLAYMAN SKULL SKULLY MONKEY MONKEYS APE APES MONK MONKS
JOE JOES EVO EVOS EVIL PUFF PUFFS GUM GUMS GLEN GLENN GLENNS YNTI YNTIS
RAT RATS WORM EARTHWORM PHRO PHROG FROG FROGS HAMSTER HAMSTERS BAT BATS
BOIL BRG1 CAVE CLOU CRYS CSTL EGGS EVIL FINN FOOD GLEN GLID HEAD MEGA
MENU MOSS PHRO RUNN SCIE SEVN SNOW SOAR TMPL WEED WIZZ KLOG
WIN LOSE BONUS EXTRA SCORE HIGH LOW TIME LIFE LIVES LEVEL STAGE
WORLD AREA ZONE ROUND BOSS MISS WARP DIE DEAD ALIVE
EARTH PRESS SHOOT FIGHT BLAST POWER FALLS FALL JUMP JUMPS JUMPED
CRACK SPLAT SPRAY SHELL EGG FOOT FEET HAND FIST FOOTS HEAD HEADS
BUSH GRASS TREE FERN LEAF ROCK RUBY GEM GEMS GOLD SILVER STAR STARS
MOON SUN SKY DUSK DAWN DAY NIGHT 1UP UP DOWN LEFT RIGHT
PASS PASSWORD CODE HUGS HUG LOVE SOUL HOPE NEW OLD BIG SMALL TALL
SHORT FAST SLOW YEAH OOPS WOW OUCH BAM POW BOOM DEMO LOGO BANNER
GANG TROUPE BAND CROWD PEOPLE FOLKS GUYS LOVERS MONKS PRINCESS
NEVERHOOD SKULL MONKEYS SPRINKLE SPLAT BOO BOOM CRACK BUST BURST
LEFT RIGHT EAST WEST NORTH SOUTH N S E W
PLAYER PLAYER1 PLAYER2 P1 P2 PLAY1 PLAY2
SPLAT SPLATTER SLAM CRASH SMASH GRAB CATCH THROW HURL TOSS PUSH
BIG SMALL FAT THIN BUFF SLIM TALL WEE
ONE TWO THREE FOUR FIVE SIX SEVEN EIGHT NINE TEN
ZERO ONCE TWICE TRIPLE QUAD QUINT
HERO HEROS BABY BABE KID KIDDO BUDDY DUDE BRO PAL PALS
ROYAL ROYALS REGAL THRONE KING1 KING2 KING3 KINGS3 KINGS_3
GUYS GANG TRIOS TWOS BUNCH PACK
BANNER LOGO TITLE BACKDROP SPLASH FRAME BORDER
CHOOSE CHOSEN PICK SELECT START NEW LOAD SAVE
PRINCE PRINCES PRINCESS PRINCESSES
EARTHWORM JIM EWJ
SLEDGE HAMMER MALLET
TWIST CRANK ROLL SPIN SWIRL TWIRL
LOGO ICON SIGN MARK BADGE STAMP
ROCKET ZIP DASH RUN ZOOM
CRYO ICE FROST CHILL BURN HEAT FIRE FLAME
DARK LIGHT SHADE SHADOW
BUTTON KNOB DIAL LEVER SWITCH
EYE EYES MOUTH NOSE EAR EARS HAIR FIST HAND
WALK RUN RUNN HOP JUMP LEAP DIVE FLY GLIDE
COLLECT GRAB PICKUP
""".split()

# Hardcoded extras
words += [
    'HUH','HEY','YO','HUGGY','SLAM','SMASH','HUNK','HUNKY',
    'KLAYMEN','KLAY1','KLAY2','KLAY3','KLAY4',
    'SKULLBOT','SKULLBOTS',
    'BOSS1','BOSS2','BOSS3','BOSSES',
    'JOEHEAD','JOEHEADJOE','JOE1','JOE2','JOE3',
    'EWJ3D','EWJX','EWJ3','SKMK','SKM',
    '3KINGS','3PRINCE','3PRINCES','3CROWNS',
]

# Add common name patterns (FX_, SFX_)
prefixes = ['FX_','SFX_','SND_','S_','M_','MENU_','UI_','HUD_']
suffixes = ['_LOOP','_LP','_BIG','_SM','_MED','_M','_S','_L','_END','_START','_BEGIN',
            '_01','_02','_03','_1','_2','_3','_NEW','_OLD','_FX','_GO','_OK',
            '_ICON','_LOGO','_BANNER','_TITLE','_HEAD','_TXT','_TEXT']

cands = set(w.upper() for w in words if w)
# Combine with prefixes/suffixes for known game-style names
augmented = set(cands)
for w in cands:
    for p in prefixes:
        augmented.add((p+w).upper())
    for s in suffixes:
        augmented.add((w+s).upper())
print(f'Testing {len(augmented)} augmented candidates against {len(family)} targets...')
print()

raw_targets_audio = {fid: fid for fid, info in family.items() if info[1] == 'audio'}
raw_targets_wrap = {fid: rotr(fid ^ SEED, 27) for fid, info in family.items() if info[1] == 'sprite'}
raw_targets_type700 = {fid: fid for fid, info in family.items() if info[1] == 'type700'}
raw_targets_type700_w = {fid: rotr(fid ^ SEED, 27) for fid, info in family.items() if info[1] == 'type700'}

print('=== sprite targets (WRAP, brute against inverse-WRAP) ===')
for fid, info in family.items():
    if info[1] != 'sprite': continue
    target = raw_targets_wrap[fid]
    hits = [c for c in augmented if calcHash(c) == target]
    print(f'  0x{fid:08x} {info[0]} {info[2]}: {hits if hits else "no hit"}')

print()
print('=== audio targets (RAW) ===')
for fid, info in family.items():
    if info[1] != 'audio': continue
    target = raw_targets_audio[fid]
    hits = [c for c in augmented if calcHash(c) == target]
    print(f'  0x{fid:08x} {info[0]} {info[2]}: {hits if hits else "no hit"}')

print()
print('=== type700 (try both ns) ===')
for fid, info in family.items():
    if info[1] != 'type700': continue
    target_r = raw_targets_type700[fid]
    target_w = raw_targets_type700_w[fid]
    hits_r = [c for c in augmented if calcHash(c) == target_r]
    hits_w = [c for c in augmented if calcHash(c) == target_w]
    print(f'  0x{fid:08x} {info[0]} {info[2]}: raw={hits_r} wrap={hits_w}')
