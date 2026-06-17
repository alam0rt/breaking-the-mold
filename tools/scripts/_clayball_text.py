"""Test verbatim text content for the CLAY BALL instructions sprite."""
import sys
sys.path.insert(0, 'tools/scripts')
from calchash import calcHash

TARGET = 0x80729081  # 'CLAY BALL/COLLECT 100/OF THESE FOR/A FREE LIFE' instruction text

# Verbatim text variations (calcHash strips non-alnum so spaces, slashes, punctuation all = same)
texts = [
    'CLAY BALL COLLECT 100 OF THESE FOR A FREE LIFE',
    'CLAY BALL',
    'COLLECT 100',
    'COLLECT 100 OF THESE',
    'COLLECT 100 OF THESE FOR A FREE LIFE',
    'FREE LIFE',
    'A FREE LIFE',
    'OF THESE FOR A FREE LIFE',
    'CLAY BALL COLLECT 100',
    'CLAYBALL COLLECT 100 OF THESE FOR A FREE LIFE',
    'CLAY BALLS',
    # without spaces
    'CLAYBALLCOLLECT100',
    'CLAYBALL100',
    '100CLAYBALL',
    '100CLAYBALLS',
    # Standard sprite text naming
    'CLAYBALL_INSTRUCTIONS',
    'CLAY_BALL_INSTRUCTIONS',
    'CLAYBALLINSTRUCTIONS',
    'CLAY_BALL_MSG',
    'CLAYBALLMSG',
    'CLAYBALL_INSTR',
    'CLAY_BALL_INSTR',
    # Possible variants
    'CLAYBALL_HELP',
    'CLAYBALL_HOWTO',
    'EXTRA_LIFE_HINT',
    'EXTRA_LIFE_INSTR',
    'EXTRA_LIFE_MSG',
    'CLAYBALL_SIGN',
    'CLAY_BALL_SIGN',
    'INSTRUCTION_SIGN',
    'SIGN_CLAYBALL',
    'CLAYBALL_INFOBOX',
    # Just the keyword
    'INSTRUCTIONS',
    'INSTRUCTION',
    'SIGN',
    'HOWTO',
    'HELP',
    'HINT',
    'TUTORIAL',
    'TIP',
    'INFO',
    'MESSAGE',
    'BANNER',
    'NOTE',
    'INTRO',
    'NARRATION',
    # With CLAY prefix
    'CLAY_INTRO',
    'CLAY_NOTE',
    'CLAY_INFO',
    'CLAY_INSTR',
    'CLAY_BANNER',
    'CLAY_HOWTO',
    'CLAY_TIP',
    # Tagged
    'TXT_CLAYBALL',
    'TEXT_CLAYBALL',
    'TXT_INSTRUCTIONS',
    'SIGN_CLAY',
    # With FX prefix (unlikely for sprite but try)
    'FX_CLAYBALL_INFO',
    'UI_CLAYBALL_INFO',
    'TXT_CB_INFO',
]

print('TARGET=0x%08x' % TARGET)
for n in texts:
    h = calcHash(n)
    if h == TARGET:
        print('  HIT: %r -> 0x%08x' % (n, h))
print('done')
