"""Targeted brute focused exclusively on FX_ACTOR_VERB[_NN] for AUDIO unknowns.
Audio assets follow this proven convention. Filter to only audio unknowns to avoid noise."""
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

# Only audio uncracked
audio_unk = {}
with open('docs/analysis/asset-identification/cracked_names.csv') as f:
    for row in csv.DictReader(f):
        if row['id_hex'].startswith('0x') and row.get('status')=='uncracked' and row['type'] == 'audio':
            audio_unk[int(row['id_hex'],16)] = row
print(f'Audio unknowns: {len(audio_unk)}')

# Restrict to FX_ prefix only (proven for audio)
PREFIX = 'FX_'
ACTORS = ['KLAY','KLAYMEN','KMAN','SKULL','FINN','PHART','JOPS','JOLOPS','GLID',
          'BOSS','GUARD','BUG','BIRD','EGGS','PIG','CLAY','HEAD','MIKE','SHISH',
          'BIT','GUM','MONKEY','PLAYER','EVIL','SHRINEY','GUMBALL','JOE','JOYBOY',
          'EYEBALL','BABY','SOUL','GHOST','SPIRIT','SHRINE',
          'CLAYBALL','PROJ','PROJECTILE','MISSILE','ENEMY','GUM','GUMHEAD','HEADJOE',
          'BUMP','BUMPER','BUMP_GUARD','PIGGY','BIRDIE','BIRDY','MOTH','BAT','RAT',
          'WORM','SLUG','SNAKE','SPIDER','ROACH','ANT','BEE','WASP',
          'NUN','NUNS','MONK','MONKS','PRIEST','MAGE','WIZARD',
          'KNIGHT','SOLDIER','TROOP','GOBLIN','TROLL','OGRE',
          'BARREL','BOX','CRATE','BOMB','TNT','MINE','TRAP','SAW','BLADE',
          'WHIP','CHAIN','HOOK','SPIKE','POINT','PEAK','EDGE',
          'GHOST','SOUL','PHANTOM','SPECTER','SHADE','SHADOW','MIRROR',
          'PROJ','PROJECTILE','MISSILE','BULLET','BLAST','ROCKET','BOMB',
          'GUMHEAD','GUMHEADS','HEAD_JOE','HEADJOE','JOE_HEAD','JOEHEAD',
          'CHARACTER','HUMAN','MONSTER','BEAST','CREATURE',
          'JIGGLY','JIGGLY_NUN','PRIEST','SHARD','BURP_GUARD',
          'CHASEY','CHASE','HUNTER','SCOUT','RUNNER',
          'ROCK','BOMB','EXPLODE','SMOKE','FIRE','FLAME','SPARK',
          'FATTY','THIN','TALL','SHORT','SCRAWNY','BUFF',
          'PARENT','MOTHER','FATHER','PA','MA',
          'GROUP','TEAM','SQUAD','PACK','SWARM','HORDE',
          'BUMPER','BLOAT','VENT','PUFF','BLOW','GUST',
          'ALIEN','UFO','MARTIAN','GREY','GREYS','SPACER',
          'ROBOT','MECH','BOT','DROID','ANDROID','CYBORG','MACHINE',
          'BOSS_FINN','BOSS_PHART','BOSS_HEAD','BOSS_JOE','BOSS_BIG',
          'CHEEKY','CHEEK','BUNS','BACON','HAM','PIG','PORK',
          'STAGE','SCENE','LEVEL','ROOM','MAP',
          'CARRY','CARRYBALL','CARRYING','CARRIED']

VERBS = ['JUMP','LAND','LANDING','FALL','RUN','WALK','IDLE','STAND','BLINK',
         'DIE','DEATH','DEAD','HIT','HURT','OUCH','PAIN',
         'GRAB','HOLD','PICKUP','PICK','THROW','TOSS','HURL','SHOOT','FIRE','LAUNCH',
         'EAT','BITE','CHOMP','CHEW','SPIT',
         'LAUGH','SCREAM','CRY','YELL','GASP','GIGGLE','HOWL','GROAN','MOAN','BAWL',
         'FART','BURP','SNEEZE','COUGH','SNORT','BREATH','BREATHE','HUFF','PANT',
         'BLOW','PUFF','SIGH','EXHALE','INHALE',
         'SWIM','CLIMB','DUCK','SLIDE','SIT','BEND','TURN','LOOK','POINT',
         'WAVE','DANCE','SPIN','BOUNCE','BOB','SHAKE','TWIST','SWAY','ROLL',
         'SLEEP','WAKE','SNORE','REST','BURP_REST','BURP_AWAKE',
         'FLY','GLIDE','SAIL','FLOAT','HOVER','DROP','LIFT','CARRY',
         'STOMP','KICK','PUNCH','SLASH','SWING','BLOCK','DODGE','DASH',
         'CHARGE','RECOIL','STAGGER','STUMBLE','DAZE','REVIVE','RESPAWN','HEAL',
         'POISON','BURN','FREEZE','SHOCK','GLOW','FADE','SHINE',
         'WIN','LOSE','SUCCESS','FAIL','VICTORY','DEFEAT',
         'BUMP','POP','PLOP','PLUNK','PLOOSH','SPLOOSH','SPLAT','SPLASH',
         'BANG','BOOM','SHOOM','WHOOSH','WHIZZ','THUD','THUMP','TINK','TONK','CLANG',
         'INTRO','OUTRO','START','END','FINISH','BEGIN',
         'OPEN','CLOSE','ENTER','EXIT','EXPLODE','BREAK','SHATTER',
         'WET','DRY','HOT','COLD','BIG','SMALL','TINY','HUGE',
         'STEP','FOOTSTEP','FOOTFALL',
         'SQUEAK','SQUEAL','SQUEEZE','SQUISH','SQUASH','SQUELCH',
         'BLEEP','BEEP','BUZZ','BLIP','PING','PONG','DING','DONG','BELL',
         'CHIRP','TWEET','HOOT','CROW','BARK','MEOW','MOO','BAA','OINK',
         'SLURP','GULP','MUNCH','SLAM','SLOSH','GULP',
         'CRACKLE','SIZZLE','HISS','RUSTLE','RUMBLE','WHIRR',
         'HONK','BLAST','BLARE','TOOT','TOOTLE','TUBA','TROMBONE','HORN',
         'CHURN','GRIND','GRIT','CRUNCH','SNAP',
         'WHISH','WHIRL','WHIP','WHACK','WALLOP','BOP','BUMP',
         'TAP','TICK','TOCK','CLICK','CLUNK','CLANG',
         'SAY','SHOUT','TALK','SING','HUM','WHISTLE','WHIMPER','WHINE',
         'YO','HEY','WHA','UH','HUH','OH','OW','OUCH',
         'PRAY','CHANT','GLORY','HOSANNA','HALLELUJAH','AMEN',
         'STIR','POKE','PROD','TICKLE','SCRATCH',
         'FLAP','FLUTTER','SOAR','DIVE','SWOOP','PERCH','LAND',
         'WIND','GUST','BREEZE','GALE','STORM','THUNDER','LIGHTNING',
         'WATER','SPLASH','RIPPLE','WAVE','TIDE','OCEAN','SEA',
         'METAL','WOOD','STONE','GLASS','SLIME','GOO','MUD','OOZE',
         'EXPLODE','BURST','POP','BLAST','BOOM','BANG','CRASH','SMASH',
         'PIERCE','STAB','SHOT','SHOTBOUNCE',
         'SHOTGUN','MACHINEGUN','PISTOL','LASER','PLASMA','RAY',
         'POWER','UP','DOWN','ON','OFF','START','RESET',
         'SAVE','LOAD','MENU','SELECT','CONFIRM','CANCEL']

MODS = ['','1','2','3','4','5','6','7','8','9','01','02','03','04','05','06','07','08','09','10',
        '11','12','13','14','15','16','17','18','19','20',
        'A','B','C','D','E','F','G','H','I','J',
        'BIG','SM','LG','HI','LO','LEFT','RIGHT','UP','DN','DOWN','IN','OUT','ON','OFF',
        'LOOP','START','END','MAIN','ALT','EXTRA','BONUS','TOP','BOT','BOTTOM',
        'NORTH','SOUTH','EAST','WEST','LIT','DARK','DIM','BRIGHT',
        'CRY','HAPPY','SAD','MAD','MEH','OK','GOOD','BAD']

hits = {}
seen = set()
for actor in ACTORS:
    for verb in VERBS:
        base = PREFIX + actor + '_' + verb
        for mod in [''] + MODS:
            s = base + ('_'+mod if mod else '')
            if s in seen: continue
            seen.add(s)
            h = calcHash(s)
            if h in audio_unk:
                hits.setdefault(h, []).append(s)
        # also without underscore in suffix
        for mod in MODS:
            if mod:
                s = base + mod
                if s in seen: continue
                seen.add(s)
                h = calcHash(s)
                if h in audio_unk:
                    hits.setdefault(h, []).append(s)

print(f'Tested {len(seen):,} unique strings, hits: {len(hits)} audio IDs')

print()
print('=== Audio FX_ hits ===')
for hh, names in sorted(hits.items(), key=lambda x: (len(x[1]), x[0])):
    row = audio_unk[hh]
    print(f"  0x{hh:08x}  popc={bin(hh).count('1')} levels={row['levels'][:60]} [{len(names)} cands]")
    for n in sorted(set(names), key=lambda s: (len(s), s))[:6]:
        print(f'      -> {n}')
    if len(names) > 6:
        print(f'      ... ({len(names)-6} more)')

# Save
with open('/tmp/audio_fx_hits.csv','w') as f:
    f.write('id_hex,name,levels,n_cands\n')
    for hh, names in sorted(hits.items()):
        row = audio_unk[hh]
        for n in sorted(set(names)):
            f.write(f"0x{hh:08x},{n},{row['levels'][:60]},{len(set(names))}\n")
