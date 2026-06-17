import math

print('Brute math (36-symbol alphanumeric, kcrack ~300M tests/sec):')
print()
print(' L  candidates          time @ 300M/s')
for L in range(4, 16):
    n = 36**L
    s = n / 3e8
    if s < 1:
        t = f'{s*1000:.1f} ms'
    elif s < 60:
        t = f'{s:.1f} sec'
    elif s < 3600:
        t = f'{s/60:.1f} min'
    elif s < 86400:
        t = f'{s/3600:.1f} hr'
    elif s < 86400*365:
        t = f'{s/86400:.1f} day'
    else:
        t = f'{s/86400/365:.1f} yr'
    print(f' {L:>2}  {n:>18,}   {t}')

print()
print('Expected hits-per-ID (random uniform model, hash = 32 bits):')
print('  E[hits/target] = 36^L / 2^32     (kcrack hits any of N targets)')
print()
print(' L  hits/target    1195 targets total')
for L in range(6, 14):
    e = 36**L / (2**32)
    print(f' {L:>2}  {e:>15,.1f}   {e*1195:>20,.0f}')

print()
print('Real-world example: FX_PICKUP_1970 = 14 chars (12 alnum after _ removal)')
print('  Brute reach: realistic is L<=8 (~7s).')
print('  Verified known names average ~14 chars (FX_KLAY_FOOTSTEP_QUIET_SOFT = 23).')
print('  -> For target names of length L_real, only L<=L_real-2 anchor-pair brutes are feasible.')

print()
print('Subset attack with anchored substring (--pre P --suf S, ROOT="1970"):')
print('  Total length = P + 4 + S (root "1970" is 4 chars).')
print('  At P=4, S=4: total length 12, costs 36^8 = 2.8 trillion candidates.')
print('  Found 2.27M total preimages across 231 of 1195 targets.')
print('  ~9800 hits per target -> none distinguishable from noise.')

print()
print('What WOULD work:')
print('  - L=6 brute: 19s (513K hits, ~430/target). At L=6 most targets MISS,')
print('    so the 6-char preimages that DO hit are constrained.')
print('  - But the universe of 6-char alphanumeric is meaningless: most are gibberish.')
print('  - SIGNAL filter: --wordlike --overshoot 0 (exact-floor English-shaped strings).')
