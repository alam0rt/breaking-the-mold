#!/usr/bin/env python3
"""Generate structured candidate names for the 15 FINN player-direction sprites.

Pipes plausible {prefix}_{direction}[_suffix] strings to stdout for klash_match_raw.

Targets (raw):
  0x10b85810 down
  0x10b91810 right slight down    0x10b9181c up left
  0x10b94810 right slight up      0x10b95010 up right
  0x10b9481c left slight down     0x10b95a10 up
  0x10b95a1c down slight left     0x10b95c10 up slight right
  0x10b95c1c down left            0x10b97810 right
  0x10b9781c left slight up       0x10b9d810 down right
  0x10b9d81c up slight left       0x10bb5810 down (#2)

Structural hints:
  - Floors are 9, 11, 13. Names need at least that many alnum chars.
  - 0x10b95a10 (up) vs 0x10b95a1c (dsl): differ in low byte 0x10 vs 0x1c
    -> 2 extra bits set => 2 extra chars added (or some chars cancel).
"""
import sys

ACTORS = [
    'FINN','PHART','PUFF','BABY','PUFFY','PUFFER','POOPER','POOTER','POOFY',
    'GAS','GASBABY','BABYGAS','PHARTBABY','BABYPHART','PUFFBABY','BABYPUFF',
    'PHARTPUFF','PUFFPHART','GASPUFF','PUFFGAS','POOPER',
    'PLAYER','RIDER','HERO','VEHICLE','PLAY','RIDE','BIRD','BIRDIE',
    'FBABY','FINNBABY','BABYFINN','BABYJET','JETBABY','BABYRIDE','RIDEBABY',
    'BIGFAT','BIGFATBABY','FATBABY','FATTY','FAT','FATTYBABY',
    'TADPOLE','TAD','POLE','TAD_POLE',
]

# Direction tokens — many variants
DIR_TOKENS = {
    'up':       ['UP','U','UPWARD','UPPER','TOP','RISE','CLIMB','ASCEND','TILT'],
    'down':     ['DOWN','DN','D','DOWNWARD','LOWER','BOT','BOTTOM','FALL','DIVE','DESCEND'],
    'left':     ['LEFT','LT','L','LEFTWARD','LEFTSIDE'],
    'right':    ['RIGHT','RT','R','RIGHTWARD','RIGHTSIDE'],
    'ur':       ['UR','UPRIGHT','UP_RIGHT','UPRT','URT','UPR'],
    'ul':       ['UL','UPLEFT','UP_LEFT','UPLT','ULT','UPL'],
    'dr':       ['DR','DOWNRIGHT','DOWN_RIGHT','DNRT','DRT','DR','DNRIGHT','DNR'],
    'dl':       ['DL','DOWNLEFT','DOWN_LEFT','DNLT','DLT','DNLEFT','DNL'],
    'rsu':      ['RIGHT_SLIGHT_UP','RIGHTSLIGHTUP','RSU','RIGHT_UP_SLIGHT',
                 'RIGHT_SHALLOW_UP','RIGHT_SOFT_UP','RIGHT_HALF_UP'],
    'rsd':      ['RIGHT_SLIGHT_DOWN','RSD','RIGHT_DOWN_SLIGHT','RIGHTSLIGHTDOWN'],
    'lsu':      ['LEFT_SLIGHT_UP','LSU','LEFTSLIGHTUP'],
    'lsd':      ['LEFT_SLIGHT_DOWN','LSD','LEFTSLIGHTDOWN'],
    'usr':      ['UP_SLIGHT_RIGHT','USR','UPSLIGHTRIGHT'],
    'usl':      ['UP_SLIGHT_LEFT','USL','UPSLIGHTLEFT'],
    'dsr':      ['DOWN_SLIGHT_RIGHT','DSR','DOWNSLIGHTRIGHT'],
    'dsl':      ['DOWN_SLIGHT_LEFT','DSL','DOWNSLIGHTLEFT'],
}

# Numeric/letter suffixes after direction
SUFFIXES = ['','_1','_2','_3','_0','_4','_A','_B','_C','_F0','_F1','_F2',
            '_F','_FRAME','_FR','_X','_Y','_Z','_LOOP','_END','_START',
            '_LO','_HI','_BIG','_SM',
            '_PLAYER','_BABY','_PHART','_PUFF','_FINN','_GAS','_VEHICLE',
            '_FRONT','_BACK','_SIDE',
            '1','2','3','4','5','A','B','C']

# Try MULTIPLE compound separator styles
SEPS = ['_','']

# Optional middle/connector words
MIDDLES = ['','_DIR','_FACING','_FACE','_GO','_TURN',
           '_AT','_TO','_TOWARD','_WAY']

emitted = set()
for actor in ACTORS:
    for sep1 in SEPS:
        for mid in MIDDLES:
            for dir_key, dir_variants in DIR_TOKENS.items():
                for dirtok in dir_variants:
                    for sfx in SUFFIXES:
                        s = actor + sep1 + mid + (sep1 if mid else '') + dirtok + sfx
                        if s not in emitted:
                            emitted.add(s)
                            print(s)
                            # also without sep
                            s2 = (actor + dirtok + sfx)
                            if s2 not in emitted:
                                emitted.add(s2)
                                print(s2)

print(f"# generated {len(emitted)} candidates", file=sys.stderr)
