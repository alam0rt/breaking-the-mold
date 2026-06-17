"""Test L=8, L=10, L=12 composite prefix candidates."""
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

# Compose category + "PICKUP" or other forms
categories = [
    '', 'S', 'SP', 'SPR', 'SPRT', 'SPRIT', 'SPRITE',
    'M', 'MA', 'MAIN', 'GAME', 'BLB', 'OBJ',
    'PROP', 'PROPS', 'P', 'P_', 'PT', 'PR', 
    'G', 'GFX', 'SFX', 'SX', 'A', 'AS', 'GG',
    'PCK', 'PICK', 'PICKUP',
    'B', 'BG', 'BGS', 'BGP',
    'I', 'IT', 'ITEM', 'ITEMS',
    'BS', 'BNS', 'BONUS',
    'GET', 'GRAB', 'TAKE', 'GAIN',
    'PROP', 'PROPS', 'PROPS_',
    'KLAY', 'KLAYM', 'CLAY', 'CLAYBALL',
]
words = [
    'PICKUP', 'PICKUPS', 'PICKUPER', 'PICKER',
    'BONUS', 'BNS', 'BNUS', 'BONUSE', 'BONUSES',
    'POWUP', 'POWERUP', 'POWERUPS', 'POWER',
    'OBJ', 'OBJS', 'OBJECT', 'OBJECTS',
    'ITEM', 'ITEMS', 'ITEMTYPE',
    'PROP', 'PROPS', 'PROPERTY',
    'GIFT', 'GIFTS',
    'GET', 'GRAB', 'TAKE', 'HOLD',
    'COLLECT', 'COLLECTABLE', 'COLLECTIBLE',
    'GOODY', 'GOODIES', 'PRIZE', 'PRIZES', 'TREAS', 'TREASURE',
    'PICK', 'PICKS', 'PICKED', 'GRAB', 'GRABBED',
    'PUP', 'PUPS',
]

# Build composites of even length
composites = set()
for c in categories:
    for w in words:
        composites.add(c + w)
        composites.add(w + c)
        if c:
            composites.add(c + '_' + w)
            composites.add(w + '_' + c)
        # Numeric suffixes
        for n in ['', '0', '1', '2', '3', '0123', '00', '01', '02', '0001', '_0', '_1']:
            composites.add(c + w + n)
            composites.add(w + c + n)
# Filter to alnum length even and within range
def alnum_len(s):
    return sum(1 for c in s.upper() if c in CS)
filtered = {s for s in composites if alnum_len(s) in (4, 6, 8, 10, 12)}
print(f'Generated {len(composites)} composites, {len(filtered)} have even alnum length 4-12')

exact = []
near = []
for s in filtered:
    h, sh = calc(s)
    if h == T and sh == TS:
        exact.append(s)
    elif sh == TS:
        bd = bin(h ^ T).count('1')
        near.append((bd, s, h, alnum_len(s)))

print(f'Exact matches: {len(exact)}')
for s in exact:
    print(f'  *** EXACT: {s!r}  alnum_len={alnum_len(s)}')

near.sort()
print('\nNear matches (sh=27, smallest h-distance):')
for bd, s, h, L in near[:40]:
    print(f'  bd={bd:2d}  alnum_L={L:2d}  {s!r:<28s} h=0x{h:08x}')
