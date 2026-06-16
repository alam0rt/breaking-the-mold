"""Massive FX_ enumeration. Targets audio uncracked. Reports all hits per ID with
cohort-fitness scoring to help select the best candidate.

Vocabulary: ~100 actors, ~250 verbs, ~70 modifiers. Brute generates roughly
hundreds of millions of unique strings via FX_<ACTOR>_<VERB>[_<MOD>] templates plus
no-underscore and reversed orders.
"""
import csv
import sys
import os
from collections import defaultdict


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


# ---- Load targets and metadata ----
audio_unk = {}
with open('docs/analysis/asset-identification/cracked_names.csv') as f:
    for row in csv.DictReader(f):
        if row['id_hex'].startswith('0x') and row.get('status') == 'uncracked' and row['type'] == 'audio':
            audio_unk[int(row['id_hex'], 16)] = row

# Single-level cohort hints (which actors live where)
# Levels with their known character/enemy associations from docs:
LEVEL_ACTORS = {
    'BOIL':    ['SKULL', 'BUG', 'PIG', 'GUM', 'FATTY', 'ENEMY'],
    'BRG1':    ['SKULL', 'RAT', 'BIRD', 'SPIDER', 'ENEMY'],
    'CAVE':    ['SKULL', 'BUG', 'PIG', 'WORM', 'BAT', 'ENEMY'],
    'CLOU':    ['SKULL', 'BIRD', 'CLOUD', 'ANGEL', 'ENEMY'],
    'CRYS':    ['SKULL', 'CRYSTAL', 'GEM', 'ENEMY'],
    'CSTL':    ['SKULL', 'GUARD', 'KNIGHT', 'CASTLE', 'GHOST', 'ENEMY'],
    'EGGS':    ['SKULL', 'EGG', 'CHICK', 'BIRD', 'ENEMY'],
    'EVIL':    ['SKULL', 'EVIL', 'BIG_FAT', 'BIGFAT', 'BAD', 'BOSS', 'JOPS', 'JOLOPS', 'ENEMY'],
    'FINN':    ['FINN', 'BABY', 'PUFF', 'PHART', 'BOSS', 'BUMP'],
    'FOOD':    ['SKULL', 'GUM', 'EAT', 'FOOD', 'BURGER', 'PIE', 'ENEMY'],
    'GLEN':    ['GLEN', 'TREE', 'FOREST', 'MOSS', 'OAK', 'ENEMY'],
    'GLID':    ['SKULL', 'BIRD', 'GLIDER', 'GLID', 'WIND', 'ENEMY'],
    'HEAD':    ['BOSS_HEAD', 'BOSSHEAD', 'HEAD', 'BIG_HEAD', 'BIGHEAD', 'JOEHEAD', 'JOE'],
    'KLOG':    ['KLOG', 'KLOGG', 'LOG', 'TREE', 'TIMBER', 'WOOD', 'AXE', 'CUTTER'],
    'MEGA':    ['GUM', 'MEGA', 'BOSS', 'MEGAGUM', 'GUMBOSS', 'BIG_GUM'],
    'MENU':    ['UI', 'MENU', 'CURSOR', 'SELECT', 'CONFIRM', 'CANCEL', 'PAUSE'],
    'MOSS':    ['SKULL', 'MOSS', 'TREE', 'GREEN', 'ENEMY'],
    'PHRO':    ['SKULL', 'PHRO', 'PHARAOH', 'EGYPT', 'MUMMY', 'ENEMY'],
    'RUNN':    ['RUNN', 'RUNNER', 'CHASE', 'CHASEY', 'SPIKE', 'TRAIN'],
    'SCIE':    ['SKULL', 'SCIE', 'SCIENCE', 'LAB', 'ROBOT', 'ENEMY'],
    'SEVN':    ['SKULL', 'SEVN', 'SEVEN', '7', 'ENEMY'],
    'SNOW':    ['SKULL', 'SNOW', 'SNOWMAN', 'ICE', 'PENGUIN', 'ENEMY'],
    'SOAR':    ['BIRD', 'SOAR', 'SKY', 'CLOUD', 'WIND', 'KLAY'],
    'TMPL':    ['SKULL', 'TMPL', 'TEMPLE', 'STONE', 'IDOL', 'GOLD', 'BAT', 'ENEMY'],
    'WEED':    ['SKULL', 'WEED', 'PLANT', 'FLOWER', 'BUG', 'ENEMY'],
    'WIZZ':    ['WIZZ', 'WIZARD', 'MAGE', 'MAGIC', 'SPELL', 'WHIZ', 'TINGLE', 'BUBBLE', 'BOSS'],
}

ACTORS = sorted(set(
    ['KLAY', 'KLAYMEN', 'KMAN', 'PLAYER', 'KMANBABY', 'BABY', 'KIDLY']
    + ['SKULL', 'SKULLBOT', 'SKULLY', 'SKULLBALL', 'SKULLBOY']
    + ['FINN', 'FINNY', 'BABYFINN']
    + ['PHART', 'PHARTY', 'PHARTYBOY', 'PHARTYTOOT']
    + ['JOPS', 'JOLOPS', 'JOPSIE']
    + ['GLID', 'GLIDER', 'GLIDE', 'GLIDY']
    + ['BOSS', 'BOSSHEAD', 'HEADBOSS', 'BIG_HEAD', 'BIGHEAD', 'HEAD', 'JOEHEAD', 'JOE_HEAD', 'JOE', 'JOYBOY']
    + ['BIRD', 'BIRDIE', 'BIRDY', 'CHIRP']
    + ['RAT', 'RATTY', 'MOUSE']
    + ['GUM', 'GUMBALL', 'GUMHEAD', 'GUMS', 'BIG_GUM', 'BIGGUM', 'MEGAGUM', 'MEGA_GUM']
    + ['BUG', 'BUGS', 'BEETLE', 'ROACH', 'ANT', 'BEE', 'WASP', 'SPIDER', 'WORM', 'SLUG', 'CATERPILLAR', 'CAT']
    + ['PIG', 'PIGGY', 'BACON', 'HAM', 'PORK']
    + ['GUARD', 'GUARDIAN', 'KNIGHT', 'SOLDIER', 'TROOP']
    + ['NUN', 'NUNS', 'MONK', 'MONKS', 'PRIEST', 'POPE', 'CARDINAL']
    + ['MAGE', 'WIZARD', 'WIZ', 'WHIZ', 'WIZZ', 'MAGIC', 'MAGICIAN', 'WIZBOSS']
    + ['GOBLIN', 'TROLL', 'OGRE', 'IMP', 'DEMON', 'DEVIL']
    + ['MIKE', 'SHISH', 'KEN', 'TIM', 'TOM', 'JIM', 'BOB', 'JOSEPH']
    + ['BIT', 'BITTY', 'BITS', 'CHIP', 'PIECE', 'PARTICLE']
    + ['SHRINEY', 'SHRINE', 'SHRINEYS', 'SHRINER']
    + ['MONKEY', 'MONKEYS', 'CHIMP', 'APE', 'GORILLA']
    + ['ALIEN', 'UFO', 'GREY', 'MARTIAN', 'SPACER']
    + ['ROBOT', 'ROBOTS', 'BOT', 'DROID', 'CYBORG', 'MECH', 'MACHINE']
    + ['ROCK', 'STONE', 'BRICK', 'BOULDER']
    + ['BARREL', 'BOX', 'CRATE', 'BOMB', 'TNT', 'MINE']
    + ['SPIKE', 'SAW', 'BLADE', 'TRAP', 'HOOK', 'CHAIN', 'WHIP']
    + ['DOOR', 'GATE', 'PORTAL', 'TELEPORT', 'WARP']
    + ['PROJ', 'PROJECTILE', 'MISSILE', 'BULLET', 'BLAST', 'ROCKET']
    + ['CHASEY', 'CHASE', 'HUNTER', 'SCOUT', 'STALKER', 'RUNNER']
    + ['PUFF', 'PUFFY', 'PUFFER', 'POOFY']
    + ['GHOST', 'SOUL', 'PHANTOM', 'SPECTER', 'SHADE', 'SHADOW', 'WRAITH']
    + ['WORM', 'GRUB', 'CATERPILLAR', 'LARVA', 'MAGGOT']
    + ['BAT', 'BATTY', 'NIGHT', 'BATWING']
    + ['FROG', 'TOAD', 'NEWT', 'LIZARD']
    + ['FISH', 'FISHY', 'EEL', 'CRAB', 'OCTOPUS']
    + ['SNAKE', 'SERPENT', 'WYRM', 'NAGA']
    + ['SCORPION', 'CENTIPEDE', 'MILLIPEDE']
    + ['KLOG', 'KLOGG', 'LOG', 'LOGGY', 'TIMBER', 'AXE', 'CUTTER']
    + ['CLAYBALL', 'BALL', 'CLAY', 'CLAYMAN']
    + ['CRATE', 'BOX', 'CHEST']
    + ['PARENT', 'MOTHER', 'FATHER', 'PA', 'MA']
    + ['FATTY', 'FAT', 'BIG_FAT', 'BIGFAT', 'CHUBBY', 'CHUBS']
    + ['BOSS', 'BOSSY', 'BOSSMAN']
    + ['BUMPER', 'BUMP', 'BUMPS']
    + ['SHRINER', 'SHRINER_NUN', 'NUNGUARD']
    + ['ROOM', 'STAGE', 'SCENE', 'AREA', 'ZONE']
    + ['LEVEL', 'LVL', 'WORLD', 'MAP']
    + ['GUMSHOT', 'SHOT', 'PIERCE', 'STAB', 'SLASH', 'SLICE']
    + ['SHIPMASTER', 'SHIP', 'BOAT', 'RAFT', 'TRAIN', 'TRACK']
    + ['MAGIC', 'SPELL', 'CHARM', 'POTION']
    + ['EVIL', 'BAD', 'VILLAIN', 'BOSS', 'CHIEF', 'LEADER']
    + ['MOTH', 'BUTTERFLY', 'FLUTTER']
    + ['SPRING', 'BOUNCE', 'LAUNCHER']
    + list({a for L in LEVEL_ACTORS for a in LEVEL_ACTORS[L]})
))

# Comprehensive verb list — sound-effect oriented
VERBS = sorted(set(
    # Movement
    ['JUMP', 'LAND', 'LANDING', 'FALL', 'FALLING', 'RUN', 'RUNNING', 'WALK', 'WALKING',
     'IDLE', 'STAND', 'STANDING', 'BLINK',
     'CLIMB', 'CLIMBING', 'SWIM', 'SWIMMING', 'DUCK', 'DUCKING', 'SLIDE', 'SLIDING',
     'SIT', 'BEND', 'BENDING', 'TURN', 'TURNING', 'LOOK', 'POINT', 'POINTING',
     'WAVE', 'WAVING', 'DANCE', 'DANCING', 'SPIN', 'SPINNING',
     'BOUNCE', 'BOUNCING', 'BOB', 'SHAKE', 'SHAKING', 'TWIST', 'SWAY', 'SWAYING',
     'ROLL', 'ROLLING', 'STEP', 'FOOTSTEP', 'FOOTFALL', 'STRIDE', 'TIPTOE',
     'STOMP', 'STOMPING', 'KICK', 'KICKING', 'PUNCH', 'PUNCHING',
     'SLASH', 'SWING', 'SWINGING', 'BLOCK', 'DODGE', 'DASH', 'DASHING',
     'CHARGE', 'CHARGING', 'RECOIL', 'STAGGER', 'STUMBLE', 'DAZE',
     'REVIVE', 'RESPAWN', 'HEAL', 'CRAWL', 'CREEP', 'CRAWLING',
     'TODDLE', 'WADDLE', 'WIGGLE', 'SHIMMY']
    # Damage
    + ['DIE', 'DEATH', 'DEAD', 'HIT', 'HURT', 'OUCH', 'PAIN', 'HURTING', 'HITTING',
       'BURN', 'BURNING', 'MELT', 'FREEZE', 'SHOCK', 'POISON', 'SQUISH', 'SQUASH', 'SQUELCH',
       'EXPLODE', 'EXPLODING', 'BURST', 'BREAK', 'BREAKING', 'SHATTER', 'CRACK',
       'KILL', 'KILLED', 'DEFEAT', 'DEFEATED']
    # Item interactions
    + ['GRAB', 'GRABBING', 'HOLD', 'HOLDING', 'PICKUP', 'PICK', 'PICKING',
       'THROW', 'THROWING', 'TOSS', 'TOSSING', 'HURL', 'HURLING',
       'SHOOT', 'SHOOTING', 'FIRE', 'FIRING', 'LAUNCH', 'LAUNCHING',
       'CARRY', 'CARRYING', 'LIFT', 'LIFTING', 'DROP', 'DROPPING',
       'RELEASE', 'RELEASING', 'COLLECT', 'COLLECTING', 'GET', 'GOT']
    # Vocal
    + ['LAUGH', 'LAUGHING', 'SCREAM', 'SCREAMING', 'CRY', 'CRYING',
       'YELL', 'YELLING', 'GASP', 'GASPING',
       'GIGGLE', 'GIGGLING', 'HOWL', 'HOWLING', 'GROAN', 'MOAN', 'BAWL', 'SOB',
       'FART', 'FARTING', 'BURP', 'BURPING', 'SNEEZE', 'SNEEZING',
       'COUGH', 'COUGHING', 'SNORT', 'SNORTING',
       'BREATH', 'BREATHE', 'BREATHING', 'HUFF', 'PANT', 'PANTING',
       'BLOW', 'BLOWING', 'PUFF', 'PUFFING',
       'SIGH', 'SIGHING', 'EXHALE', 'INHALE',
       'SAY', 'SHOUT', 'SHOUTING', 'TALK', 'TALKING', 'SING', 'SINGING',
       'HUM', 'WHISTLE', 'WHIMPER', 'WHIMPERING', 'WHINE', 'MUTTER', 'MUMBLE',
       'PRAY', 'CHANT', 'CHANTING', 'SQUEAL',
       'YO', 'HEY', 'UH', 'HUH', 'OH', 'OW', 'OUCH', 'MEH', 'WHA',
       'ROAR', 'ROARING', 'BELLOW', 'GROWL', 'GROWLING', 'SNARL']
    # Action sounds
    + ['EAT', 'EATING', 'BITE', 'BITING', 'CHOMP', 'CHEW', 'CHEWING',
       'SPIT', 'SPITTING', 'GULP', 'GULPING',
       'SLURP', 'SLURPING', 'MUNCH', 'MUNCHING']
    # Mechanical / environmental
    + ['BUMP', 'BUMPING', 'POP', 'PLOP', 'PLUNK', 'SPLAT', 'SPLASH', 'SPLOOSH',
       'BANG', 'BOOM', 'BLAST', 'THUD', 'THUMP', 'TINK', 'TONK', 'CLANG',
       'SLAM', 'SMACK', 'SLAP', 'WHACK', 'WALLOP',
       'SHOOM', 'WHOOSH', 'WHIZZ', 'WHISH', 'WOOSH', 'VROOM',
       'SQUEAK', 'SQUEAL', 'SQUEEZE',
       'BLEEP', 'BEEP', 'BUZZ', 'BLIP', 'PING', 'PONG', 'DING', 'DONG', 'BELL',
       'CHIRP', 'TWEET', 'HOOT', 'BARK', 'MEOW', 'MOO', 'BAA', 'OINK',
       'SLOSH', 'SLOSHING', 'CRACKLE', 'SIZZLE', 'HISS', 'HISSING',
       'RUSTLE', 'RUSTLING', 'RUMBLE', 'WHIRR', 'WHIRRING',
       'HONK', 'HONKING', 'BLARE', 'TOOT', 'TOOTLE', 'HORN', 'TUBA', 'TROMBONE',
       'CHURN', 'GRIND', 'GRIT', 'CRUNCH', 'SNAP', 'SNAPPING',
       'WHIP', 'WHIPPING', 'SCRATCH', 'POKE', 'PROD', 'TICKLE',
       'TAP', 'TICK', 'TOCK', 'CLICK', 'CLUNK',
       'FLAP', 'FLAPPING', 'FLUTTER', 'FLUTTERING', 'SOAR', 'DIVE', 'SWOOP',
       'WIND', 'GUST', 'BREEZE', 'GALE', 'STORM', 'THUNDER', 'LIGHTNING',
       'WATER', 'RIPPLE', 'TIDE', 'OCEAN', 'SEA', 'PIERCE', 'STAB']
    # Game state
    + ['INTRO', 'OUTRO', 'START', 'END', 'FINISH', 'BEGIN',
       'READY', 'GO', 'SET', 'RESET', 'WAIT',
       'OPEN', 'OPENING', 'CLOSE', 'CLOSING', 'ENTER', 'ENTERING',
       'EXIT', 'EXITING', 'WIN', 'WINNING', 'LOSE', 'LOSING',
       'VICTORY', 'DEFEAT', 'SUCCESS', 'FAIL', 'PASS', 'FAILURE',
       'CHECKPOINT', 'CKPT', 'SAVE', 'LOAD', 'SELECT', 'CONFIRM', 'CANCEL', 'PAUSE',
       'TRANSFORM', 'MORPH', 'EMERGE', 'APPEAR', 'VANISH', 'DISAPPEAR', 'TELEPORT']
    # Misc / states
    + ['WET', 'SOAKED', 'DRY', 'MUDDY', 'SLIMY', 'GOOEY', 'OILY', 'GREASY',
       'GLOW', 'SHINE', 'FADE', 'LIT', 'DARK', 'DIM', 'BRIGHT',
       'BIG', 'SM', 'LG', 'TINY', 'HUGE', 'GIANT', 'SMALL', 'LARGE',
       'HOT', 'COLD',
       'AH', 'UH', 'EH', 'OH', 'HM', 'UM',
       'HUMP', 'RUMP', 'BUTT', 'BELLY', 'PAW', 'PAWS', 'FEET', 'FOOT',
       'BLAH', 'GROSS', 'YUCK', 'EW', 'UGH', 'OOPS', 'WHOA', 'WOW', 'YEAH']
))

# Modifiers
MODS = sorted(set(
    ['1', '2', '3', '4', '5', '6', '7', '8', '9',
     '01', '02', '03', '04', '05', '06', '07', '08', '09', '10',
     '11', '12', '13', '14', '15', '16', '17', '18', '19', '20',
     'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K',
     'BIG', 'SM', 'LG', 'HI', 'LO', 'TALL', 'SHORT',
     'LEFT', 'RIGHT', 'UP', 'DN', 'DOWN', 'IN', 'OUT', 'ON', 'OFF',
     'LOOP', 'START', 'END', 'MAIN', 'ALT', 'EXTRA', 'BONUS',
     'TOP', 'BOT', 'BOTTOM', 'NORTH', 'SOUTH', 'EAST', 'WEST',
     'LIT', 'DARK', 'DIM', 'BRIGHT',
     'CRY', 'HAPPY', 'SAD', 'MAD', 'MEH', 'OK', 'GOOD', 'BAD',
     'BREATHIN', 'BUMPY', 'POOPY', 'WET', 'DRY', 'BIGFAT']
))

# ---- Brute ----
hits = defaultdict(list)
seen = set()


def try_(s):
    if s in seen:
        return
    seen.add(s)
    h = calcHash(s)
    if h in audio_unk:
        hits[h].append(s)


count = 0
for actor in ACTORS:
    for verb in VERBS:
        # FX_ACTOR_VERB and FX_ACTOR_VERB_<MOD>
        for sep in ['_', '']:
            base = 'FX_' + actor + sep + verb
            try_(base)
            for mod in MODS:
                try_(base + sep + mod)
                try_(base + '_' + mod)
                try_(base + mod)
        # NOTE: All verified raw-namespace audio cracks follow FX_ACTOR_VERB
        # order. Reversed FX_VERB_ACTOR is intentionally NOT brute-forced — it
        # produces high-numbered modifier collisions that look real but aren't.
        count += 1

print(f'Tested {len(seen):,} unique strings, hits: {len(hits)}', file=sys.stderr)

# ---- Score and report ----
# Two normalizations: identity (s) for raw uniqueness, and alpha-stripped
# (calcHash sees both as identical). Group multi-cand hits by alpha-stripped
# form, since FX_KLAY_DUCK_DOWN and FX_KLAYDUCKDOWN are the same semantic name.
def alpha_only(s):
    return ''.join(c for c in s.upper() if c.isalnum())

semantic_hits = {}
for h, names in hits.items():
    by_form = defaultdict(list)
    for n in names:
        by_form[alpha_only(n)].append(n)
    semantic_hits[h] = by_form  # alpha-form -> [literal variants]

print(f'\n=== Audio FX_ hits (semantic-deduplicated) ===')
single_sem = []
multi_sem = []
for h in sorted(semantic_hits, key=lambda x: (len(semantic_hits[x]), x)):
    forms = semantic_hits[h]
    n_sem = len(forms)
    row = audio_unk[h]
    if n_sem == 1:
        # Only one semantic name (just separator variants)
        form = next(iter(forms))
        # Pick the most-underscored (with FX_ACTOR_VERB_MOD style)
        best = sorted(forms[form], key=lambda s: (-s.count('_'), len(s), s))[0]
        single_sem.append((h, best, row))
    else:
        # Multiple semantically distinct candidates
        candidates = []
        for form, variants in forms.items():
            best = sorted(variants, key=lambda s: (-s.count('_'), len(s), s))[0]
            candidates.append(best)
        multi_sem.append((h, candidates, row))

print(f'\n--- {len(single_sem)} SEMANTICALLY-UNIQUE hits ---')
for h, name, row in single_sem:
    levels = row['levels'][:60]
    popc = bin(h).count('1')
    print(f'  0x{h:08x}  popc={popc:2d}  levels={levels}')
    print(f'      -> {name}')

print(f'\n--- {len(multi_sem)} MULTI-CANDIDATE hits (semantically distinct) ---')
for h, names, row in multi_sem[:60]:
    levels = row['levels'][:60]
    popc = bin(h).count('1')
    print(f'  0x{h:08x}  popc={popc:2d}  levels={levels} [{len(names)} cands]')
    for s in sorted(names, key=lambda x: (len(x), x))[:8]:
        print(f'      -> {s}')
    if len(names) > 8:
        print(f'      ... ({len(names)-8} more)')

# Save
os.makedirs('docs/analysis/asset-identification', exist_ok=True)
with open('docs/analysis/asset-identification/fx_saturation_hits.csv', 'w') as f:
    f.write('id_hex,name,popc,n_sem_cands,levels\n')
    for h, name, row in single_sem:
        popc = bin(h).count('1')
        f.write(f'0x{h:08x},{name},{popc},1,{row["levels"][:60]}\n')
    for h, names, row in multi_sem:
        for n in names:
            popc = bin(h).count('1')
            f.write(f'0x{h:08x},{n},{popc},{len(names)},{row["levels"][:60]}\n')

print(f'\nSaved to docs/analysis/asset-identification/fx_saturation_hits.csv', file=sys.stderr)
