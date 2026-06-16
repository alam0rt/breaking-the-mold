"""Faster direction-sprite cracker using bidirectional search."""

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

TARGETS = {
    0x10b85810: 'down',
    0x10b91810: 'right slight down',
    0x10b9181c: 'up left',
    0x10b94810: 'right slight up',
    0x10b95010: 'up right',
    0x10b9481c: 'left slight down',
    0x10b95a10: 'up',
    0x10b95a1c: 'down slight left',
    0x10b95c10: 'up slight right',
    0x10b95c1c: 'down left',
    0x10b97810: 'right',
    0x10b9781c: 'left slight up',
    0x10b9d810: 'down right',
    0x10b9d81c: 'up slight left',
    0x10bb5810: 'down',
}

PREFIXES = [
    'FINN_','PLAYER_','PHART_','PUFF_','BABY_','VEHICLE_','RIDE_','PLAY_',
    'FINNDIR_','PLAYDIR_','BABY_DIR_','PHART_DIR_',
    'FINN_PLAYER_','PLAYER_FINN_','FINN_PUFF_','FINN_PHART_',
    'FINN_BABY_','BABY_FINN_','BABY_PUFF_','PUFF_BABY_',
    'BIRD_','PHART_BIRD_','BIRD_PHART_',
    'PHART_PLAYER_','PLAYER_PHART_',
    'POOPER_','POOTER_','PUFFY_','PUFFER_','POOFY_',
    'PHARTPUFF_','PUFFPHART_',
    'GAS_','GASBABY_','BABYGAS_','GAS_BABY_','BABY_GAS_',
    'BABY_PHART_','PHART_BABY_',
    '',
]

TOKENS = []
for d in ['UP','DOWN','LEFT','RIGHT','DN','LT','RT',
          'U','D','L','R',
          'NORTH','SOUTH','EAST','WEST','N','S','E','W',
          'NE','NW','SE','SW','EN','EW','WN','WS',
          'TOP','BOTTOM','BOT','BOTM','BTM',
          'FORWARD','FWD','BACK','BWD','BKWD',
          'CLIMB','DIVE','GLIDE','LEVEL','BANK',
          'FLAT','FALL','RISE','SHEET',
          'ASCEND','DESCEND','LOWER','RAISE','HIGH','LOW',
          'OVER','UNDER','ABOVE','BELOW',
          'TURN','TURNUP','TURNDN','TURNLEFT','TURNRIGHT',
          'ANGLE','ANG','TILT','TILTUP','TILTDN','TILTL','TILTR',
          'PITCH','YAW','ROLL',
          'HOVER','FLY']:
    TOKENS.append(d)

COMPOUNDS = []
for d1 in ['UP','DOWN','LEFT','RIGHT','DN','LT','RT','U','D','L','R',
           'N','S','E','W','NE','NW','SE','SW']:
    for d2 in ['UP','DOWN','LEFT','RIGHT','DN','LT','RT','U','D','L','R',
              'N','S','E','W']:
        if d1 != d2:
            for sep in ['_','']:
                COMPOUNDS.append(d1 + sep + d2)
            COMPOUNDS.append(d1 + 'AND' + d2)
            COMPOUNDS.append(d1 + '_AND_' + d2)

SLIGHTS = []
for d1 in ['UP','DOWN','LEFT','RIGHT','DN','LT','RT']:
    for d2 in ['UP','DOWN','LEFT','RIGHT','DN','LT','RT']:
        if d1 != d2:
            for slight in ['SLIGHT','SLIGHTLY','SL','SLT','MILD','HALF','SHALLOW','SOFT']:
                for sep in ['_','']:
                    SLIGHTS.append(d1 + sep + slight + sep + d2)
                    SLIGHTS.append(slight + sep + d1 + sep + d2)
                    SLIGHTS.append(d1 + sep + d2 + sep + slight)

ALL_TOKENS = TOKENS + COMPOUNDS + SLIGHTS

SUFFIXES = ['','_1','_2','_3','_0','_A','_B','_C','_F0','_F1','_F2',
            '_FR','_F','_X','_Y','_Z',
            '_LO','_HI','_SM','_BIG',
            '_ANIM','_LOOP','_END','_START','_MAIN','_ALT',
            '_PUFF','_PHART','_BABY','_FINN','_PLAYER']

cracks = {tid: [] for tid in TARGETS}
n = 0
for pfx in PREFIXES:
    for tok in ALL_TOKENS:
        for sfx in SUFFIXES:
            full = pfx + tok + sfx
            n += 1
            h = calcHash(full)
            if h in cracks:
                cracks[h].append(full)

print(f'Tested {n:,} strings')
print()
for tid, desc in TARGETS.items():
    if cracks[tid]:
        results = sorted(set(cracks[tid]), key=lambda s: (len(s), s))
        print(f'  0x{tid:08x} ({desc}):')
        for r in results[:6]:
            print(f'    {r}')
    else:
        print(f'  0x{tid:08x} ({desc}): no match')
"""Brute names for the 15 FINN player-vehicle direction sprites.

Pattern observed by user (hex prefixes mostly 0x10b9):
  0x10b85810 - down
  0x10b91810 - right slight down
  0x10b9181c - up left
  0x10b94810 - right slight up
  0x10b95010 - up right
  0x10b9481c - left slight down
  0x10b95a10 - up
  0x10b95a1c - down slight left
  0x10b95c10 - up slight right
  0x10b95c1c - down left
  0x10b97810 - right
  0x10b9781c - left slight up
  0x10b9d810 - down right
  0x10b9d81c - up slight left
  0x10bb5810 - down

Notes:
  - All player sprites for FINN bonus level
  - Two "down" entries (0x10b85810 and 0x10bb5810) -> probably two distinct names
  - Last byte 0x10 vs 0x1c: 0x10 = 1 bit set (pos 4), 0x1c = 3 bits set (pos 2,3,4)
  - 0x1c versions (5 of them) are all "diagonal slight" variants
"""

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

TARGETS = {
    0x10b85810: 'down',
    0x10b91810: 'right slight down',
    0x10b9181c: 'up left',
    0x10b94810: 'right slight up',
    0x10b95010: 'up right',
    0x10b9481c: 'left slight down',
    0x10b95a10: 'up',
    0x10b95a1c: 'down slight left',
    0x10b95c10: 'up slight right',
    0x10b95c1c: 'down left',
    0x10b97810: 'right',
    0x10b9781c: 'left slight up',
    0x10b9d810: 'down right',
    0x10b9d81c: 'up slight left',
    0x10bb5810: 'down',
}

# Direction tokens (ordered by likelihood)
DIRS = [
    'UP','DOWN','LEFT','RIGHT',
    'DN','LT','RT','U','D','L','R',
    'NORTH','SOUTH','EAST','WEST',
    'N','S','E','W',
    'NE','NW','SE','SW',
    'NNE','NNW','SSE','SSW','ENE','ESE','WNW','WSW',
    'UR','UL','DR','DL',
    'UPRIGHT','UPLEFT','DOWNRIGHT','DOWNLEFT',
    'UP_RIGHT','UP_LEFT','DOWN_RIGHT','DOWN_LEFT',
    'TOP','BOTTOM','TOPLEFT','TOPRIGHT','BOTLEFT','BOTRIGHT',
    'FORWARD','BACK','FRONT','BEHIND','REAR',
    'FLAT','LEVEL','HIGH','LOW',
    'STRAIGHT','TURN','BANK',
    'FORE','AFT','PORT','STARBOARD',
    'TURNUP','TURNDN','TURNLEFT','TURNRIGHT',
    'CLIMB','DIVE','GLIDE','LEVEL','BANKLEFT','BANKRIGHT',
]

# Sub-direction (slight)
SUBS = [
    '','SLIGHT','SLIGHTLY','SL','SLT','SOFT','SHALLOW','HALF','MILD','SMALL','SLITE','LITTLE',
    'A','B','MILD','LITE',
]

PREFIXES = [
    'FINN','PLAYER','PHART','PUFF','BABY','VEHICLE','RIDE','PLAY',
    'FINNDIR','FINN_DIR','PLAYDIR','PLAYER_DIR','PHART_DIR',
    'FINNANIM','FINN_ANIM','FINN_FRAME','FINN_F',
    'PHARTBABY','BABYPHART','BABYDIR','PHART_BABY',
    'BBOY','BABYBOY','RIDER','HERO',
    'FINN_PLAYER','FINN_PUFF','FINN_PHART','PUFF_PHART','PHART_PUFF',
    'PHARTS','PHART','PUFFER','PUFFY','POOPER','POOTER',
    'FINN_VEHICLE','PLAYER_VEHICLE',
    'BIRD','BIRDIE','BIRDIR','PHART_BIRD',
    # Maybe with 'FINN_' or 'PHART_' as prefix
]

# Suffixes after direction
SUFFIXES = [
    '','_1','_2','_3','_0','_A','_B','_C',
    '_F0','_F1','_F2','_FRAME','_FRAME0','_FRAME1','_F00','_F01',
    '_ANIM','_ANI','_LOOP','_END','_START','_MAIN','_ALT',
    '_LO','_HI','_SM','_BIG','_LG',
    'X','Y','Z','XX',
]

# Build reverse lookup
def find_for_target(target, max_results=8, name_filter=None):
    matches = []
    for pfx in PREFIXES:
        for d in DIRS:
            for sub in SUBS:
                for sfx in SUFFIXES:
                    for sep1 in ['_','']:
                        for sep2 in ['_','']:
                            # direction + (optional) sub + suffix
                            if sub:
                                middles = [d+sep1+sub, sub+sep1+d]
                            else:
                                middles = [d]
                            for mid in middles:
                                full = pfx + sep2 + mid + sfx
                                if calcHash(full) == target:
                                    if name_filter and not name_filter(full):
                                        continue
                                    matches.append(full)
                                    if len(matches) >= max_results:
                                        return matches
    return matches


print('=== First pass: simple FINN_<DIR> ===')
for tid, desc in TARGETS.items():
    matches = find_for_target(tid, max_results=10)
    if matches:
        print(f'  0x{tid:08x} ({desc}):')
        for m in sorted(set(matches), key=len)[:10]:
            print(f'    {m}')
    else:
        print(f'  0x{tid:08x} ({desc}): NO MATCH')
