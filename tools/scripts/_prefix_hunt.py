"""Verify candidate pickup-sprite PREFIX strings against the known constraint:
   calcHash(PREFIX) = 0x88200080, end-shift = 27.

The real PREFIX produces sprite_id = calcHash(PREFIX + ITEM) for items like
FARTHEAD/GROW/ONE_UP/UNIVERSE_ENEMA_1/WILLIE. The Phart-Head sprite ID is
0x8c510186, so PREFIX+'FARTHEAD' hashes to that.
"""
import sys
sys.path.insert(0, 'tools/scripts')

CS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
STEP = [0]*36
for d, c in enumerate(CS):
    o = ord(c)
    if c.isdigit():
        o += 22
    STEP[d] = (o - 64) & 31

def calc(s):
    """Return (hash, sh) — both case-insensitive + strips non-alnum."""
    h, sh = 0, 0
    for c in s.upper():
        if c in CS:
            d = CS.index(c)
            sh = (sh + STEP[d]) & 31
            h ^= 1 << sh
    return h & 0xFFFFFFFF, sh

TARGET_H = 0x88200080
TARGET_SH = 27
PHARTHEAD = 0x8c510186  # calcHash(PREFIX + 'FARTHEAD')

# Verify our setup with the page-provided info
# The page says PREFIX is 4 or 6 chars (long alphanumeric — strip _ and uppercase).
# Real prefix is likely 4 or 6 alphanumeric chars.

candidates = [
    # Obvious naming-convention guesses
    'PICKUP','PUP','GET','GRAB','ITEM','ITEMS','OBJ','OBJS','OBJECT','OBJECTS',
    'BONUS','POWERUP','POWER','PWR','GIFT','TREAT','PRIZE','LOOT','DROP','DROPS',
    'SPAWN','SPRITE','SPRT','PICK','GET','TAKE','HAVE','OWN','CARRY','HOLD',
    # File-system style
    'S_OBJ','SOBJ','S_BNS','SBNS','S_PUP','SPUP','S_ITM','SITM',
    'I_','I_OBJ','IOBJ','OBJ_','OBJS_',
    # Klay-themed
    'KLAY','KMEN','KLAYP','KLAYPUP','KP','KPUP','KGET','KOBJ','KBNS','KBONUS',
    'KLY','KPCK','KLAYPCK','KLPCK','KLPUP','KLB','KLBN','KLBONUS',
    # Audio-prefix-mirror
    'FX','SFX','SND','S','M','FX_','SFX_','MF','MN','MX',
    'SPR','SPR_','SPRITE_','SPRITES_',
    # Common 4-char and 6-char abbrevs
    'GAME','MAIN','BASE','TYPE','MISC','THNG','THING','THINGS','GLOB','GLBL','GLOB',
    'IT','IT_','ITEMS_','THIS','THAT','HERE','THERE','NEW','OLD',
    # Phrases (alphanumeric only)
    'BLB','BLBS','BLBP','BLBPUP','MAINBNS','MAINPUP',
    # File-extension-style
    'TIM','BS','VAB','XA','STR','MOV','TXT','BIN','SPRITE','ANIM','SND',
    # 4-char single English-ish
    'DROP','PROP','TOOL','TOOLS','PROP','PROPS','GAIN','GIVE','GAVE','HAVE',
    'PICK','MARK','MARKS','MARKER','MARKERS',
    # Game-asset standards (compact)
    'GFX','VFX','PFX','PFC','PCK','PFP','OBJ','OBJS',
    # Skullmonkeys-specific
    'BABY','HERO','PROT','PROTO','CLAY','CLAYS','BALL','BALLS',
    'MENU','MNU','UI','HUD','GUI',
    # Concatenated
    'OBJPUP','OBJPICK','OBJBNS','OBJBONUS','BNSOBJ','BONUSOBJ','BONUSPUP',
    'OBJECTPICKUP','PICKUPOBJ','PICKUPOBJECT','ITEMOBJ','OBJITEM',
    # The cracked audio uses FX_PICKUP_, sprite might use S_PICKUP_ or M_PICKUP_
    'SPICKUP','MPICKUP','BPICKUP','BPICK','BPCK','PPCK','PPICK',
    # The famous JOE / JOE_HEAD_JOE / boss-naming convention
    'JOE','JJOE','JHJ','JHJOE',
    # The "object" prefix
    'OB','OBJ','OBJECT',
    # Test the canonical: PICKUP
    'PICKUP',
]
# Add Klaymen/Skullmonkeys all-3-letter abbrevs
for a in 'ABCDEFGHIJKLMNOPQRSTUVWXYZ':
    for b in 'ABCDEFGHIJKLMNOPQRSTUVWXYZ':
        for c in 'ABCDEFGHIJKLMNOPQRSTUVWXYZ':
            pass  # too many; do it programmatically below

# Test the manual list first
print('=== Verifier mode: test each candidate ===')
print(f'Target: calcHash={TARGET_H:#010x}, sh={TARGET_SH}')
print(f'Verify: calcHash(PREFIX + "FARTHEAD") must equal 0x{PHARTHEAD:08x}')
print()
matched = []
for c in candidates:
    h, sh = calc(c)
    full_h, _ = calc(c + 'FARTHEAD')
    pharthead_match = (full_h == PHARTHEAD)
    if h == TARGET_H and sh == TARGET_SH:
        print(f'  *** VALID PREFIX: {c!r} (h={h:#010x}, sh={sh})')
        print(f'      calcHash({c!r}+"FARTHEAD") = 0x{full_h:08x} {"== 0x8c510186 OK" if pharthead_match else "MISMATCH"}')
        matched.append(c)
    elif pharthead_match:
        print(f'  WARNING: {c!r} gives FARTHEAD match but h={h:#010x},sh={sh}')

print()
print(f'Matched: {len(matched)} candidates')
print()

# Now enumerate the 2996 preimages from the page and search for English-shaped
# names. We'll fetch them from /tmp/prefix.html.
print('=== Looking for English-shaped 4 & 6 char preimages ===')
import re
with open('/tmp/prefix.html') as f:
    page = f.read()
m = re.search(r'var ALL=\[(.+?)\];', page)
if m:
    preimages = re.findall(r'"([^"]+)"', m.group(1))
    print(f'Loaded {len(preimages)} preimages from page')
    
    # Verify a few hash correctly
    sample_ok = 0
    for p in preimages[:20]:
        h, sh = calc(p)
        if h == TARGET_H and sh == TARGET_SH:
            sample_ok += 1
    print(f'Verified first 20: {sample_ok}/20 hash to target+sh')
    
    # Look for English vowels — names with at least 2 vowels in 4-6 char strings
    vowels = set('AEIOU')
    likely = []
    for p in preimages:
        if 1 <= len([c for c in p if c in vowels]):
            # Has at least 1 vowel
            # Filter: no 3+ consecutive digits, no 3+ consecutive same char
            ok = True
            digs = 0
            for c in p:
                if c.isdigit():
                    digs += 1
                    if digs >= 3:
                        ok = False
                        break
                else:
                    digs = 0
            if ok:
                likely.append(p)
    print(f'  Filtered to "{len(likely)}" with vowel and no 3+digit run')
    
    # Show those with high vowel density (>=33%)
    print('  Sample (vowel-rich):')
    for p in likely:
        nv = sum(1 for c in p if c in vowels)
        if nv >= len(p) * 0.33 and not any(c.isdigit() for c in p):
            print(f'    {p}')
