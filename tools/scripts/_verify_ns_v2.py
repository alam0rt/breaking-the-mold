"""Verify which namespace each sprite/audio anchor uses."""
import sys
sys.path.insert(0, 'tools/scripts')
from calchash import calcHash

def rotl(v, r):
    r &= 31
    return ((v << r) | (v >> ((32 - r) & 31))) & 0xFFFFFFFF

def wrap(s):
    return 0x28C0E011 ^ rotl(calcHash(s), 27)

def raw(s):
    return calcHash(s)

print('=== sprite anchors (NO/YES/PAUSED/QUIT/CONTINUE/QUITGAME) ===')
anchors = {
    'NO':       0x29c0e211,
    'YES':      0x2ad0f011,
    'PAUSED':   0x0ad0f813,
    'QUIT':     0x68c0f413,
    'CONTINUE': 0x69c04050,
    'QUITGAME': 0x69c8f473,
}
for n, t in anchors.items():
    r, w = raw(n), wrap(n)
    rh, wh = ('YES' if r == t else 'no'), ('YES' if w == t else 'no')
    print(f'  {n:<10s} target=0x{t:08x}  raw=0x{r:08x}({rh})  wrap=0x{w:08x}({wh})')

print()
print('=== scummvm anchor sprite: Klaymen_idleblink_anim target=0x5900c41e ===')
n = 'Klaymen_idleblink_anim'
r, w = raw(n), wrap(n)
print(f'  raw=0x{r:08x}  wrap=0x{w:08x}')

print()
print('=== scummvm anchor audio: Klaymen_idlehead_sound target=0x9d406340 ===')
n = 'Klaymen_idlehead_sound'
r, w = raw(n), wrap(n)
print(f'  raw=0x{r:08x}  wrap=0x{w:08x}')

print()
print('=== a few audio FX anchors (expected RAW) ===')
audio = {
    'FX_KLAY_JUMP':       0x4a60ca40,
    'FX_KLAY_LAND':       0x5860c640,
    'FX_KLAY_FART':       0x50008340,
    'FX_KLAY_HURT':       0x00e49240,
    'FX_MENU_PASSWORD':   0x00880c1e,
    'FX_PICKUP_PHOENIX':  0x44c26454,
    'FX_PICKUP_GROW':     0x40864668,
    'FX_PICKUP_ONE_UP':   0x428254e2,
    'FX_PICKUP_1970':     0x408a6461,
    'FX_PICKUP_1970_03':  0x428a6465,
}
for n, t in audio.items():
    r, w = raw(n), wrap(n)
    rh, wh = ('YES' if r == t else 'no'), ('YES' if w == t else 'no')
    print(f'  {n:<22s} target=0x{t:08x}  raw=0x{r:08x}({rh})  wrap=0x{w:08x}({wh})')
