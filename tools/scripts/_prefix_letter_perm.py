"""Check XX_PICKUP variants and all 2-letter prefix combos."""
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
A = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789'

# Test all 2-char + PICKUP, 2-char + PCK, 2-char + OBJ, etc
# Also test PICKUP + 2-char and similar permutations
TAILS = ['PICKUP', 'PICKUPS', 'PICKED', 'PCKUP', 'PCKUPS',
         'OBJ', 'OBJS', 'OBJECT', 'OBJECTS',
         'BONUS', 'BNS', 'POW', 'POWUP', 'POWERUP', 'POWER',
         'GET', 'GRAB', 'TAKE', 'COLLECT', 'PROP', 'PROPS',
         'ITEM', 'ITEMS', 'PUP', 'PUPS', 'PCK', 'GIFT',
         'POPP', 'POPS', 'POPSPR', 'POPPED',
         'TYPE', 'TYPES', 'CAT', 'CATS', 'CATEGORY',
         'COLLECTABLE','COLLECTIBLE','REWARDS','REWARD',
         # Skullmonkeys-themed
         'EGG', 'EGGS', 'SKULL', 'POOPS', 'POOP', 'POPP',
         'POPPY', 'POOPY', 'POPS',
         # Things ending with 'P', 'S', 'X'
         'AS', 'BS', 'CS', 'DS', 'ES', 'FS', 'GS', 'HS', 'IS', 'JS', 'KS', 'LS', 'MS', 'NS',
         'OS', 'PS', 'QS', 'RS', 'SS', 'TS', 'US', 'VS', 'WS', 'XS', 'YS', 'ZS',
        ]

cand_exact = []
n = 0
for t in TAILS:
    for a in A:
        for b in A:
            for arr in [a+b+t, t+a+b, a+t+b, b+t+a, b+a+t, t+b+a]:
                n += 1
                h, sh = calc(arr)
                if h == T and sh == TS:
                    cand_exact.append(arr)

# Also 3-char additions
for t in ['PCK','PUP','BNS','POW','OBJ','GET','PRP','GFX','SFX','BLB','HUD','UI','OB','PR','BN','PU','PW','GE','PI','PCK','PICK','POW']:
    for a in A:
        for b in A:
            for c in A:
                for arr in [a+b+c+t, t+a+b+c, a+t+b+c, a+b+t+c]:
                    n += 1
                    h, sh = calc(arr)
                    if h == T and sh == TS:
                        cand_exact.append(arr)

print(f'Tested {n} candidates')
print(f'Exact matches: {len(cand_exact)}')
unique = sorted(set(cand_exact))
for s in unique[:200]:
    print(f'  *** {s!r}')
print(f'Total unique: {len(unique)}')
