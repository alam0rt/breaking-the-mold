"""Grep the 2996 preimages for common English / game-meaningful subsequences."""
import re

with open('/tmp/prefix.html') as f:
    page = f.read()
m = re.search(r'var ALL=\[(.+?)\];', page)
preimages = re.findall(r'"([^"]+)"', m.group(1))
print(f'Loaded {len(preimages)} preimages')

# Patterns to search
patterns = [
    'OBJ','OBJS','PICK','PCK','PUP','BNS','BONUS','POW','PWR','SPRT','SPR','SPRITE',
    'ITEM','GET','GRAB','TAKE','HAVE','HOLD','MAIN','GAME','MENU','BASE','TYPE',
    'BLB','GFX','SFX','FX','SND','PROP','GIFT','LOOT','DROP','GAIN','BOOST',
    'HERO','PLAY','KLAY','KLY','KMEN','MARK','TAG','POW','POWER','EXTRA','PERK',
    'COIN','GEM','GOLD','RING','KEY','LIFE','HP','HEAL','MAG','POT','POT',
    'PCKUP','PICKER','BNS01','OBJ01','BLB01','PCK01','OBJ_','PCK_',
    'ITEM01','ITEM1','MISC','GLOB',
    'GLIDE','GLIDEY','FART','FARTH','HEAD','HEADS','HEAR','HEART','GROW','UNI',
    'UNIV','UNIVR','UNIVRS','PHOEN','PHOENIX','WILLIE','WILL','SHIELD','SHLD',
    'TAKEABLE','TAKEABL','TKBL','LIFTABLE','LIFTABL','LFTBL',
    'PRIZE','PRIZ','WARD','REWARD','RWARD','TREAS','TREASR','TREASURE',
    'POPPER','POPPED','HOPPER','GETTER','HOLDER','KEEPER','MARKER','MARKD',
    'CRAFT','CRAFTD','MADE','MAKER','MAKES','MAKERS','MAKE','MAKR',
]

for pat in patterns:
    matches = [p for p in preimages if pat in p]
    if matches:
        print(f'\n  Pattern {pat!r}: {len(matches)} matches')
        for m in matches[:8]:
            print(f'    {m}')

# Also: just show 30 preimages with NO digits
print('\n=== All-letter preimages (no digits) ===')
no_digits = [p for p in preimages if not any(c.isdigit() for c in p)]
print(f'  {len(no_digits)} all-letter preimages:')
for p in sorted(no_digits):
    print(f'    {p}')
