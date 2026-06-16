"""Brute exhaustively for HEAD-only audio. These are boss-related sounds — try every plausible verb."""
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

targets = {0x302016c4, 0x502840c3, 0x662096e9, 0x70a09ce1, 0xd0cc5442,
           0xf03391e9, 0xf033a1e9, 0xf2810224}

# Massive verb list
VERBS = ['HIT','TURN','WALK','IDLE','JUMP','LAND','LANDING','FALL','RUN','BLINK','DIE','DEATH','DEAD',
         'HURT','OUCH','PAIN','GRAB','SHOOT','SHOOTING','LAUGH','SCREAM','CRY','GASP','GIGGLE',
         'HOWL','GROAN','MOAN','BAWL','SOB','FART','BURP','SAY','SHOUT','TALK','SING',
         'EAT','BITE','CHOMP','CHEW','SPIT','SLURP','GULP','MUNCH','BREATH','BREATHE','BLOW','PUFF',
         'SLEEP','WAKE','SNORE','REST','SIT','BEND','LOOK','POINT','SPIN','BOUNCE','BOB','ROLL',
         'SHAKE','TWIST','SWAY','CLIMB','SWIM','DUCK','SLIDE','STAND','PEEK','HIDE',
         'STOMP','KICK','PUNCH','SLASH','SWING','BLOCK','DODGE','DASH','CHARGE','RECOIL',
         'STAGGER','STUMBLE','REVIVE','RESPAWN','HEAL','POISON','BURN','MELT','FREEZE','SHOCK',
         'GLOW','SHINE','FADE','DEFEAT','VICTORY','WIN','LOSE','SUCCESS','FAIL','PASS',
         'BUMP','POP','PLOP','PLUNK','SPLAT','SPLASH','BANG','BOOM','BLAST','THUD','THUMP','TINK','TONK',
         'CLANG','SLAM','SMACK','SLAP','SHOOM','WHOOSH','WHIZZ','VROOM','SQUEAK','SQUEAL','SQUELCH','SQUISH',
         'BLEEP','BEEP','BUZZ','BLIP','PING','DING','BELL','CHIRP','HOOT','BARK','MEOW','MOO','OINK',
         'CRACKLE','SIZZLE','HISS','RUSTLE','RUMBLE','WHIRR','HONK','BLARE','TOOT','HORN','TUBA',
         'CHURN','GRIND','CRUNCH','SNAP','WHACK','TAP','TICK','CLICK','CLUNK','DROP','RELEASE','LIFT',
         'CARRY','HOLD','PICKUP','THROW','TOSS','HURL','LAUNCH','FIRE','EXPLODE','BURST','SHATTER',
         'GHOST','SOUL','PHANTOM','SHADOW','MIRROR',
         'INTRO','OUTRO','START','END','FINISH','BEGIN','READY','GO','SET','RESET',
         'OPEN','CLOSE','ENTER','EXIT','EXPLODING','CHARGE','CHARGING','RAGE','RAGING',
         'PRAY','CHANT','HOSANNA','HALLELUJAH','AMEN',
         'AH','UH','EH','OH','HM','UM','HUH','OW','OUCH','MEH','HEY','YO',
         'HUMP','RUMP','HEAD','BUTT','BELLY','PAW','FEET','FOOT',
         'BLAH','GROSS','YUCK','EW','UGH','OOPS','WHOA','WOW','YEAH','UH_OH',
         # boss-specific
         'TRANSFORM','MORPH','EMERGE','APPEAR','VANISH','DISAPPEAR','TELEPORT',
         'BOSS_HIT','BOSS_DIE','BOSS_INTRO','BOSS_OUTRO','BOSS_RAGE',
         'CHARGED','LOADING','UNLEASHED','UNLEASH',
         'ROAR','ROARING','BELLOW','GROWL','GROWLING','SNARL',
         'HEADBUTT','SLAM','POUND','POUNDING','SMASH','SMASHING','RAM','RAMMING',
         'THINK','THINKING','PONDER','PLAN','SCHEME',
         'HEALTH','HP','LIFE','BAR','LOWHEALTH','CRITICAL','RAGE','POWER',
         'SPAWN','SUMMON','CALL','MINIONS',
         'WALKING','RUNNING','FALLING','JUMPING','LANDING','TURNING','LOOKING','POINTING',
         'TALKING','SHOUTING','LAUGHING','CRYING','SCREAMING','GASPING','GIGGLING',
         'EATING','BITING','CHEWING','CHOMPING','SPITTING','SLURPING','GULPING','MUNCHING',
         'BREATHING','HUFFING','PANTING','BLOWING','PUFFING','SIGHING','EXHALING','INHALING',
         'STAYING','HUMMING','SINGING','WHISTLING','WHIMPERING','WHINING','MUTTERING','MUMBLING',
         'PRAYING','CHANTING',
         'SITTING','BENDING','LOOKING','POINTING','WAVING','DANCING','SPINNING','BOUNCING',
         'TURNING','SHAKING','TWISTING','SWAYING','ROLLING',
         'CLIMBING','SWIMMING','DUCKING','SLIDING','STANDING','PEEKING','HIDING',
         'STOMPING','KICKING','PUNCHING','SWINGING','DODGING','DASHING','CHARGING',
         'STAGGERING','STUMBLING','REVIVING','RESPAWNING','HEALING','POISONING','BURNING',
         'MELTING','FREEZING','SHOCKING','GLOWING','SHINING','FADING',
         'WINNING','LOSING','SUCCEEDING','FAILING','PASSING','FAILING',
         'POPPING','PLOPPING','PLUNKING','SPLATTING','SPLASHING','BANGING','BOOMING','BLASTING',
         'THUDDING','THUMPING','SLAMMING','SMACKING','SLAPPING','VROOMING','SQUEAKING','SQUEALING',
         'BUZZING','PINGING','DINGING','CHIRPING','HOOTING','BARKING','MEOWING','MOOING','OINKING',
         'CRACKLING','SIZZLING','HISSING','RUSTLING','RUMBLING','WHIRRING','HONKING','BLARING',
         'TOOTING','GRINDING','CRUNCHING','SNAPPING','WHACKING','TAPPING','TICKING','CLICKING','CLUNKING',
         'DROPPING','RELEASING','LIFTING','CARRYING','HOLDING','PICKING','THROWING','TOSSING',
         'HURLING','LAUNCHING','FIRING','EXPLODING','BURSTING','SHATTERING',
         'STAGGERING','REVIVING','RESPAWNING','HEALING']

# Boss head subjects
ACTORS = ['BOSS','HEAD','BOSSHEAD','BOSS_HEAD','HEADBOSS','HEAD_BOSS','HEADJOE','HEAD_JOE',
          'BIG_HEAD','BIGHEAD','GIANT_HEAD','GIANTHEAD','MEGA_HEAD','MEGAHEAD',
          'JOEHEAD','JOE_HEAD','JOSEPH','JOE','JOYBOY','JIM','JIMHEAD',
          'MIKE','MICHAEL','MIKEHEAD','MIKE_HEAD',
          'SHISH','SHISHHEAD','SHISH_HEAD',
          'KLAYHEAD','KLAY_HEAD','KLAYMEN_HEAD','KMAN_HEAD']

PFXS = ['FX_','SFX_','SND_']

hits = {}
seen = set()
for pfx in PFXS:
    for actor in ACTORS:
        for v in VERBS:
            for sfx in ['','_1','_2','_3','_4','_5','_01','_02','_03','_LOOP','_END','_START','_MAIN',
                        '_BIG','_SM','_LG','_HI','_LO','_LEFT','_RIGHT','_UP','_DN','_A','_B','_C']:
                s = pfx + actor + '_' + v + sfx
                if s in seen: continue
                seen.add(s)
                h = calcHash(s)
                if h in targets:
                    hits.setdefault(h, []).append(s)
print(f'Tested {len(seen):,} unique strings, hits: {len(hits)}')
for h, names in sorted(hits.items()):
    print(f"  0x{h:08x}  popc={bin(h).count('1')} [{len(names)} cands]")
    for s in sorted(set(names), key=lambda x: (len(x), x))[:8]:
        print(f'      -> {s}')
    if len(names) > 8: print(f'      ... ({len(names)-8} more)')
