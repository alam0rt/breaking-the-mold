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

names = ['PAUSED', 'NO', 'YES', 'QUIT', 'CONTINUE', 'QUIT GAME', 'QUITGAME', 'CURSOR']
for n in names:
    print('%-20s raw=0x%08x  wrap=0x%08x' % (repr(n), raw(n), wrap(n)))
