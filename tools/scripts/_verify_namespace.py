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

# Audio names from cracked_names.csv
audio_known = [
    ('FX_KLAY_JUMP', 0x4a60ca40),
    ('FX_KLAY_LAND', 0x5860c640),
    ('FX_KLAY_FART', 0x50008340),
    ('FX_KLAY_HURT', 0x00e49240),
    ('FX_SKULL_LAND_01', 0x70404350),
    ('FX_SKULL_EAT', 0x7000c248),
    ('FX_MENU_SELECT', 0x90810000),
    ('FX_MENU_PASSWORD', 0x00880c1e),
    ('FX_PICKUP_GLIDEY', 0x6082c120),
    ('FX_PICKUP_SHIELD', 0xe0880448),
    ('FX_KLAY_FOOTSTEP_QUIET', 0x44d4c8d8),
    ('FX_KLAY_RUN_FAST', 0x00248e52),
    ('FX_KLAY_RUN_FAST_SOFT', 0x0434ce72),
    ('FX_KLAY_FOOTSTEP_QUIET_SOFT', 0x04dce858),
]

print('=== Audio name verification ===')
for n, expected in audio_known:
    r = raw(n)
    w = wrap(n)
    r_ok = '*' if r == expected else ' '
    w_ok = '*' if w == expected else ' '
    print('  %-30s expected=0x%08x  raw=0x%08x %s  wrap=0x%08x %s' % (n, expected, r, r_ok, w, w_ok))
