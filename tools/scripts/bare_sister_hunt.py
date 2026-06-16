#!/usr/bin/env python3
"""Bare-style sister hunt for the 3 unverified candidates."""

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

SKULL_VERBS = ['UP','DN','DOWN','TAP','GRAB','HOLD','LOOK','TURN','SLEEP','WAKE','BLINK','TALK','SHOUT','LAUGH','CRY','SING','HUM','BUZZ','HISS','GROWL','BARK','HOWL','PURR','SNORE','GASP','PANT','CLICK','TICK','TOCK','POP','ZIP','BUMP','THUD','BOOM','BANG','PLOP','SPLAT','PLOOP','BLOP','BLOOP','SPLISH','SPLASH','DRIP','DROP','BUBBLE','POOF','PFFT','WHOOSH','SWISH','WOOSH','SWOOSH','GULP','SWALLOW','PEEK','PEEP','LURK','SCOUT','SPY','HUNT','CHASE','RUN','SCURRY','DASH','LEAP','HOP','SKIP','BOUNCE','SLIDE','GLIDE','SOAR','FLAP','FLY','LAND','TAKEOFF','PAUSE','BIRTH','LIFE','DEATH','GRIN','SMILE','FROWN','RIBBIT','CROAK','CHIRP','TWEET','SQUEAK','RATTLE','SHIVER','SHAKE','VIBRATE','RUMBLE','TREMBLE','QUAKE']
KLAY_VERBS = ['WET','DRY','HOT','COLD','BURN','FREEZE','HEAL','REVIVE','RESPAWN','RESTART','TRIP','BUMP','SNORE','PEE','POO','BURP','SNEEZE','COUGH','HICCUP','YAWN','LICK','CHEW','BITE','SPIT','GULP','SLURP','SUCK','BLOW','PUFF','SUFF','CUSS','SCREAM','YELL','HOWL','LAUGH','CHUCKLE','GIGGLE','CRY','SOB','GROAN','GRUNT','MOAN','SIGH','GASP','BREATHE','THINK','LISTEN','LOOK','SEE','BLINK','WINK','SNICKER','TICKLE','SCRATCH','SHAKE','TWITCH','TURN','SPIN','SQUAT','DUCK','BEND','REACH','GRAB','HOLD','THROW','TOSS','PUNCH','KICK','SLAP','HIT','POKE','PUSH','PULL','LIFT','DROP','PICKUP','CARRY','RIDE','MOUNT','DISMOUNT','CRAWL','CLIMB','HANG','SWING','SLIDE','GLIDE','TUMBLE','ROLL','TUMBL','LEAP','HOP','SKIP','TAP','STOMP','STAMP','SLAM','HEEL','TOE','TIPTOE','SPLASH','DOUSE','SOAK','DRIP','DROWN','SINK','SWIM','PADDLE','WADE','WET','DAMP','MUDDY']
PUFF_VERBS = ['FALL','RISE','GROW','SHRINK','POP','BURST','EXPAND','EXPLODE','BLOW','PUFF','BLOOM','DEFLATE','INFLATE','FLOAT','HOVER','DRIFT','WAFT','DROP','BUMP','SPIN','TWIRL','TUMBLE','ROLL','BOUNCE','LAND','HIT','CRASH','POOF','PFFT','WHOOSH','SWISH','BUBBLE','PLOP','PLOOP','BLOP','SPLAT','SPLISH','SPLASH','DRIP','LEAK','FILL','EMPTY','VENT','SEEP','FART','SLURP','SUCK','GULP','SWALLOW','BREATHE','GASP','PANT','SIGH','SNORE','HUM','SING','TALK','LAUGH','SCREAM','HISS','GROWL','BARK','BUZZ']

targets = {
    0x30004240: '(BRG1, candidate-SKULL)',
    0xc0608a40: '(12 levels, candidate-KLAY)',
    0x40e0824c: '(FINN, candidate-PUFF)',
    0x68009240: '(FX_SKULL_FLAP committed)',
}

# Test bare-style names: FX_<actor>_<verb>
for actor, verbs in [('SKULL', SKULL_VERBS), ('KLAY', KLAY_VERBS), ('PUFF', PUFF_VERBS)]:
    print(f'\n== FX_{actor}_<verb> ==')
    for v in verbs:
        s = f'FX_{actor}_{v}'
        h = calcHash(s)
        if h in targets:
            print(f'  {s:25s} -> 0x{h:08x}  {targets[h]}')

# Also try with no prefix and other actors
print('\n== Alt actor sweep ==')
for actor in ['BIRD','RAT','GUM','BUG','FROG','TOAD','SLUG','SNAKE','WORM','BAT','MOTH','SPIDER','SCORPION','SCRAB','LARVA','GRUB','PUFF','MOOSE','DOG','CAT','HOG','PIG','CRAB','FISH','BIRD','EAGLE','HAWK','OWL','WOLF','BEAR','LION','PIRA','OOZE','BLOB','SLIME','SLUDGE','GOO','MUCK','GUNK','SPLAT','MUD']:
    for v in PUFF_VERBS + SKULL_VERBS:
        s = f'FX_{actor}_{v}'
        h = calcHash(s)
        if h in targets:
            print(f'  {s:30s} -> 0x{h:08x}  {targets[h]}')
