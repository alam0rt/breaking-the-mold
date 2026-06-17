"""Try OBJ-family and other near-matches with extra characters."""
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

# Try systematic stem appending
print('=== Single-char additions to known abbreviations ===')
stems = ['OBJ','PCK','PUP','BNS','POW','PWR','SPR','GET','FX','SX','GFX',
         'BG','SP','SPRT','ANM','UI','MENU','SM','MS','A','S','M','GET','GIVE',
         'PROP','PROPS','BLB','HUD','SCN','TIM','VAB',
         # 4-char
         'OBJ1','OBJ2','OBJ0','PCKS','PUPS','BNSS','PUFP','PUFS',
         # 5-char
         'SPRPCK','BLBPCK','OBJPCK','BNSPCK','PUPPCK','PWRPCK','GETPCK',
        ]

# Try N-char extensions to short stems
all_results = []
for stem in stems:
    # Single, double, triple, quad char additions both before AND after
    for a in ALPHABET:
        name = stem + a
        h, sh = calc(name)
        if h == T and sh == TS: all_results.append(name)
        name = a + stem
        h, sh = calc(name)
        if h == T and sh == TS: all_results.append(name)
    # 2-char additions
    for a in ALPHABET:
        for b in ALPHABET:
            name = stem + a + b
            h, sh = calc(name)
            if h == T and sh == TS: all_results.append(name)
            name = a + b + stem
            h, sh = calc(name)
            if h == T and sh == TS: all_results.append(name)
            name = a + stem + b
            h, sh = calc(name)
            if h == T and sh == TS: all_results.append(name)

print(f'Found {len(all_results)} matches')
for s in sorted(set(all_results)):
    print(f'  *** {s!r}')
