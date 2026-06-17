"""Meet-in-the-middle preimage enumerator for the pickup-sprite PREFIX hash.

Constraint: calcHash(s) = TARGET_H (= 0x88200080) AND end_shift = TARGET_SH (= 27)

The hash is XOR of (1 << T_i) where T_i are cumulative shifts mod 32. For a
split s = A || B with |A|=ka, |B|=kb:
    h(s)        = h(A) XOR rotate_left(h_zero(B), shift(A))
    shift(s)    = (shift(A) + shift_zero(B)) % 32
where _zero subscripts mean "computed assuming initial shift is 0".

So we precompute a table B -> (shift_zero(B), h_zero(B)), keyed by that pair.
For each prefix A with (shift_A, h_A), the matching suffix must satisfy:
    shift_zero(B) = (TARGET_SH - shift_A) % 32
    h_zero(B)     = rotate_right(h_A XOR TARGET_H, shift_A)

Usage:
    python _prefix_mitm.py 8       # all L=8 preimages
    python _prefix_mitm.py 8 --contains PICKUP
    python _prefix_mitm.py 8 --letters-only
    python _prefix_mitm.py 8 --score 6
"""
import argparse
import itertools
import sys
import time

ALPHABET = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789'  # 36 symbols
STEP = [0] * 36
for d, c in enumerate(ALPHABET):
    o = ord(c)
    if c.isdigit():
        o += 22
    STEP[d] = (o - 64) & 31  # all in 1..26

TARGET_H = 0x88200080
TARGET_SH = 27


def rol32(x, n):
    n &= 31
    return ((x << n) | (x >> (32 - n))) & 0xFFFFFFFF if n else x & 0xFFFFFFFF


def ror32(x, n):
    n &= 31
    return ((x >> n) | (x << (32 - n))) & 0xFFFFFFFF if n else x & 0xFFFFFFFF


def calc(s):
    h, sh = 0, 0
    for c in s.upper():
        i = ALPHABET.find(c)
        if i < 0:
            continue
        sh = (sh + STEP[i]) & 31
        h ^= (1 << sh)
    return h & 0xFFFFFFFF, sh


def build_halves(klen):
    """Enumerate all strings of length klen, return dict
       (shift, hash) -> list of strings."""
    table = {}
    bytes_alpha = [c.encode() for c in ALPHABET]
    # Iterative DFS to avoid product overhead
    for tup in itertools.product(range(36), repeat=klen):
        sh = 0
        h = 0
        for d in tup:
            sh = (sh + STEP[d]) & 31
            h ^= (1 << sh)
        key = (sh, h)
        if key not in table:
            table[key] = []
        # Build string bytes
        table[key].append(bytes(ALPHABET[d] for d in tup))
    return table


# English-shape scoring (re-use from _prefix_rank if available)
COMMON_BIGRAMS = set([
    'TH','HE','IN','ER','AN','RE','ON','AT','EN','ND','TI','ES','OR','TE','OF',
    'ED','IS','IT','AL','AR','ST','TO','NT','NG','SE','HA','AS','OU','IO','LE',
    'VE','CO','ME','DE','HI','RI','RO','IC','NE','EA','RA','CE','LI','CH','LL',
    'BE','MA','SI','OM','UR','CA','EL','TA','LA','NS','DI','FO','HO','PE','EC',
    'PR','NO','CT','US','AC','OT','IL','TR','LY','NC','ET','UT','SS','SO','RS',
    'UN','LO','WA','GE','IE','WH','EE','WI','EM','AD','EX',
    'OB','BJ','JC','PI','CK','KU','UP','BO','SP','PR',
])
COMMON_TRIGRAMS = set([
    'THE','AND','ING','ION','TIO','ENT','ATI','FOR','HER','TER',
    'HAT','THA','ERE','ATE','HIS','CON','RES','VER','ALL','ONS',
    'NCE','MEN','ITH','TED','ERS','PRO','THI','WIT','ARE','ESS',
    'NOT','IVE','WAS','ECT','REA','COM','EVE','PER','INT','EST',
    'STA','CTI','ICA','IST','EAR','AIN','ONE','OUR','ITE','HAS',
    'OBJ','PIC','CKU','KUP','PIK','BNS','BON','GET','GRA','POW',
])
GAME_TOKENS = ['PICK','PCK','BNS','BONUS','POW','PWR','OBJ','SPR','SPRT',
               'PUP','GFX','SFX','BLB','HUD','UI','GET','GRAB','ITEM',
               'PROP','PRP','GFT','GIFT','MENU','MAIN','OBJECT','POOP',
               'POPP','TAKE','HOLD','COLLECT','MAGIC','SPELL','TYPE',
               'EGGS','SKULL','MONK','PLAY','HERO','KMEN','KLAY','CLAY']


def english_score(s):
    s = s.upper()
    score = 0
    n = len(s)
    if n < 2:
        return 0
    # bigrams
    bg = 0
    for i in range(n - 1):
        if s[i:i+2] in COMMON_BIGRAMS:
            bg += 1
    score += bg
    # trigrams
    tg = 0
    for i in range(n - 2):
        if s[i:i+3] in COMMON_TRIGRAMS:
            tg += 1
    score += tg * 2
    # vowel balance: 1 point if 25-65% are vowels
    vow = sum(1 for c in s if c in 'AEIOU')
    if 0.25 * n <= vow <= 0.65 * n:
        score += 1
    # game tokens
    for tok in GAME_TOKENS:
        if tok in s:
            score += 3
    # penalize all-digit or many digits
    digits = sum(1 for c in s if c.isdigit())
    if digits > n // 2:
        score -= 3
    if digits == 0:
        score += 1
    return score


def enumerate_preimages(L, contains=None, starts_with=None, ends_with=None,
                        letters_only=False, min_score=None, limit=None):
    ka = L // 2
    kb = L - ka
    print(f'Building suffix table for kb={kb}...', file=sys.stderr)
    t0 = time.time()
    # Build suffix table keyed on (suffix_shift_zero, suffix_hash_zero)
    suffix_table = {}
    for tup in itertools.product(range(36), repeat=kb):
        sh = 0
        h = 0
        for d in tup:
            sh = (sh + STEP[d]) & 31
            h ^= (1 << sh)
        key = (sh, h)
        if key not in suffix_table:
            suffix_table[key] = []
        suffix_table[key].append(tup)
    print(f'  suffix table: {sum(len(v) for v in suffix_table.values()):,} entries, '
          f'{len(suffix_table):,} keys, {time.time()-t0:.1f}s', file=sys.stderr)

    print(f'Scanning prefixes ka={ka}...', file=sys.stderr)
    t0 = time.time()
    results = []
    n_prefix = 0
    for tup in itertools.product(range(36), repeat=ka):
        sh_a = 0
        h_a = 0
        for d in tup:
            sh_a = (sh_a + STEP[d]) & 31
            h_a ^= (1 << sh_a)
        n_prefix += 1

        # Required suffix
        req_sh = (TARGET_SH - sh_a) & 31
        req_h = ror32(h_a ^ TARGET_H, sh_a)
        matches = suffix_table.get((req_sh, req_h))
        if not matches:
            continue

        prefix_str = ''.join(ALPHABET[d] for d in tup)
        if starts_with and not prefix_str.startswith(starts_with[:ka]):
            continue

        for suffix_tup in matches:
            suffix_str = ''.join(ALPHABET[d] for d in suffix_tup)
            cand = prefix_str + suffix_str

            # Apply filters
            if letters_only and any(c.isdigit() for c in cand):
                continue
            if contains and contains not in cand:
                continue
            if ends_with and not cand.endswith(ends_with):
                continue
            if min_score is not None:
                sc = english_score(cand)
                if sc < min_score:
                    continue
                results.append((sc, cand))
            else:
                results.append((0, cand))

            if limit and len(results) >= limit:
                break
        if limit and len(results) >= limit:
            break

    print(f'  scanned {n_prefix:,} prefixes, {len(results):,} hits, '
          f'{time.time()-t0:.1f}s', file=sys.stderr)
    return results


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('L', type=int, help='Length of preimage (even recommended)')
    ap.add_argument('--contains', default=None, help='Required substring')
    ap.add_argument('--starts-with', default=None)
    ap.add_argument('--ends-with', default=None)
    ap.add_argument('--letters-only', action='store_true')
    ap.add_argument('--score', type=int, default=None,
                    help='Minimum English-shape score')
    ap.add_argument('--top', type=int, default=50, help='Show top N by score')
    ap.add_argument('--limit', type=int, default=None,
                    help='Stop after N raw matches')
    args = ap.parse_args()

    results = enumerate_preimages(
        args.L,
        contains=args.contains,
        starts_with=args.starts_with,
        ends_with=args.ends_with,
        letters_only=args.letters_only,
        min_score=args.score,
        limit=args.limit,
    )

    # Verify a few
    for _, s in results[:5]:
        h, sh = calc(s)
        assert h == TARGET_H and sh == TARGET_SH, f'{s!r} fails verification!'

    # Show top by score
    if args.score is not None or args.top:
        results.sort(key=lambda x: (-x[0], x[1]))
    print(f'\n{len(results)} candidates found. Top {args.top}:')
    for sc, s in results[:args.top]:
        print(f'  score={sc:2d}  {s}')


if __name__ == '__main__':
    main()
