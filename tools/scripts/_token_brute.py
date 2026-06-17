"""Brute Klaymen-themed compound names against player base + all low-floor sprites.
"""
import sys, itertools
sys.path.insert(0, 'tools/scripts')
from calchash import calcHash

def rotr(x, n): return ((x >> n) | (x << (32-n))) & 0xFFFFFFFF
SEED = 0x28C0E011

# All 13 low-floor uncracked targets
TARGETS = {
    0x28a0c119: ('wrap', 'duckling/MENU/fl5'),
    0x38a0c119: ('wrap', 'sister/MENU/fl6'),
    0x30a0c119: ('wrap', 'duckling76/MENU/fl7'),
    0x68c01218: ('wrap', 'MENU/fl8 (z=2000)'),
    0x088c5011: ('wrap', 'FINN/fl7'),
    0x28c080df: ('wrap', 'DEMO_global/fl7'),
    0x21842018: ('wrap', 'PLAYERBASE_KLAYMEN/fl8'),
    0x10882010: ('wrap', 'EVIL_clayball/fl8'),
    0x2c182010: ('wrap', 'MEGA_boss_body/fl8'),
    0x0c01c014: ('wrap', 'GLEN/fl8'),
    0x04084011: ('wrap', 'global_throw_anim/fl8 (12 levels)'),
    0x29808408: ('raw',  'global_audio/fl8 (22 levels)'),
    0x1847c001: ('raw',  'type700/fl8'),
}

# Tokens from existing cracked names + likely additions
TOKENS_ENTITY = ['KLAY','KLAYMEN','SKULL','BIRD','RAT','PUFF','GUM','FINN','GLEN','GLENN',
                 'JOE','EVO','PHRO','HAMSTER','BAT','FROG','WORM','BUG','BEE','BABY']
TOKENS_ACTION = ['IDLE','BASE','STAND','READY','DEFAULT','STILL','POSE','REST','WAIT',
                 'WALK','RUN','JUMP','FALL','LAND','HIT','HURT','DIE','DEAD','EAT',
                 'SCREAM','GRUNT','BITE','FLAP','FLY','SPIN','TURN','BOUNCE',
                 'BLINK','LOOK','TALK','SHOUT','WAVE','PUNCH','THROW','SHOOT','PICKUP',
                 'CARRY','LIFT','HOLD','GRAB','RELEASE','DROP']
TOKENS_BODY = ['HEAD','BODY','ARM','LEG','FOOT','HAND','FACE','EYE','MOUTH','HAIR','HAT']
TOKENS_DESC = ['SOFT','HARD','BIG','SMALL','LITTLE','TINY','HUGE','MEGA','MINI','MICRO',
               'TALL','SHORT','THIN','FAT','BUFF','WEAK','STRONG','ANGRY','HAPPY','SAD']
TOKENS_DIRS = ['LEFT','RIGHT','UP','DOWN','NORTH','SOUTH','EAST','WEST','FRONT','BACK',
               'TOP','BOTTOM','SIDE','HIGH','LOW','MID']
TOKENS_PREFIX = ['FX','SFX','SND','M','S','MENU','UI','HUD','GLOBAL','GAME','BG','BANNER','LOGO']
TOKENS_NUMS = ['','01','02','03','04','05','1','2','3','4','5','11','12','22']
TOKENS_LEVEL = ['BOIL','BRG1','CAVE','CLOU','CRYS','CSTL','EGGS','EVIL','FINN','FOOD',
                'GLEN','GLID','HEAD','MEGA','MENU','MOSS','PHRO','RUNN','SCIE','SEVN',
                'SNOW','SOAR','TMPL','WEED','WIZZ','KLOG']

# Build candidates
candidates = set()

# Single tokens
all_tokens = set(TOKENS_ENTITY+TOKENS_ACTION+TOKENS_BODY+TOKENS_DESC+TOKENS_DIRS+
                 TOKENS_PREFIX+TOKENS_LEVEL)
all_tokens.update(['THREE','3','TRIO','CROWNED','ROYAL','REGAL','BANNER','LOGO',
                   'SCREEN','TITLE','FRAME','BORDER','PLATE','TAB','BAR','BTN','BOX',
                   'CLAYBALL','CLAY','BALL','BONUS','SHIELD','PHOENIX','GLIDEY',
                   'FARTHEAD','SUPER','WILLIE','UNIVERSE','ENEMA','PICKUP','GROW',
                   'PASSWORD','SELECT','PAUSE','PAUSED','QUIT','EXIT','CONTINUE',
                   'PROMO','DEMO','OUTRO','INTRO','SPLASH','START','READY','GO',
                   'OK','YES','NO','BACK','NEXT','PREV','HOME','MENU'])

for t in all_tokens:
    candidates.add(t)
    # Numeric variants
    for n in TOKENS_NUMS:
        candidates.add(t + n)
        if n: candidates.add(t + '_' + n)
        candidates.add(n + t if n else t)

# 2-token compounds with various separators
for a in TOKENS_ENTITY + TOKENS_LEVEL + TOKENS_PREFIX + ['DEMO','MENU','BANNER']:
    for b in TOKENS_ACTION + TOKENS_BODY + TOKENS_DESC + ['BASE','IDLE','BANNER',
                                                          'LOGO','TITLE','BG']:
        for sep in ['', '_', ' ']:
            candidates.add(a + sep + b)
            candidates.add(b + sep + a)

# 3-token compounds (FX_X_Y)
for p in TOKENS_PREFIX[:5]:
    for e in TOKENS_ENTITY:
        for a in TOKENS_ACTION:
            candidates.add(f'{p}_{e}_{a}')
            candidates.add(f'{p}{e}{a}')

# Add cracked-name-style FX prefixes
for e in TOKENS_ENTITY + TOKENS_LEVEL:
    for a in TOKENS_ACTION + TOKENS_BODY + TOKENS_DESC + ['BASE','IDLE']:
        candidates.add(f'FX_{e}_{a}')
        candidates.add(f'SFX_{e}_{a}')
        candidates.add(f'M_{e}_{a}')
        candidates.add(f'S_{e}_{a}')

# Klaymen-specific
candidates.update([
    'Klaymen','klaymen','Klaymen_base','klaymen_base','Klaymen_idle',
    'Klaymen_run','Klaymen_walk','Klaymen_jump','Klaymen_fall',
    'Klaymen_idle_anim','Klaymen_walk_anim','Klaymen_run_anim',
    'Klaymen_base_sprite','Klaymen_player','Klaymen_default',
    'Klaymen_pose','Klaymen_anim_base',
    'KLAYMEN_BASE','KLAYMEN_IDLE','KLAYMEN_DEFAULT','KLAYMEN_PLAYER',
    'PLAYER_KLAYMEN','HERO_KLAYMEN','BASE_KLAYMEN','IDLE_KLAYMEN',
])

# Mega boss body
candidates.update([
    'MEGA_BODY','MEGABODY','MEGA_BOSS_BODY','MEGABOSSBODY','BOSS_BODY_MEGA',
    'mega','Mega','Mega_body','MegaBody','Mega_Body',
    'MEGA_BOSS_TORSO','MEGA_TORSO','MEGABOSS_TORSO','MEGA_FRAME','MEGAFRAME',
    'MEGA_LIMB','MEGALIMB','MEGA_GUY','MEGAGUY','MEGA_MAN','MEGAMAN',
    'BOSS_MEGA','MEGABOSS','MEGA_BOSS',
])

# Duckling/three crowned
candidates.update([
    'TRIO','3DUCKS','3KING','3KINGS','3PRINCE','3PRINCES','TRIPLE','3CROWN','3CROWNS',
    'CROWN_TRIO','TRIO_CROWN','THREE_CROWN','THREE_CROWNS','CROWNTRIO','TRIPCROWN',
    'PRINCESSES','PRINCES','PRINCESS_TRIO','PRINCESSES','SISTERS','BROTHERS',
    'BABY3','BABIES','3BABIES','THREE_BABIES','BABY_TRIO','TRIPBABY',
    'GANG_OF_3','THREE_GUYS','3GUYS','TRIO_GUYS',
])

# Demo banner / global
candidates.update([
    'DEMO_BANNER','DEMOBANNER','DEMO_LOGO','DEMOLOGO','DEMO_TXT','DEMOTXT',
    'DEMO_TAG','DEMOTAG','DEMO_OVERLAY','DEMOOVERLAY','DEMO_MARK','DEMOMARK',
    'DEMO_BAR','DEMOBAR','DEMO_FRAME','DEMOFRAME','DEMO_BORDER','DEMOBORDER',
    'DEMO_TITLE','DEMOTITLE','DEMO_HEADER','DEMOHEADER','DEMO_PROMO','DEMOPROMO',
])

# Bonus clayball
candidates.update([
    'CLAYBALL','CLAY_BALL','BONUS_CLAY','BONUSCLAY','CLAY_BONUS','CLAYBONUS',
    'BONUSBALL','BONUS_BALL','BIG_BALL','BIGBALL','SUPER_BALL','SUPERBALL',
    'CLAY','CLAYS','clay','Clay','BIG_CLAY','BIGCLAY','SUPER_CLAY','SUPERCLAY',
    'EVIL_CLAY','EVILCLAY','EVIL_BALL','EVILBALL','EVIL_CLAYBALL','EVILCLAYBALL',
])

# Throw animation
candidates.update([
    'THROW','THROWING','THROW_ANIM','THROWANIM','TOSS','HURL','PITCH','SHOOT',
    'BALL_THROW','THROWBALL','THROW_BALL','HUCK','LOB','CHUCK','CAST',
    'KLAY_THROW','THROWKLAY','THROW_POSE','THROWPOSE','THROWSTART','THROW_START',
])

# Audio global
candidates.update([
    'BEEP','PING','CHIME','BLIP','POP','CLICK','TICK','TAP','TIK',
    'UI_CONFIRM','UI_BEEP','UI_BLIP','UI_PING','UI_CLICK','UI_OK',
    'MENU_BEEP','MENU_BLIP','MENU_PING','MENU_CLICK','MENU_OK',
    'GLOBAL_BEEP','GLOBAL_BLIP','GLOBAL_PING','GLOBAL_CLICK','GLOBAL_OK',
    'POP','PLOP','BLOP','BLOOP','BLOP1','BLOP2','BLEEP','BLEEP1','BLEEP2',
])

print(f'Testing {len(candidates)} candidates...')
print()

for tid, (ns, label) in TARGETS.items():
    target_w = rotr(tid ^ SEED, 27)
    target_r = tid
    hits_w = [c for c in candidates if calcHash(c) == target_w]
    hits_r = [c for c in candidates if calcHash(c) == target_r]
    if hits_w or hits_r:
        print(f'>>> 0x{tid:08x} {label}')
        for h in hits_w:
            print(f'    WRAP-hit: {h!r}')
        for h in hits_r:
            print(f'    RAW-hit:  {h!r}')

print()
print('Total iterations:', len(candidates) * len(TARGETS) * 2)
