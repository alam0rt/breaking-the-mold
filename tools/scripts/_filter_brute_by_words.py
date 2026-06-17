"""Filter kcrack brute output for preimages containing recognizable substrings.

The kcrack brute generates 36^L candidate strings. At L=8 each target gets
~657 hits (collision noise). The filter idea: scan all hits and keep ONLY
strings that contain at least one game-relevant word from the vocab list.

Inputs:
  /tmp/preimages_l*.txt  — kcrack output (id\tname\tfloor=F\tlen=L)
  tools/data/game_vocab.txt + game_focused_wordlist.txt

Outputs:
  /tmp/filtered_<L>.txt  — hits whose name contains a known token
"""
import sys, re, csv
from pathlib import Path
from collections import defaultdict

ROOT = Path(__file__).resolve().parents[2]
VOCAB_FILE = ROOT / 'tools/data/game_vocab.txt'
FOCUSED_FILE = ROOT / 'tools/data/game_focused_wordlist.txt'

# Load game-vocab words (all caps, alphanumeric only, length 3-7)
words = set()
for src in [VOCAB_FILE, FOCUSED_FILE]:
    with src.open() as f:
        for line in f:
            w = line.strip().upper()
            # Remove non-alnum
            w = re.sub(r'[^A-Z0-9]', '', w)
            if 3 <= len(w) <= 8 and w:
                # Skip pure-digit garbage
                if not w.isdigit() or len(w) >= 4:
                    words.add(w)

# Add "1970" and other key tokens
for k in ('1970','KLAY','KLOG','GLEN','WIZZ','FINN','MEGA','BOIL','SNOW',
          'PHRO','SCIE','TMPL','BRG1','CAVE','WEED','EGGS','CLOU','SEVN',
          'SOAR','CRYS','CSTL','RUNN','MOSS','EVIL','GLID','HEAD',
          'PAUSED','QUIT','CONTINUE','YES','NO','PRESS','START','MENU',
          'DEMO','LOAD','SAVE','PLAY','TITLE','GAME','PASS','OK','SKULL',
          'MONKEY','CLAY','BALL','LIFE','PHART','FART','BURP','HURL',
          'JOE','GUM','BIRD','RAT','PUFF','SHIELD','PHOENIX','HAMSTER',
          'GLIDEY','WILLIE','FARTHEAD','SLAPPY','SWIRLY','UNIVERSE',
          'JUMP','FALL','LAND','WALK','RUN','RUNS','HIT','HURT','DIE',
          'IDLE','STAND','EAT','BLINK','SHOOT','FIRE','SPIN','TURN',
          'OPEN','CLOSE','REST','WAIT','SPAWN','GROW','PICK','GRAB',
          'TAKE','GIVE','GET','LOSE','WIN','LOST','WON','EXIT','ENTER',
          'CURSOR','POINTER','ARROW','BUTTON','TEXT','FONT','DIGIT','NUM',
          'NUMBER','SCORE','BONUS','HP','LIFE','LIVES','HEALTH','POWER',
          'COIN','GEM','HEART','STAR','RING','SHIELD','HALO','ICON',
          'SHRINE','GUARD','TROOP','MAGE','BOSS','ENEMY','PLAYER','ACTOR',
          'TUTORIAL','HINT','SIGN','BANNER','BOARD','TOTEM','CHECKPOINT',
          'CONTROL','MAP','PAD','JOYPAD','DPAD','OPTIONS','CONFIG',
          'AUDIO','VIDEO','SOUND','MUSIC','SFX','LANG','VOL'):
    words.add(k)

print(f'Vocabulary: {len(words)} words')

# Build a regex for fast substring search
# Sort by length desc so longer matches win
words_sorted = sorted(words, key=lambda w: (-len(w), w))
pattern = re.compile('|'.join(re.escape(w) for w in words_sorted))

# Process each input
import os
inputs = sys.argv[1:]
if not inputs:
    inputs = ['/tmp/preimages_l6.txt', '/tmp/preimages_l7.txt']

for inp in inputs:
    if not os.path.exists(inp):
        print(f'  skip (missing): {inp}')
        continue
    out = inp.replace('preimages_', 'filtered_')
    counts = defaultdict(int)
    matched = 0
    total = 0
    with open(inp) as f, open(out, 'w') as fo:
        for line in f:
            total += 1
            parts = line.rstrip('\n').split('\t')
            if len(parts) < 2:
                continue
            name = parts[1]
            m = pattern.search(name)
            if m:
                fo.write(line)
                matched += 1
                counts[parts[0]] += 1
    print(f'{inp}: {matched}/{total} kept (matched word) -> {out}')
    print(f'  Top-hit IDs (most filtered preimages):')
    top = sorted(counts.items(), key=lambda kv: -kv[1])[:10]
    for idv, n in top:
        print(f'    {idv}  hits={n}')
