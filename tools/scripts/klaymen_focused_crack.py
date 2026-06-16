"""Focused brute force on the 3 confirmed Klaymen IDs.
We know they exist in BOTH Neverhood and Skullmonkeys binaries.
The string was used by the ORIGINAL devs (TenNapel/Dietz) at Neverhood Inc.
"""
import itertools

def calcHash(s):
    h, sh = 0, 0
    for ch in s:
        c = ord(ch)
        if 65 <= c <= 90: pass
        elif 97 <= c <= 122: c -= 32
        elif 48 <= c <= 57: c += 22
        else: continue
        sh = (sh + c - 64) & 31
        h ^= 1 << sh
    return h & 0xFFFFFFFF

TARGETS = {
    0x5900c41e: 'IdleBlink animation',
    0x9d406340: 'IdleHeadOff sound',
    0x5860c640: 'JumpLand sound',
}

# Build a vocabulary of likely tokens used in TenNapel's Neverhood naming convention
TOKENS = {
    # Klaymen variants
    'KLAYMEN','KLAYMAN','KLAY','KLM','KMAN','PLAYER','PL','GUY','DUDE','HERO','MAIN','TOON',
    # Verbs (idle/jump variants)
    'IDLE','BLINK','BLNK','HEAD','OFF','LAND','LANDING','JUMP','JUMPING','JMP','LANDED',
    'STAND','REST','BREATHE','BREATH','LOOK','TURN','SLEEP','WAIT','PAUSE','EYE','EYES',
    'FALL','FALLING','DROP','PLOP','THUD','SLAM',
    # Asset categories
    'ANIM','ANIMATION','ANI','SND','SOUND','AUDIO','SFX','VOICE','VO','VOX','SAMPLE',
    'SPRITE','SPR','IMG','IMAGE','PIC','FRAME','SEQ','SEQUENCE',
    # Body parts
    'EYE','EYES','MOUTH','HAND','FOOT','FEET','BODY','HEAD','FACE','ARM','LEG',
    # Variants
    'A','B','C','LFT','RGT','UP','DN','SM','LG','BIG','SMALL','REG',
    '1','2','3','01','02','03','001','002','003',
}

def tokens_to_strs(tokens, depth=2, sep=''):
    """Generate every depth-N concatenation of tokens with separators."""
    seen = set()
    for combo in itertools.product(tokens, repeat=depth):
        s = sep.join(combo)
        if s not in seen:
            seen.add(s)
            yield s

# Try depth 2 and 3 with various separators
hits_per_id = {t: [] for t in TARGETS}

count = 0
for depth in (2, 3, 4):
    for sep in ('', '_'):
        for s in tokens_to_strs(TOKENS, depth, sep):
            h = calcHash(s)
            if h in hits_per_id:
                hits_per_id[h].append(s)
            count += 1

print(f"Tried ~{count:,} concatenations")
print()
for h, label in TARGETS.items():
    matches = hits_per_id[h]
    print(f"  0x{h:08x}  {label}  -> {len(matches)} matches")
    for s in matches[:20]:
        print(f"    {s}")
