"""Hash every English word at L=5..8 and check against ALL low-floor uncracked targets.

This brutes the dictionary against all 13 low-floor targets simultaneously.
Real names that are dictionary words will surface here.
"""
import sys; sys.path.insert(0, 'tools/scripts')
from calchash import calcHash

def rotr(x, n): return ((x>>n)|(x<<(32-n)))&0xFFFFFFFF
SEED = 0x28C0E011

# All low-floor targets, with namespace
TARGETS = {
    # WRAP namespace (sprites): the brute target is rotr(id^SEED, 27)
    0x28a0c119: ('wrap', 5, 'sprite', 'duckling MENU'),
    0x38a0c119: ('wrap', 6, 'sprite', 'MENU sister'),
    0x30a0c119: ('wrap', 7, 'sprite', 'duckling 76-frame'),
    0x68c01218: ('wrap', 8, 'sprite', 'MENU'),
    0x088c5011: ('wrap', 7, 'sprite', 'FINN'),
    0x28c080df: ('wrap', 7, 'sprite', 'DEMO global'),
    0x21842018: ('wrap', 8, 'sprite', 'Klaymen FINN'),
    0x10882010: ('wrap', 8, 'sprite', 'EVIL'),
    0x2c182010: ('wrap', 8, 'sprite', 'MEGA boss body'),
    0x0c01c014: ('wrap', 8, 'sprite', 'GLEN'),
    0x04084011: ('wrap', 8, 'sprite', 'global 12-level'),
    # RAW namespace (audio):
    0x29808408: ('raw',  8, 'audio',  'global audio 22-level'),
    # type700 - try both
    0x1847c001: ('raw',  8, 'type700', 'try-raw'),
}

# Build target-set per namespace for quick lookup
raw_set = {}  # raw_target_hash -> list of (id, ...info)
wrap_set = {}
for tid, (ns, fl, typ, label) in TARGETS.items():
    if ns == 'raw':
        raw_set.setdefault(tid, []).append((tid, fl, typ, label))
    else:  # wrap
        inv = rotr(tid ^ SEED, 27)
        wrap_set.setdefault(inv, []).append((tid, fl, typ, label))
# Also try type700 in wrap
for tid, (ns, fl, typ, label) in TARGETS.items():
    if typ == 'type700':
        inv = rotr(tid ^ SEED, 27)
        wrap_set.setdefault(inv, []).append((tid, fl, typ, label+'-wrap'))

print(f'Brute namespaces: {len(raw_set)} raw targets, {len(wrap_set)} wrap targets')

# Hash every word and check both namespaces
hits = []
for length in [5, 6, 7, 8]:
    with open(f'/tmp/words{length}.txt') as f:
        words = [w.strip() for w in f if w.strip()]
    print(f'\n=== L={length} ({len(words)} words) ===')
    for w in words:
        h = calcHash(w)
        if h in raw_set:
            for info in raw_set[h]:
                hits.append((length, w, info))
                print(f'  RAW HIT: {w!r:<10s} -> 0x{info[0]:08x} (fl={info[1]}, {info[2]}, {info[3]})')
        if h in wrap_set:
            for info in wrap_set[h]:
                hits.append((length, w, info))
                print(f'  WRAP HIT: {w!r:<10s} -> 0x{info[0]:08x} (fl={info[1]}, {info[2]}, {info[3]})')

print()
print(f'Total: {len(hits)} hits')
print()
if hits:
    print('=== Summary by target ===')
    by_target = {}
    for L, w, info in hits:
        by_target.setdefault(info[0], []).append((L, w, info[3]))
    for tid, lst in sorted(by_target.items()):
        print(f'  0x{tid:08x}: {len(lst)} hits')
        for L, w, label in lst[:10]:
            print(f'      L={L} {w}  ({label})')
