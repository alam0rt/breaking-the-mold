"""Verify the Skullmonkeys hash model:
  candidate A: id = calcHash(name)             (raw, like Neverhood)
  candidate B: id = 0x28C0E011 ^ rotl(calcHash(name), 27)

Anchors (verified from binary):
  NO        -> 0x29c0e211
  YES       -> 0x2ad0f011
  PAUSED    -> 0x0ad0f813
  QUIT      -> 0x68c0f413
  CONTINUE  -> 0x69c04050
  QUITGAME  -> 0x69c8f473

Klaymen hits (NEW evidence -- raw calcHash matches):
  Nev calcHash hash 0x5900c41e => SM id 0x5900c41e (Klaymen idle blink anim)
  Nev calcHash hash 0x9d406340 => SM id 0x9d406340 (Klaymen idle head sound)
  Nev calcHash hash 0x5860c640 => SM id 0x5860c640 (Klaymen jump-land sound)
"""

# Neverhood calcHash (verbatim from ScummVM resourceman.cpp).
# def calcHash(s):
#   hash = 0
#   for ch in s:
#     c = ord(ch)
#     if c >= ord('a'): c -= 32          # uppercase
#     if c >= ord('0') and c <= ord('9'): c += 22  # 0->'F', 9->'O'
#     elif c < ord('A') or c > ord('Z'): continue
#     hash ^= 1 << (c - ord('A'))        # toggle the (c-A)-th bit
#   return hash
# WAIT -- that's wrong. Real calcHash is NOT a bit-toggle. Let me recheck.

def calcHash_neverhood(s):
    """Neverhood calcHash from ScummVM resource.cpp lines 566-582 (verbatim).
    
        shiftValue is a CUMULATIVE counter; each char adds (ch - 64), wrapping mod 32.
    """
    h = 0
    shift = 0
    for ch in s:
        c = ord(ch)
        if ord('A') <= c <= ord('Z'):
            pass
        elif ord('a') <= c <= ord('z'):
            c -= 32
        elif ord('0') <= c <= ord('9'):
            c += 22
        else:
            continue
        shift = (shift + (c - 64)) & 31
        h ^= 1 << shift
    return h & 0xFFFFFFFF

# Let me test against the known anchors and the Klaymen hits:
print("=== Test single-bit-toggle ===")
for name, expected in [
    ("NO",       0x29c0e211),
    ("YES",      0x2ad0f011),
    ("PAUSED",   0x0ad0f813),
    ("QUIT",     0x68c0f413),
    ("CONTINUE", 0x69c04050),
    ("QUITGAME", 0x69c8f473),
]:
    got = calcHash_neverhood(name)
    print(f"  {name:12s} got=0x{got:08x}  expect=0x{expected:08x}  {'MATCH' if got == expected else 'no'}")

# Now verify the wrap formula
def rotl(v, r):
    r &= 31
    return ((v << r) | (v >> ((32 - r) & 31))) & 0xFFFFFFFF

print("\n=== Test wrap formula 0x28C0E011 ^ rotl(calcHash, 27) ===")
for name, expected in [
    ("NO",       0x29c0e211),
    ("YES",      0x2ad0f011),
    ("PAUSED",   0x0ad0f813),
    ("QUIT",     0x68c0f413),
    ("CONTINUE", 0x69c04050),
    ("QUITGAME", 0x69c8f473),
]:
    raw = calcHash_neverhood(name)
    wrapped = (0x28C0E011 ^ rotl(raw, 27)) & 0xFFFFFFFF
    print(f"  {name:12s} raw=0x{raw:08x} wrap=0x{wrapped:08x}  expect=0x{expected:08x}  {'MATCH' if wrapped == expected else 'no'}")
