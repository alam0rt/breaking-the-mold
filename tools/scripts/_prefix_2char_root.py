"""Brute all 2-letter + PICKUP and similar pattern variations.

Audio: FX_PICKUP_X -> FXPICKUP (8 alnum)
Sprite: ?? + PICKUP variants

Also try common abbreviation prefixes + PICKUP/OBJ/etc.
"""
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
ALPHABET = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789'

# Test all 2-char prefixes + various word
print('=== 2-char prefix + PICKUP (8 alnum) ===')
roots = ['PICKUP', 'OBJ', 'BONUS', 'POW', 'POWUP', 'POWERUP', 'POWER',
         'OBJECT', 'OBJECTS', 'OBJ_', 'PICKUPS', 'ITEM', 'ITEMS',
         'BNS', 'POWUPS', 'GET', 'GRAB', 'PROP', 'PROPS', 'GIFT', 'GIFTS',
         'BLB', 'BLBP', 'BLBPCK', 'SPRITE', 'SPRPICKUP', 'SPRPICKUP_', 
         'COLLECT', 'COLLECTABLE', 'COLLECTIBLE', 'CB']

exact = []
hits_2char = []
for root in roots:
    for a in ALPHABET:
        for b in ALPHABET:
            name = a + b + root
            h, sh = calc(name)
            if h == T and sh == TS:
                exact.append(name)
                hits_2char.append((name, root))

print(f'Exact 2-char prefix + root matches: {len(hits_2char)}')
for name, root in hits_2char[:50]:
    print(f'  *** {name!r} = {name[:2]} + {root!r}')

# Test prefix + 2-char suffix (analogous structure)
print('\n=== root + 2-char suffix (8 alnum) ===')
hits_2suf = []
for root in roots:
    for a in ALPHABET:
        for b in ALPHABET:
            name = root + a + b
            h, sh = calc(name)
            if h == T and sh == TS:
                hits_2suf.append((name, root))

print(f'Exact root + 2-char suffix matches: {len(hits_2suf)}')
for name, root in hits_2suf[:50]:
    print(f'  *** {name!r} = {root!r} + {name[-2:]}')

# Test 1-char + root + 1-char
print('\n=== 1-char + root + 1-char (8 alnum) ===')
hits_sandwich = []
for root in roots:
    for a in ALPHABET:
        for b in ALPHABET:
            name = a + root + b
            h, sh = calc(name)
            if h == T and sh == TS:
                hits_sandwich.append((name, root))

print(f'Sandwich matches: {len(hits_sandwich)}')
for name, root in hits_sandwich[:50]:
    print(f'  *** {name!r}')

# Test single-letter prefix + various
print('\n=== single letter + various roots (test parity) ===')
single_roots = ['OBJ','PICKUP','BONUS','POW','POWUP','SPRITE','OBJECT','BNS','GFXPICKUP','PSXPICKUP','GET','GRAB','HUD','HUDPICKUP']
for root in single_roots:
    for a in ALPHABET:
        name = a + root
        h, sh = calc(name)
        if h == T and sh == TS:
            print(f'  *** {name!r}')

# Test 3-char prefix + root
print('\n=== 3-char prefix + short root (test 6,8,10 alnum) ===')
for root in ['OBJ','PICKUP','POW','GET','PUP','BNS','PCK','GFX','SFX']:
    for a in ALPHABET:
        for b in ALPHABET:
            for c in ALPHABET:
                name = a + b + c + root
                h, sh = calc(name)
                if h == T and sh == TS:
                    # Filter to "looks like English" or has good chars
                    if all(x in 'ABCDEFGHIJKLMNOPQRSTUVWXYZ' for x in [a,b,c]):
                        bd = bin(h ^ T).count('1')  # always 0 for exact
                        print(f'  *** {name!r} (XXX + {root!r})')
