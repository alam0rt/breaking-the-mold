"""Rank all 2996 PREFIX preimages by English-shape heuristics.

Heuristics (each candidate is scored):
- Contains common English bigrams (TH, ER, IN, EN, AT, OR, ON, AN, ED, ES, EA, RE, IT, AR, ST...)
- Has good vowel density (3-5 vowels in 6 chars is ideal)
- No 3+ digit runs
- No 3+ consonant runs
- Looks like a known abbreviation
"""
import re

with open('/tmp/prefix.html') as f:
    page = f.read()
preimages = re.findall(r'"([^"]+)"', re.search(r'var ALL=\[(.+?)\];', page).group(1))
print(f'Loaded {len(preimages)} preimages')

VOWELS = set('AEIOU')
DIGITS = set('0123456789')
COMMON_BIGRAMS = {'TH','HE','IN','ER','AN','RE','ON','AT','EN','ND','TI','ES','OR','TE',
                  'OF','ED','IS','IT','AL','AR','ST','TO','NT','NG','SE','HA','AS','OU',
                  'IO','LE','VE','CO','ME','DE','HI','RI','RO','IC','NE','EA','RA','CE',
                  'LI','CH','LL','BE','MA','SI','OM','UR'}
COMMON_TRIGRAMS = {'THE','AND','ING','ION','TIO','ENT','ATI','FOR','HER','TER','HAT','THA',
                   'ERE','ATE','HIS','CON','RES','VER','ALL','ONS','NCE','MEN','ITH','TED',
                   'ERS','PRO','THI','WIT','ARE','ESS','NOT','IVE','WAS','ECT','REA','COM',
                   'EVE','PER','INT','EST','STA','CTI','ICA','IST','EAR','AIN','ONE','OUR',
                   'ITE','OUT','ENC','HAS','PRE','SHE','ROM','TRA','NTH','ENT','ANT','ATIO'}
GAME_TOKENS = {'PCK','PICK','PCKUP','PUP','BNS','BONUS','OBJ','SOBJ','BLB','SPR','SPRITE',
               'KLAY','KLY','KMEN','GAME','MENU','PAUSE','HERO','PROT','ITEM','GET','GRAB',
               'TAKE','HAVE','MARK','HOLD','TYPE','MISC','GLB','GLOB','MAIN','BASE','THING',
               'GIFT','TREAT','TYP','SPRT','SPT','ANIM','SND','FX','SFX','SPR','SPRT',
               'OBJ','OBJS','EVENT','EVT','TIM','POW','PWR','POWER'}

def score(s):
    """Return (score, reason) for a preimage candidate."""
    L = len(s)
    sc = 0.0
    reasons = []
    
    n_vowels = sum(1 for c in s if c in VOWELS)
    n_digits = sum(1 for c in s if c in DIGITS)
    n_cons = L - n_vowels - n_digits
    
    # Penalty for digits
    if n_digits > 0:
        sc -= n_digits * 2
        reasons.append(f'-digits({n_digits})')
    
    # Reward decent vowel density (1-3 in 4-char, 2-4 in 6-char)
    if L == 4:
        if 1 <= n_vowels <= 3: sc += 1; reasons.append('+vowels-4')
    elif L == 6:
        if 2 <= n_vowels <= 4: sc += 2; reasons.append('+vowels-6')
    
    # Penalty for 3+ consecutive consonants
    consec_c = 0
    max_consec_c = 0
    for c in s:
        if c not in VOWELS and c not in DIGITS:
            consec_c += 1
            max_consec_c = max(max_consec_c, consec_c)
        else:
            consec_c = 0
    if max_consec_c >= 4:
        sc -= 3
        reasons.append(f'-cluster({max_consec_c})')
    elif max_consec_c >= 3:
        sc -= 1
        reasons.append('-cluster3')
    
    # Reward bigrams
    bigrams_found = 0
    for i in range(L-1):
        bg = s[i:i+2]
        if bg in COMMON_BIGRAMS:
            bigrams_found += 1
            reasons.append(f'+{bg}')
    sc += bigrams_found * 1.5
    
    # Reward trigrams
    trigrams_found = 0
    for i in range(L-2):
        tg = s[i:i+3]
        if tg in COMMON_TRIGRAMS:
            trigrams_found += 1
            reasons.append(f'+++{tg}')
    sc += trigrams_found * 3
    
    # Reward known game tokens as substring
    for tok in GAME_TOKENS:
        if tok in s:
            sc += 3
            reasons.append(f'+TOK:{tok}')
    
    return sc, reasons

scored = sorted(((score(p), p) for p in preimages), key=lambda x: -x[0][0])

print('TOP 50 BY ENGLISH-SHAPE SCORE:')
for (sc, reasons), p in scored[:50]:
    print(f'  {sc:5.1f}  {p:<10s}  {",".join(reasons)}')

print()
print('LENGTH=4 ONLY:')
l4 = [(sc, p) for (sc, _), p in scored if len(p) == 4]
l4.sort(key=lambda x: -x[0])
for sc, p in l4[:20]:
    print(f'  {sc:5.1f}  {p}')

print()
print('Strings looking like common english words:')
english_like = ['HERE','THERE','GIFT','HAVE','SAVE','MAKE','TAKE','GAME','PICK','GRAB',
                'ITEM','HELP','OBJS','OBJ_','PRIZE','TYPE','GIVE','SPAWN','PUP','PAGE',
                'HUNT','PLAY','SHOW','HOLD','LOOK','LURK','MARK','PROP','TOOL','BANG',
                'BONK','BONUS','BUFF','BUMP','BUNK','BURP','CAST','CATCH','CHOP','CLAP',
                'CLAW','CLUB','CRAB','DASH','DAZE','DEAL','DICE','DING','DIVE','DOPE',
                'DUMP','EMIT','FANG','FIRE','FISH','FIST','FIVE','FOOT','GAIN','GAZE',
                'GEAR','GEM','GLOB','GOAL','GOLD','GRAB','HASH','HASP','HAUL','HAWK',
                'HEAR','HOLD','HOOK','HOOP','HOOT','HOPE','HUNK','HURL','HUSH','HUNT',
                'JERK','JOKE','JOLT','JUMP','KEEP','KICK','KILL','KILO','KIND','KNAP',
                'KNIT','KNOB','KNOT','LACE','LASH','LAVA','LAZE','LEAK','LEAN','LEAP',
                'LEND','LENS','LIFE','LIFT','LIMP','LINE','LINK','LION','LIST','LOAD',
                'LOCK','LOOP','LOSE','LOSS','LOUD','LURE','LURK','MAGE','MAID','MAKE',
                'MAMA','MAPS','MARK','MASH','MATE','MAUL','MAZE','MEND','MICE','MIDI',
                'MIST','MORE','MUSE','MUTE','NEAR','NECK','NEED','NEON','NERD','NEST',
                'NEWS','NICE','NIGHT','NOON','OBJS','OPEN','PACK','PAGE','PAID','PAIL',
                'PALM','PARK','PART','PASS','PEEK','PILE','PILL','PIPE','PLOP','PLOT',
                'PLUM','POKE','POLE','POOL','POOR','POPE','POSE','PROP','PUFF','PULL',
                'PUMP','PUNK','PURE','PUSH','RACE','RAGE','RAIL','RAIN','RANT','RARE',
                'RATE','RAYS','READ','REAR','REEF','REEL','REND','REST','RIDE','RIFF',
                'RING','RISE','RISK','ROAD','ROAM','ROAR','ROBE','ROCK','RODE','ROLE',
                'ROOM','ROOT','ROPE','ROSE','RUBY','RUIN','RULE','SACK','SAFE','SAGA',
                'SAGE','SAID','SAIL','SALE','SAND','SAUL','SAVE','SCAB','SCAN','SCAR',
                'SEAL','SEAR','SEAT','SEED','SEEK','SEEM','SEEN','SEER','SELF','SELL',
                'SEMI','SEND','SHED','SHIN','SHIP','SHOE','SHOP','SHOT','SHOW','SIDE',
                'SIGH','SIGN','SILK','SING','SINK','SIRE','SITE','SIZE','SKID','SKIM',
                'SKIN','SKIP','SLAM','SLAP','SLED','SLIM','SLIP','SLIT','SLOT','SLOW',
                'SLUM','SMUG','SNAP','SNUB','SOAK','SOAP','SOAR','SOCK','SODA','SOFA',
                'SOFT','SOLD','SOLE','SOLO','SOME','SONG','SONS','SOON','SORE','SORT',
                'SOUL','SOUP','SOUR','SPAR','SPAT','SPED','SPIN','SPIT','SPRY','SPUR',
                'STAG','STAR','STAY','STEM','STEP','STIR','STUB','STUN','SUCH','SUCK',
                'SUDS','SUED','SUES','SUET','SUGAR','SUIT','SUM','SUMP','SUNS','SURE',
                'SURF','SWAG','SWAM','SWAP','SWAY','SWIM','TACK','TAIL','TAKE','TALE',
                'TAME','TANK','TAPE','TARN','TART','TASK','TEAL','TEAM','TEAR','TEEN',
                'TELL','TEND','TENS','TENT','TERM','TERN','TEST','TEXT','THAN','THAT',
                'THAW','THEE','THEM','THEN','THEY','THIN','THIS','THUD','THUG','TICK',
                'TIDE','TIDY','TIED','TIER','TIES','TIGER','TILE','TILT','TIME','TINT',
                'TIPS','TIRE','TOAD','TOIL','TOLD','TOLL','TOMB','TONE','TOOK','TOOL',
                'TOOT','TORE','TORN','TOSS','TOTE','TOUR','TOWN','TRAM','TRAP','TRAY',
                'TREE','TREK','TRIM','TRIO','TRIP','TROT','TRUE','TUBE','TUNE','TURF',
                'TURN','TWIG','TWIN','TYPE','USED','USER','USES','VAIN','VAMP','VAST',
                'VEAL','VEER','VEIL','VEIN','VENT','VERB','VERY','VEST','VIEW','VINE',
                'VIPE','VISE','VITA','VIVA','VOLE','VOLT','VOTE','WADE','WAGE','WAIL',
                'WAIT','WAKE','WALK','WALL','WAND','WANE','WANT','WARD','WARM','WARN',
                'WARP','WART','WARY','WASH','WASP','WAVE','WAVY','WEAK','WEAR','WEED',
                'WEEK','WEEP','WEFT','WELD','WELL','WELT','WEND','WENT','WERE','WEST',
                'WHAM','WHAT','WHEN','WHEY','WHIM','WHIP','WHIR','WHIT','WHIZ','WHO',
                'WHOA','WHOM','WHOP','WICK','WIDE','WIFE','WIGS','WILD','WILE','WILL',
                'WILT','WIMP','WIND','WINE','WING','WINK','WINS','WIPE','WIRE','WIRY',
                'WISE','WISH','WISP','WITH','WIVE','WOKE','WOLF','WOMB','WONK','WONT',
                'WOOD','WOOF','WOOL','WORD','WORE','WORK','WORM','WORN','WORST','WOVE',
                'WRAP','WREN','WRIT','YACK','YANK','YARD','YARN','YAWN','YEAR','YELL',
                'YELP','YOGA','YOKE','YOLK','YORE','YOUR','YOWL','YULE','YURT','ZANY',
                'ZEAL','ZEBU','ZERO','ZEST','ZEUS','ZINC','ZINE','ZING','ZIP','ZONE',
                'ZONK','ZOOM']

# Convert all preimages to set for fast lookup
preimg_set = set(preimages)
print(f'\nWords matching preimage set ({len(english_like)} candidates):')
hits = [w for w in english_like if w in preimg_set]
for h in hits:
    print(f'  *** EXACT MATCH: {h!r}')
if not hits:
    print('  None.')
