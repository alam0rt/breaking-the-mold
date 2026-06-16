"""Per-family targeted brute with high-quality verb dictionaries.
For each verified FX_X family root, hammer it with all VERB_NN combos and check uncracked."""
import csv

def calcHash(s):
    h, sh = 0, 0
    for ch in s:
        c = ord(ch)
        if 65 <= c <= 90: pass
        elif 97 <= c <= 122: c -= 32
        elif 48 <= c <= 57: c += 22
        else: continue
        sh = (sh + c - 64) & 31
        h ^= 1 << sh
    return h & 0xFFFFFFFF

allids = {}
with open('docs/analysis/asset-identification/cracked_names.csv') as f:
    for row in csv.DictReader(f):
        if row['id_hex'].startswith('0x'):
            allids[int(row['id_hex'],16)] = row

# Comprehensive verb list — sound-effect oriented
VERBS = [
    # Movement
    'JUMP','LAND','LANDING','FALL','FALLING','RUN','RUNNING','WALK','WALKING','IDLE','STAND','STANDING','BLINK',
    'CLIMB','CLIMBING','SWIM','SWIMMING','DUCK','DUCKING','SLIDE','SLIDING','SIT','BEND','BENDING',
    'TURN','TURNING','LOOK','POINT','POINTING','WAVE','WAVING','DANCE','DANCING','SPIN','SPINNING',
    'BOUNCE','BOUNCING','BOB','SHAKE','SHAKING','TWIST','SWAY','SWAYING','ROLL','ROLLING',
    'STOMP','STOMPING','KICK','KICKING','PUNCH','PUNCHING','SLASH','SWING','SWINGING','BLOCK','DODGE','DASH','DASHING',
    'CHARGE','CHARGING','RECOIL','STAGGER','STUMBLE','DAZE','REVIVE','RESPAWN','HEAL',
    'STEP','FOOTSTEP','FOOTFALL',
    # Damage states
    'DIE','DEATH','DEAD','HIT','HURT','OUCH','PAIN','HURTING','HITTING',
    'BURN','BURNING','MELT','FREEZE','SHOCK','POISON','SQUISH','SQUASH','SQUELCH',
    'EXPLODE','EXPLODING','BURST','BREAK','BREAKING','SHATTER',
    # Item interactions
    'GRAB','GRABBING','HOLD','HOLDING','PICKUP','PICK','PICKING',
    'THROW','THROWING','TOSS','TOSSING','HURL','HURLING','SHOOT','SHOOTING','FIRE','FIRING','LAUNCH','LAUNCHING',
    'CARRY','CARRYING','LIFT','LIFTING','DROP','DROPPING','RELEASE','RELEASING',
    # Sounds (vocal)
    'LAUGH','LAUGHING','SCREAM','SCREAMING','CRY','CRYING','YELL','YELLING','GASP','GASPING',
    'GIGGLE','GIGGLING','HOWL','HOWLING','GROAN','MOAN','BAWL','BAWLING','SOB','SOBBING',
    'FART','FARTING','BURP','BURPING','SNEEZE','SNEEZING','COUGH','COUGHING','SNORT','SNORTING',
    'BREATH','BREATHE','BREATHING','HUFF','PANT','PANTING','BLOW','BLOWING','PUFF','PUFFING',
    'SIGH','SIGHING','EXHALE','INHALE',
    'SAY','SHOUT','SHOUTING','TALK','TALKING','SING','SINGING','HUM','WHISTLE','WHIMPER','WHIMPERING',
    'WHINE','MUTTER','MUMBLE','MUMBLING','PRAY','CHANT','CHANTING','MOAN','HOWL',
    'YO','HEY','UH','HUH','OH','OW','OUCH','MEH','HUH',
    # Sounds (action)
    'EAT','EATING','BITE','BITING','CHOMP','CHEW','CHEWING','SPIT','SPITTING','GULP','GULPING',
    'SLURP','SLURPING','MUNCH','MUNCHING','BURP_REST',
    # Sounds (mechanical/environmental)
    'BUMP','BUMPING','POP','PLOP','PLUNK','SPLAT','SPLASH','SPLOOSH',
    'BANG','BOOM','BLAST','THUD','THUMP','TINK','TONK','CLANG','SLAM','SMACK','SLAP',
    'SHOOM','WHOOSH','WHIZZ','WHISH','WOOSH','VROOM','VROOMING',
    'SQUEAK','SQUEAL','SQUEEZE','BLEEP','BEEP','BUZZ','BLIP','PING','PONG','DING','DONG','BELL',
    'CHIRP','TWEET','HOOT','BARK','MEOW','MOO','BAA','OINK','OINKING',
    'SLOSH','SLOSHING','CRACKLE','SIZZLE','HISS','HISSING','RUSTLE','RUSTLING','RUMBLE','WHIRR',
    'HONK','HONKING','BLARE','TOOT','TOOTLE','HORN','TUBA','TROMBONE',
    'CHURN','GRIND','GRIT','CRUNCH','SNAP','SNAPPING',
    'WHACK','WALLOP','BOP','TAP','TICK','TOCK','CLICK','CLUNK',
    'WHIP','WHIPPING','SCRATCH','POKE','PROD','TICKLE',
    'FLAP','FLAPPING','FLUTTER','FLUTTERING','SOAR','DIVE','SWOOP','PERCH','LAND',
    'WIND','GUST','BREEZE','GALE','STORM','THUNDER','LIGHTNING',
    'WATER','RIPPLE','TIDE','OCEAN','SEA',
    # Game state
    'INTRO','OUTRO','START','END','FINISH','BEGIN','READY','GO','SET','RESET',
    'OPEN','OPENING','CLOSE','CLOSING','ENTER','ENTERING','EXIT','EXITING',
    'WIN','WINNING','LOSE','LOSING','VICTORY','DEFEAT','SUCCESS','FAIL','PASS','FAILURE',
    'CHECKPOINT','SAVE','LOAD','MENU','SELECT','CONFIRM','CANCEL','PAUSE',
    # Boss
    'BOSS_HIT','BOSS_DIE','BOSS_INTRO','BOSS_OUTRO',
    # Klaymen / monkey specific
    'WET','SOAKED','DRY','MUDDY','SLIMY','GOOEY','OILY','GREASY',
    'GHOST','SOUL','PHANTOM','SHADOW','MIRROR','GLOW','SHINE','FADE','LIT','DARK','DIM','BRIGHT',
    'BIG','SM','LG','TINY','HUGE','GIANT','SMALL','BIG_FAT',
    'HOT','COLD','BURN','MELT','FREEZE',
    # 1-2 letter / dirty
    'AH','UH','EH','OH','HM','UM',
    'HUMP','RUMP','HEAD','BUTT','BELLY','PAW','PAWS','FEET','FOOT',
    'BLAH','MEH','GROSS','YUCK','EW','UGH','OOPS','WHOA','WOW',
]

# Strong family roots
ROOTS = ['FX_KLAY_','FX_SKULL_','FX_FINN_','FX_PHART_','FX_JOPS_','FX_GLID_','FX_BIRD_','FX_RAT_',
         'FX_BOSS_','FX_GUARD_','FX_PIG_','FX_BUG_','FX_PUFF_','FX_NUN_','FX_GUM_','FX_BIT_',
         'FX_KLAYMEN_','FX_KMAN_','FX_PLAYER_','FX_HEAD_','FX_BOSS_HEAD_','FX_GUMBALL_','FX_GUMHEAD_',
         'FX_BIRDIE_','FX_BIRDY_','FX_RATTY_','FX_SKULLBOT_','FX_SKULLBALL_','FX_SKULLY_',
         'FX_GHOST_','FX_SOUL_','FX_SHRINE_','FX_SHRINEY_','FX_BUMP_','FX_BUMPER_',
         'FX_JOLOPS_','FX_JOPSIE_','FX_PHARTY_',
         'FX_BOMB_','FX_TNT_','FX_BARREL_','FX_BOX_','FX_CRATE_','FX_BLOCK_','FX_DOOR_','FX_GATE_',
         'FX_PORTAL_','FX_TELEPORT_','FX_SPIKE_','FX_SAW_','FX_BLADE_',
         'FX_PROJ_','FX_PROJECTILE_','FX_MISSILE_','FX_BULLET_','FX_BLAST_','FX_ROCKET_',
         'FX_CHASEY_','FX_CHASE_','FX_HUNTER_','FX_SCOUT_','FX_RUNNER_',
         'FX_WORM_','FX_SLUG_','FX_SNAKE_','FX_SPIDER_','FX_ROACH_','FX_ANT_','FX_BEE_','FX_WASP_',
         'FX_MONK_','FX_MONKS_','FX_NUNS_','FX_PRIEST_','FX_MAGE_','FX_WIZARD_','FX_KNIGHT_','FX_SOLDIER_',
         'FX_GOBLIN_','FX_TROLL_','FX_OGRE_','FX_BEAST_','FX_MONSTER_','FX_CREATURE_',
         'FX_BAT_','FX_RAT_','FX_MOTH_',
         'FX_FISH_','FX_FROG_','FX_TURTLE_',
         'FX_ALIEN_','FX_UFO_','FX_GREY_','FX_MARTIAN_',
         'FX_ROBOT_','FX_BOT_','FX_DROID_','FX_ANDROID_','FX_CYBORG_','FX_MECH_','FX_MACHINE_',
         'FX_GUM_','FX_GUMS_',
         # Without FX_ prefix
         'SFX_KLAY_','SFX_SKULL_','SFX_FINN_','SFX_BOSS_','SFX_PHART_','SFX_GLID_',
         'SND_KLAY_','SND_SKULL_','SND_FINN_','SND_BOSS_','SND_PHART_','SND_GLID_',]

SUFS = ['','_1','_2','_3','_4','_5','_6','_7','_8','_9',
        '_01','_02','_03','_04','_05','_06','_07','_08','_09','_10',
        '_11','_12','_13','_14','_15','_16','_17','_18','_19','_20',
        '_LOOP','_END','_START','_MAIN','_ALT','_BIG','_SM','_LG','_HI','_LO',
        '_LEFT','_RIGHT','_UP','_DN','_DOWN','_IN','_OUT','_ON','_OFF',
        '_A','_B','_C','_D','_E','_F','_G','_H','_I','_J',
        '1','2','3','4','5','6','7','8','9','01','02','03','A','B','C']

hits = {}
seen = set()
for pfx in ROOTS:
    for v in VERBS:
        for suf in SUFS:
            s = pfx + v + suf
            if s in seen: continue
            seen.add(s)
            h = calcHash(s)
            if h in allids and allids[h].get('status') == 'uncracked':
                hits.setdefault(h, []).append(s)
                
print(f'Tested {len(seen):,} unique strings, hits: {len(hits)}')
print()
print('=== Sister hits (sorted by candidate count, then ID) ===')
for h, names in sorted(hits.items(), key=lambda x: (len(x[1]), x[0])):
    row = allids[h]
    n = len(names)
    levels = row['levels'][:60]
    typ = row['type']
    print(f"  0x{h:08x}  popc={bin(h).count('1')} type={typ:12s} levels={levels} [{n} cands]")
    for s in sorted(set(names), key=lambda x: (len(x), x))[:6]:
        print(f'      -> {s}')
    if n > 6:
        print(f'      ... ({n-6} more)')
