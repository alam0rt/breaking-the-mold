"""Try names for the green checkmark/heart collectible (BRG1;SOAR)."""
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

target_audio = 0x62000441
target_sprite = 0x08624580

WORDS_CORE = [
    'CHECK','CHECKMARK','TICK','TICKMARK','MARK','HEART',
    'BLINK','GREEN','LEAF','GROWTH',
    'DIAMOND','GEM','JEWEL','SHARD','CRYSTAL','EMERALD','JADE','VINE',
    'PIE','SLEEP','SLOTH','POTION','HEALTH',
    'BATTERY','POWER','PLUG','SOCKET','CHARGE',
    'DRIP','DROP','WET','RAIN','SLIME','OOZE','GOO',
    'BANANA','APPLE','CHERRY','BERRY','LEMON','MELON','FRUIT',
    'BIRD','FEATHER','EGG','CHICK','HATCH','NEST',
    'PIRATE','FLAG','SWORD','HAT','BEARD',
    'SOAR','SKY','CLOUD','WING','BREEZE','FLIGHT',
    'BRG','BRIDGE',
    'PRIZE','GIFT','HAUL','LOOT','BOOTY','TREASURE',
    'HALO','ANGEL','HOLY','BLESSING','SAINT',
    'DOZEN','EXTRA',
    'JOE','MIKE','SHISH','KEBAB','KABOB',
    'KLAYMEN','KLOG','KLOGG','SKULLBIT','BIT',
    'COMICAL','COMIC','BAUBLE','TROPHY',
    'GREENHEART','GREEN_HEART','GREEN1','GREEN_1',
    'BIGGUM','GUMBALL','GUMHEAD','HEAD',
    'TURTLE','SHELL','CRAB','FISH',
    'WAVE','SURF','BOARD','SAIL','BOAT','RAFT',
    'BUBBLE','BUBBLY','BLOB',
    'EYE','EYEBALL','PUPIL','GAZE',
    'SCROLL','BOOK','PAPER','LETTER','LET','MAIL','POST',
    'KEY','LOCK','RING','RINGS',
    'STAR','MOON','SUN','BLAZE','FLAME','FIRE',
    'CHIME','BELL','TONE','NOTE','TUNE','SONG',
    'PRESS','PRESSED','BUTTON',
    'WATER','LIQUID','POTION','HEALTH',
    'WAND','STAFF','WANDS',
    'STONE','PEBBLE','ROCK',
    'CONE','TRIANGLE','SQUARE','CIRCLE','OVAL',
    'SLOT','MACHINE','LEVER',
    'COMP','COMPASS','MAP',
    'CAKE','BREAD','LOAF','PIE','SLICE',
    'BUBBLY','SUDS','FOAM','SOAP','SHAMPOO',
    'TOOTH','TONGUE','LIP','MOUTH','TEETH',
    'TOOL','HAMMER','SAW','SCISSORS','PLIERS',
    'WHEEL','GEAR','COG','SPRING','BOLT','NUT','SCREW',
    'EARTH','GROUND','DIRT','MUD','ICE','SNOW',
    'CANDY','CHOCO','SWEET','SUGAR','SALT','PEPPER',
    'CHEESE','BURGER','HOTDOG','PIZZA',
    'BAT','MOUSE','RAT','CAT','DOG','PIG','COW',
    'KITTY','PUPPY','PIGGY','DOGGY',
    'POOPSCOOP','POOP','SCOOP','SPOON','FORK','KNIFE',
    'SODA','POP','COKE','JUICE','MILK','TEA',
    'SOUP','STEW','BROTH','GRAVY',
]

PREFIXES_AUDIO = ['FX_','SFX_','SND_','UI_']
PREFIXES_SPRITE = ['SPR_','IMG_','PIC_','ANI_','ANIM_','OBJ_','PROP_','ENT_','MDL_']
PREFIXES_BARE = ['']

SUFFIXES = ['','_1','_2','_3','_GET','_PICKUP','_GRAB','_HIT','_TOUCH','_USE',
            '_LO','_HI','_BIG','_SM','_LG',
            '_LOOP','_END','_START','_MAIN','_ALT',
            '_A','_B','_C','_GREEN','_BLUE','_RED','_GOLD','_SILVER']

audio_matches = []
sprite_matches = []
seen = set()
for w in WORDS_CORE:
    for pfx in PREFIXES_AUDIO + PREFIXES_SPRITE + PREFIXES_BARE:
        for sfx in SUFFIXES:
            for sep in ['_','']:
                s = pfx + w + sep + sfx.lstrip('_') if sfx else pfx + w
                if s in seen: continue
                seen.add(s)
                h = calcHash(s)
                if h == target_audio:
                    audio_matches.append(s)
                if h == target_sprite:
                    sprite_matches.append(s)

print(f'Tested {len(seen):,} strings')
print(f'\nAudio (0x62000441):')
for m in sorted(audio_matches, key=lambda s: (len(s), s)):
    print(f'  {m}')
print(f'\nSprite (0x08624580):')
for m in sorted(sprite_matches, key=lambda s: (len(s), s)):
    print(f'  {m}')
