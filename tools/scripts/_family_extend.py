"""Extend known audio families to find new sibling names."""
import sys, csv
sys.path.insert(0, 'tools/scripts')
from calchash import calcHash

# Load uncracked audio IDs
unknowns = {}
with open('docs/analysis/asset-identification/cracked_names.csv') as f:
    for r in csv.DictReader(f):
        if r['status'] == 'uncracked' and r['type'] == 'audio':
            unknowns[int(r['id_hex'], 16)] = (r['levels'], int(r['floor']))

# Verified families to extend
families = {
    'FX_KLAY': [
        # actions
        'JUMP','LAND','FALL','WALK','RUN','HIT','HURT','DIE','DEATH',
        'DUCK','DUCK_DOWN','DUCK_UP','DUCK_LEFT','DUCK_RIGHT',
        'FOOTSTEP','FOOTSTEP_LEFT','FOOTSTEP_RIGHT','FOOTSTEP_QUIET',
        'FOOTSTEP_LEFT_SOFT','FOOTSTEP_RIGHT_SOFT','FOOTSTEP_QUIET_SOFT',
        'RUN_FAST','RUN_FAST_SOFT','RUN_LEFT','RUN_RIGHT',
        'JUMP_SOFT','LAND_SOFT','HURT_SOFT',
        'FART','BURP','LAUGH','CRY','SCREAM','YELL','EAT','BITE',
        'HIT_HEAD','HIT_HEAD_SOFT','PICKUP','GRAB',
        'CRAWL','SLIDE','ROLL','SPIN','SLIP',
        'DIE_FALL','DIE_HURT','DIE_HIT','DIE_LAND','DIE_FAR','DIE_BIG',
        'BREATHE','BREATH','PANT','GASP','SIGH','HUFF','HUM',
        'CLIMB','LIFT','PUSH','PULL','THROW','TOSS','KICK','PUNCH',
        'BLINK','EYES','LOOK','FACE','TURN','HEAD','BODY','ARM',
        'JUMP_END','LAND_END','HURT_END','DIE_END',
        'JUMP_01','LAND_01','HURT_01','DIE_01','RUN_01','WALK_01',
        'JUMP_02','LAND_02','HURT_02','DIE_02','RUN_02','WALK_02',
    ],
    'FX_SKULL': [
        'JUMP','LAND','LAND_01','LAND_02','LAND_03',
        'FIRE','FIRE_01','FIRE_02','FIRE_03',
        'SCREAM','SCREAM_01','SCREAM_02','SCREAM_03',
        'BITE','BITE_01','BITE_02','BITE_03',
        'FLY','FLY_01','FLY_02','FLY_03','FLY_04',
        'EAT','EAT_01','EAT_02',
        'SPRING','SPRING_01','SPRING_02','SPRING_03',
        'GRUNT','GRUNT_01','GRUNT_02','GRUNT_03',
        'UP','UP_01','UP_02',
        'FLAP','FLAP_01','FLAP_02',
        'WALK','WALK_01','WALK_02',
        'RUN','RUN_01','RUN_02',
        'DIE','DIE_01','DIE_02','DIE_03',
        'HURT','HURT_01','HURT_02','HURT_03',
        'IDLE','IDLE_01','IDLE_02','IDLE_03',
        'FOOTSTEP','FOOTSTEP_LEFT','FOOTSTEP_RIGHT',
        'TURN','TURN_01','TURN_02',
        'HIT','HIT_01','HIT_02','HIT_03',
        'ATTACK','ATTACK_01','ATTACK_02','ATTACK_03',
    ],
    'FX_BOSS': [
        'HEAD','HEAD_HIT','HEAD_TURN','HEAD_WALK','HEAD_IDLE','HEAD_IDLE_01','HEAD_IDLE_02',
        'HEAD_DIE','HEAD_DEATH','HEAD_ATTACK','HEAD_HURT','HEAD_SCREAM','HEAD_ROAR',
        'HEAD_JUMP','HEAD_LAND','HEAD_FALL','HEAD_TAUNT','HEAD_LAUGH',
        'HEAD_BOUNCE','HEAD_HIDE','HEAD_REVEAL','HEAD_RISE','HEAD_GROW','HEAD_SHRINK',
        'HEAD_01','HEAD_02','HEAD_03','HEAD_04','HEAD_05',
        'BIG','BIG_HIT','BIG_TURN','BIG_WALK','BIG_IDLE','BIG_DIE','BIG_ATTACK',
        'GLEN','GLEN_HIT','GLEN_TURN','GLEN_WALK','GLEN_IDLE','GLEN_DIE','GLEN_ATTACK','GLEN_HURT',
        'GLENN','GLENN_HIT','GLENN_TURN','GLENN_WALK','GLENN_IDLE','GLENN_DIE','GLENN_ATTACK','GLENN_HURT',
        'KLOG','KLOG_HIT','KLOG_TURN','KLOG_WALK','KLOG_IDLE','KLOG_DIE','KLOG_ATTACK','KLOG_HURT',
        'KLOGG','KLOGG_HIT','KLOGG_TURN','KLOGG_WALK','KLOGG_IDLE','KLOGG_DIE','KLOGG_ATTACK','KLOGG_HURT',
        'WIZZ','WIZZ_HIT','WIZZ_TURN','WIZZ_WALK','WIZZ_IDLE','WIZZ_DIE','WIZZ_ATTACK','WIZZ_HURT',
        'WIZARD','WIZARD_HIT','WIZARD_TURN','WIZARD_WALK','WIZARD_IDLE','WIZARD_DIE','WIZARD_ATTACK','WIZARD_HURT',
        'MAGE','MAGE_HIT','MAGE_TURN','MAGE_WALK','MAGE_IDLE','MAGE_DIE','MAGE_ATTACK','MAGE_HURT',
        'SHRINEY','SHRINEY_HIT','SHRINEY_TURN','SHRINEY_WALK','SHRINEY_IDLE','SHRINEY_DIE','SHRINEY_ATTACK','SHRINEY_HURT',
        'SHRINE','SHRINE_HIT','SHRINE_TURN','SHRINE_WALK','SHRINE_IDLE','SHRINE_DIE','SHRINE_ATTACK','SHRINE_HURT','SHRINE_SLAM','SHRINE_YELL',
        'GUARD','GUARD_HIT','GUARD_TURN','GUARD_WALK','GUARD_IDLE','GUARD_DIE','GUARD_ATTACK','GUARD_HURT','GUARD_SLAM','GUARD_YELL',
        'JOE','JOE_HIT','JOE_TURN','JOE_WALK','JOE_IDLE','JOE_DIE','JOE_ATTACK','JOE_HURT','JOE_ROLL','JOE_BOUNCE',
        'JHJ','JHJ_HIT','JHJ_TURN','JHJ_WALK','JHJ_IDLE','JHJ_DIE','JHJ_ATTACK','JHJ_HURT','JHJ_ROLL','JHJ_BOUNCE',
    ],
    'FX_PICKUP': [
        'PHOENIX','GLIDEY','SHIELD','FARTHEAD','SUPER_WILLIE','ONE_UP','1UP','GROW','SHRINK',
        'UNIVERSE_ENEMA','1970','1970_03','HAMSTER','HAMSTER_03','SLAPPY','SLAPPY_HAMSTER',
        'BULLETS','GREEN_BULLETS','SPARKLE','GEM','GEMS','BLUE_GEM','GREEN_GEM','ORB','POW',
        'PLAY','CHIME','BELL','JINGLE','MIDI','TONE','TWANG','TICK','POP','BLIP',
        'WINGS','WING','WINGED','FLAPPER','FEATHER','HORN','PHART','FART','EXTRA','EXTRAS',
        'CLAYBALL','CLAY','TOKEN','TOKEN_01','TOKEN_02','TOKEN_03',
        'POWERUP','POWER','SUPER','MEGA','BIG','GIANT','HUGE','BOOST','EXTEND','PUMP',
        'BONUS','BIG_BONUS','HEAL','HEAL_01','HEAL_02','HEAL_03',
    ],
    'FX_BIRD': [
        'FLY','FLY_01','FLY_02','FLY_03','FLAP','FLAP_01','FLAP_02',
        'SQUISH','SQUISH_01','SQUISH_02','SQUASH','SCREAM','SCREAM_01',
        'CHIRP','SING','TWEET','PECK','DIVE','SOAR',
        'DIE','DIE_01','DIE_02','HURT','LAND','TAKEOFF',
    ],
    'FX_RAT': [
        'DASH','DASH_END','DASH_START','RUN','RUN_END','SCURRY','SQUEAK','CHATTER',
        'DIE','HURT','BITE','EAT','HISS','SQUEAL',
    ],
    'FX_MENU': [
        'SELECT','CONFIRM','CANCEL','BACK','PAUSE','RESUME','UP','DOWN','LEFT','RIGHT',
        'NAV','NAV_UP','NAV_DOWN','NAV_LEFT','NAV_RIGHT','SCROLL','BLIP','PRESS','TICK',
        'PASSWORD','UNLOCK','LOCK','WRONG','HURT','OK','BAD','GOOD','WAKE','START',
        'OPEN','CLOSE','ENTER','EXIT','BEEP','BUZZ','TONE','CHIME','BELL',
    ],
    'FX_GUM': [
        'PIERCE','PIERCE_DN','PIERCE_UP','PIERCE_LEFT','PIERCE_RIGHT',
        'POP','POP_01','POP_02','CHEW','BURST','BUBBLE','BUBBLE_01','BUBBLE_02',
        'STRETCH','SNAP','SPIT','SPLAT','SQUISH',
    ],
    'FX_FINN': [
        'DIE','DIE_1','DIE_2','DIE_3','DIE_4','DIE_5','DIE_6',
        'HURT','HIT','LAND','JUMP','FALL','WALK','RUN','IDLE',
        'ATTACK','TAUNT','LAUGH','CRY','SCREAM','YELL',
        'PUFF','PUFF_FALL','PUFF_FALL_1','PUFF_FALL_2','PUFF_FALL_3','PUFF_FALL_4',
        'DEFLATE','INFLATE','BLOAT','FLOAT','BOUNCE',
    ],
    'FX_PUFF': [
        'FALL','FALL_1','FALL_2','FALL_3','FALL_4','FALL_5',
        'RISE','UP','UP_1','UP_2','UP_3','POP','POP_1','POP_2','POP_3',
        'PUFF','PUFFER','GASP','HUFF','BLOAT',
    ],
}

# === Try all combinations of <FAMILY>_<SUFFIX> ===
hits = []
for family, suffixes in families.items():
    for suffix in suffixes:
        name = family + '_' + suffix
        h = calcHash(name)
        if h in unknowns:
            hits.append((name, h, unknowns[h]))

print('Tested %d names, found %d hits:' % (sum(len(v) for v in families.values()), len(hits)))
for n, h, (lv, fl) in hits:
    print('  0x%08x  %-50s  levels=%s  popc=%d' % (h, n, lv, fl))
