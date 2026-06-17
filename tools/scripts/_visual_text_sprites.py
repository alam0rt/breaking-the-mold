"""Verify text sprite identifications from review sheets."""
import sys
sys.path.insert(0, 'tools/scripts')
from calchash import calcHash

def rotl(v, r):
    r &= 31
    return ((v << r) | (v >> ((32 - r) & 31))) & 0xFFFFFFFF

def wrap(name):
    return 0x28C0E011 ^ rotl(calcHash(name), 27)

def raw(name):
    return calcHash(name)

# Visually identified text sprites
TARGETS = {
    # Direct visual reads
    0x28c080df: 'DEMO text',
    # Other interesting
    0x80e85ea0: 'Hamster Shield orbit icon (green @)',
    0x9158a0f6: 'Phoenix Hand projectile sprite (green hand)',
    0x902c0002: 'Super Willie collectible icon (Klay with q-tip)',
    0x88a28194: '1970 hamster collectible sprite (yellow 1970)',
    0x8c510186: 'Phart Head feather/horn sprite',
    0xa9240484: '1-Up extra life collectible',
    0x6a351094: 'Sparkle/star particle (large)',
    0x6c351094: 'Sparkle/star particle (small)',
}

# Common name candidates
candidates = [
    # Direct text candidates
    'DEMO','DEMOTEXT','DEMO_TEXT','DEMOSTAMP','DEMO_STAMP','TXT_DEMO','TEXT_DEMO',
    'NOTFORSALE','NOT_FOR_SALE','SAMPLE','PROTOTYPE','PROTO','PROMO','TRIAL',
    # Hamster Shield
    'SHIELD','HAMSHIELD','HAMSTER_SHIELD','HAMSTERSHIELD','HAM','HAMSTER','SLAPPY',
    'SLAPPYHAMSTER','SLAPPY_HAMSTER','HAMSPRITE','HAM_SPRITE','SHIELDORB','SHIELD_ORB',
    'HAMORB','HAM_ORB','HAMSTERORB','HAMSTER_ORB','SHIELDICON','SHIELD_ICON',
    # Phoenix Hand
    'PHOENIX','PHOENIXHAND','PHOENIX_HAND','HAND','PHEONIX','PHEONIXHAND',
    'PHOENIXPROJ','PHOENIX_PROJ','PHOENIXFIST','PHOENIX_FIST','FIST','HANDPROJ',
    'PHANDS','PHAND','PROJHAND','PROJ_HAND','GREENHAND','GREEN_HAND',
    # Super Willie
    'SUPERWILLIE','SUPER_WILLIE','WILLIE','WILLY','SUPERWILLY','SUPER_WILLY',
    'SUPERICON','SUPER_ICON','WILLIE_ICON','WILLIEICON','SUPERWILLIESPRITE',
    'WILLIESPRITE','WILLIE_SPRITE','SUPERWILLIE_ICON','SWICON','SW_ICON',
    # 1970
    '1970','1970ICON','1970_ICON','HAMSTERICON','HAMSTER_ICON','SLAPPYICON',
    'SLAPPY_ICON','GOLDHAMSTER','GOLD_HAMSTER','YEARICON','YEAR_ICON',
    # Phart Head
    'PHART','PHARTHEAD','PHART_HEAD','FARTHEAD','FART_HEAD','PHARTFETHER','PHART_FEATHER',
    'PHARTSPRITE','PHART_SPRITE','FEATHER','HORN','FARTICON','FART_ICON',
    'PHARTICON','PHART_ICON','GAS_ICON','GASCAN_ICON','GASCAN','TOOT',
    # 1-Up
    'ONEUP','ONE_UP','EXTRALIFE','EXTRA_LIFE','EXTRALIFEICON','EXTRA_LIFE_ICON',
    '1UP','1_UP','LIFE','LIFE_ICON','LIFEICON','XLIFE','XLIFEICON','EXTRA_LIVES',
    # Sparkle
    'SPARKLE','SPARKLES','SPARK','STAR','STARS','TWINKLE','GLITTER','GLINT',
    'SHINE','SHINY','SHIMMER','GLOW','FLASH','SPARKICON','SPARK_ICON',
]

print('Testing %d candidates against %d targets:' % (len(candidates), len(TARGETS)))
for n in candidates:
    r = raw(n)
    w = wrap(n)
    if r in TARGETS:
        print('  HIT (raw): %s -> 0x%08x  (%s)' % (n, r, TARGETS[r]))
    if w in TARGETS:
        print('  HIT (wrap): %s -> 0x%08x  (%s)' % (n, w, TARGETS[w]))
print('done')
