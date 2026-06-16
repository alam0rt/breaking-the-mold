"""Correct Python calcHash matching klash_brute_raw.c."""

CS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
# STEP[d] for digit index d: based on ASCII of char with +22 offset for digits
STEP = [0]*36
for d, c in enumerate(CS):
    o = ord(c)
    if c.isdigit():
        o += 22
    STEP[d] = (o - 64) & 31

def calcHash(s: str) -> int:
    """Raw namespace hash: case-insensitive, alnum only, per-char STEP shift, single-bit XOR."""
    h = 0
    sh = 0
    for c in s.upper():
        if c in CS:
            d = CS.index(c)
            sh = (sh + STEP[d]) & 31
            h ^= 1 << sh
    return h & 0xFFFFFFFF


if __name__ == "__main__":
    # Verify
    known = [
        ("FX_KLAY_DUCK_DOWN", 0x4428c941),
        ("FX_SKULL_EAT", 0x7000c248),
        ("FX_SKULL_UP", 0x30004240),
        ("FX_BLOW_GALE_SM", None),
    ]
    for name, expected in known:
        h = calcHash(name)
        if expected is None:
            print(f"  {name!r}: 0x{h:08x}")
        else:
            ok = "OK" if h == expected else "FAIL"
            print(f"{ok} {name!r}: got 0x{h:08x} expected 0x{expected:08x}")
