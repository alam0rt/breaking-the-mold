#!/usr/bin/env python3
"""Verify suspected gem-related asset names."""
import sys, csv
sys.path.insert(0, 'tools/scripts')
from compound_hash_attack import asset_id

ids = set()
with open('docs/reference/asset-ids-master.csv') as f:
    next(f)
    for line in f:
        ids.add(int(line.split(',')[0], 16))

# Manual mentions: Common Gems, Rare Gems, Royal Gems, Greenheart, Yellow Chevron
candidates = [
    # Pure
    'GEM', 'GEMS',
    # Type-suffix variants
    'GEMCOMMON', 'COMMONGEM', 'COMMONGEMS', 'GEMSCOMMON', 'GEMS_COMMON', 'COMMON_GEM', 'GEM_COMMON',
    'GEMRARE', 'RAREGEM', 'RAREGEMS', 'GEMSRARE', 'GEMS_RARE', 'RARE_GEM', 'GEM_RARE',
    'GEMROYAL', 'ROYALGEM', 'ROYALGEMS', 'GEMSROYAL', 'GEMS_ROYAL', 'ROYAL_GEM', 'GEM_ROYAL',
    # Color variants
    'GEMRED','GEMBLUE','GEMGREEN','GEMYELLOW','GEMORANGE','GEMPURPLE','GEMWHITE','GEMBLACK',
    'REDGEM','BLUEGEM','GREENGEM','YELLOWGEM','ORANGEGEM','PURPLEGEM',
    # Numeric
    'GEM01', 'GEM02', 'GEM00', 'GEM1', 'GEM2', 'GEM3', 'GEMS01', 'GEMS00',
    # Power
    'SUPERGEM', 'GEMHIGH', 'GEMLOW', 'GEMBIG', 'GEMSMALL',
    # Other manual items
    'GREENHEART', 'YELLOWCHEVRON', 'CHEVRON', 'YELLOWHEART', 'HEART_YELLOW',
    'EXTRALIFE', 'EXTRA_LIFE', 'EXTRALIFEHEAD', 'KLAYHEAD', 'KLAYMENHEAD', 'ONEUP', 'LIFE',
    # Manual enemy names
    'PHARTHEAD','FARTHEAD','PHART','PHARTBALL','PHARTCLONE',
    'FATBOOMBOOM','FATTYBOOM','FATTYBOOMBOOM','BOOMBOOM',
    'KINGSKULLMONKEY','KINGMONKEY',
    'JUMPYGORILLA','GORILLA','JUMPY',
    'CLAYKEEPER','KEEPER',
    'LOUDMOUTH','LOUDMOUTHMONKEY',
    'TRIPLELASER','TRIPLE_LASER',
    'PIPECLEANER','PIPE_CLEANER',
    'FORKSHOOTER','FORK_SHOOTER',
    'HEADSHOOTER','HEAD_SHOOTER',
    'PHOENIX','PHOENIXHAND','PHOENIX_HAND',
    'JX1137','JX_1137','TESTPILOT','TEST_PILOT',
    'BARFO','ELBARFO','EL_BARFO',
    'MENTAL','MENTALMONKEY','MENTAL_MONKEY',
    'POPCORNSKULL','POPCORN_SKULL',
    'BARKINGBIRD','BARKING_BIRD',
    'EGGBEATER','EGG_BEATER',
    'TONGUEMONK','TONGUE_MONK','TONGUE',
    'CASTLETROOPER','CASTLE_TROOPER',
    'CENTURION','SWARM','TEMPEST',
    'ROBOTHOVER','ROBOT_HOVER',
    'WORKERYNT','WORKER_YNT','WORKERMONKEY',
    'INFERNO','INFERNOMONKEY','INFERNO_MONKEY','SCREAMINGINFERNO',
    'TURRET','LASERGUN','LASER_GUN',
    # Powerups
    'HAMSTERSHIELD','HAMSTER_SHIELD','HAMSTER',
    'UNIVERSEENEMA','UNIVERSE_ENEMA','SUPERPOWER','SUPER_POWER',
    'GREENBULLETS','GREEN_BULLETS','GREENBULLET','GREEN_BULLET',
    'YELLOWBIRD','YELLOW_BIRD','GLIDEY',
    'SUPERWILLIE','SUPER_WILLIE',
    # Klogg
    'KLOGG','KLOGGBALL','KLOGG_BALL','KLOGG1','KLOGG2','KLOGGFINAL',
]

print('=== HITS ===')
for n in candidates:
    h = asset_id(n)
    if h in ids:
        print(f'  HIT  {n:30s} -> 0x{h:08x}')

print(f'\n(checked {len(candidates)} candidates)')
