"""Hash level names extracted from BLB strings against all uncracked targets.
"""
import csv
import sys; sys.path.insert(0, 'tools/scripts')
from calchash import calcHash

def rotr(x, n): return ((x >> n) | (x << (32-n))) & 0xFFFFFFFF
SEED = 0x28C0E011

# Level names from BLB strings dump
candidates = [
    'Options',
    'Skullmonkey Gate',
    'Skullmonkey Brand Hotel',
    'Skullmonkey Brand',
    'Science Center',
    'Monkey Shrines',
    'Monkey Shrine',
    'The Amazing Drivey Funtimes',
    'The Amazing Drivey',
    'The Amazing Drivey Fun',
    'Shriney Guard',
    'Hard Boiler',
    'Joe-Head-Joe',
    'Joe Head Joe',
    'Elevated Structure of Coolness',
    'Elevated Structure',
    'Ynt Death Garden',
    'Ynt Mines',
    'Ynt Weeds',
    'Ynt Eggs',
    'Glenn Yntis',
    'Glenn Ynti',
    'Monk Rushmore',
    "1970's",
    "1970",
    'Soar Head',
    'Soar Heads',
    'Shards',
    'Castle de los Muertos',
    'Castle de los Muerto',
    'Monkey Mage',
    'The Incredible Drivey',
    'The Incredible Drive',
    'Worm Graveyard',
    'Klogg',
    'Evil Engine #9',
    'Evil Engine 9',
    'Evil Engine No 9',
    # Variant casing
    'OPTIONS','SKULLMONKEY GATE','SCIENCE CENTER','MONKEY SHRINES',
    'SHRINEY GUARD','HARD BOILER','JOE HEAD JOE','YNT DEATH GARDEN',
    'YNT MINES','YNT WEEDS','YNT EGGS','GLENN YNTIS','MONK RUSHMORE',
    '1970S','SOAR HEAD','SHARDS','CASTLE DE LOS MUERTOS','MONKEY MAGE',
    'THE INCREDIBLE DRIVEY','WORM GRAVEYARD','KLOGG','EVIL ENGINE 9',
    'EVIL ENGINE','SKULLMONKEY BRAND HOTEL','THE AMAZING DRIVEY',
    'ELEVATED STRUCTURE','ELEVATED STRUCTURE OF COOLNESS',
    # Movie file names (as they appear in game)
    'MVDWI','MVLOGO','MVEA','MVINTRO1','MVINTRO2','MVGAS','MVYAM',
    'MVRED','MVYNT','MVEYE','MVEVIL','MVEND','MVWIN',
    'DWI','LOGO','EA','INTRO1','INTRO2','GAS','YAM','RED','YNT','EYE','EVIL','END','WIN',
    # Common menu / ui strings
    'demo','DEMO','Demo','demo screen','Demo Mode','Demo mode','DEMO MODE',
    # Subtitles & promo
    'PRESENTS','Presents','presents','BY','From','Starring','Featuring',
    'A NEVERHOOD GAME','THE NEVERHOOD','NEVERHOOD','SKULLMONKEYS',
    'Skullmonkeys','SKULL MONKEYS',
    # Generic UI
    'Continue','New Game','Save Game','Load Game','Pause','Resume',
    'Quit','Exit','Help','Sound','Music','Volume','Settings','Configuration',
    'Controller','Buttons','Vibration','Memory Card','Save Slot',
    'Yes','No','Cancel','Apply','Back','Next','Previous',
]

# Load all uncracked targets
unkn = {}
with open('docs/analysis/asset-identification/cracked_names.csv') as f:
    for r in csv.DictReader(f):
        if r['status'] != 'uncracked':
            continue
        if not r['id_hex'].startswith('0x'):
            continue
        idv = int(r['id_hex'], 16)
        if idv == 0:
            continue
        unkn[idv] = (r['type'], int(r['floor']), r['levels'][:60], r['alts'][:80])

# Build raw_targets map: id -> raw_target
raw_set = {}  # raw_target -> [(id, info)]
for idv, info in unkn.items():
    typ, fl, lv, alts = info
    if typ == 'audio':
        raw_set.setdefault(idv, []).append((idv, info))
    else:
        inv = rotr(idv ^ SEED, 27)
        raw_set.setdefault(inv, []).append((idv, info))

print(f'Loaded {len(unkn)} uncracked targets, {len(raw_set)} unique raw values')
print(f'Testing {len(candidates)} BLB-derived candidates...')
print()

for c in candidates:
    h = calcHash(c)
    if h in raw_set:
        print(f'>>> HIT: {c!r} (calcHash=0x{h:08x})')
        for idv, info in raw_set[h]:
            typ, fl, lv, alts = info
            print(f'      0x{idv:08x} {typ:<7s} fl={fl} lv={lv}')
            if alts: print(f'        alts={alts}')
