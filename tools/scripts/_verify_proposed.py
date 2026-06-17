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

# Visual identifications with guesses
targets = [
    # (target_id, role, proposed_name_to_try)
    (0x33808e1b, 'Menu cursor animation frame', ['CURSOR']),
    (0x63848e59, 'Menu cursor', ['CURSOR']),
    (0x61981a0c, 'Plain monkey enemy', ['SCREAMER_MONKEY', 'SCREAMERMONKEY', 'SCREAMER', 'MONKEY']),
    (0x75189608, 'Plain monkey enemy variant', ['SCREAMER_MONKEY', 'SCREAMERMONKEY']),
    (0x00e2f188, 'Number glyph 0-9', ['NUM', 'NUMBER', 'NUMBERS', 'DIGITS', 'DIGIT', 'NUMERIC', 'FONT', 'GLYPH', 'GLYPHS', 'CHARS']),
    (0xc8f90114, 'GLEN boss / large enemy creature', ['GLEN', 'GLENBOSS', 'GLENN', 'GLENNBOSS', 'GIANT', 'GIANTKLAYMEN']),
    (0x8ab92024, 'LOADING screen text + Klaymen', ['LOADING', 'LOAD', 'LOADSCREEN']),
]

print('=== Proposed name verification ===')
for tid, role, names in targets:
    print('\n0x%08x (%s):' % (tid, role))
    for n in names:
        r = raw(n)
        w = wrap(n)
        r_ok = '** HIT (raw)' if r == tid else ''
        w_ok = '** HIT (wrap)' if w == tid else ''
        print('  %-25s raw=0x%08x %s  wrap=0x%08x %s' % (n, r, r_ok, w, w_ok))
