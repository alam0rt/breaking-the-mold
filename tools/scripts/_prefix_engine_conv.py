"""Use real engine prefix conventions from the guesses page."""
CS = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789'
STEP = [0]*36
for d, c in enumerate(CS):
    o = ord(c)
    if c.isdigit(): o += 22
    STEP[d] = (o - 64) & 31

def calc(s):
    h, sh = 0, 0
    for c in s.upper():
        if c in CS:
            d = CS.index(c)
            sh = (sh + STEP[d]) & 31
            h ^= 1 << sh
    return h & 0xFFFFFFFF, sh

T, TS = 0x88200080, 27

# Real engine prefix conventions
# (Audio uses FX_, so sprite probably uses one of these)
real_prefixes = ['MENU', 'SM', 'BG', 'OBJ', 'SP', 'S', 'A', 'GFX', 'FX', 'SPRT', 'ANM', 'SPR', 'UI']
# Also try natural variants and capitalisations
extra_prefixes = ['SX','MS','MC','AS','SS','MM','PR','PRP','ENT','BLB','HUD',
                  'AN','ANI','MOV','AUD','TX','TXT','MOD','TIM','VAB',
                  'BNS','PCK','PUP','POW','PWR','GET','GRAB','MAIN',
                  'BSPR','SPRT','PRPT','OBJT','MPCK','MPUP','MSPR']
prefixes = list(set(real_prefixes + extra_prefixes))

# Various roots/tail tokens (we hash PREFIX_TAIL with underscores stripped)
roots = [
    'PICKUP','PICKUPS','PICKUP_','PCKUP','PCKUPS',
    'BONUS','BONUSES','BNS','BNSS',
    'POW','POWER','POWUP','POWERUP','POWERUPS',
    'OBJ','OBJS','OBJECT','OBJECTS',
    'ITEM','ITEMS',
    'PROP','PROPS','PROPERTY','PROPERTIES',
    'PICK','PICKS','PICKED','PICKER',
    'GIFT','GIFTS',
    'GET','GETS','GRAB','GRABS','TAKE','TAKES',
    'COLLECT','COLLECTABLE','COLLECTIBLE','COLLECTIBLES','COLLECTABLES',
    'PRIZE','PRIZES','GOODY','GOODIES','TREASURE','TREASURES',
    'PUP','PUPS','DROP','DROPS','TOKEN','TOKENS',
    'GAIN','GAINS',
    'EXTRA','EXTRAS','LIFE','LIVES','BOOST','BOOSTS',
    'POOP','POOPS','PUFF','PUFFS','BURP','BURPS',
    'POPPABLE','POPPABLES','POP','POPS','POPPED',
    'CHARM','CHARMS','MAGIC','MAGICS','SPELL','SPELLS',
    'HEAL','HEALS','HEALTH',
    'KLAY','KLAYM','KLAYMEN','KMEN',
    'CLAYBALL','CLAYBALLS','CLAY','CLAYS',
    'OBJ_PICKUP','OBJPICKUP',
]

# Number-suffix variants (the actual prefix might have a trailing num)
all_cands = set()
for pre in prefixes + ['']:
    for root in roots + ['']:
        if not pre and not root: continue
        for suf in ['', '0', '1', '2', '3', '00', '01', '02', '03',
                    '_', '_0', '_1', '_2',
                    'S','SS',
                    '_TYPE','TYPE',
                    'S_', '_S', '_X', 'X',
                   ]:
            all_cands.add(pre + root + suf)
            if pre and root:
                # Also with underscore
                all_cands.add(pre + '_' + root + suf)

print(f'Generated {len(all_cands)} candidates')

exact = []
for s in all_cands:
    h, sh = calc(s)
    if h == T and sh == TS:
        exact.append(s)

print(f'Exact matches: {len(exact)}')
for s in sorted(exact):
    print(f'  *** {s!r}')
