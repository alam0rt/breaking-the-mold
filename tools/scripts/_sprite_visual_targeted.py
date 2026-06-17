"""Search for sprite names hinted by visual review sheets."""
import sys
sys.path.insert(0, 'tools/scripts')
from calchash import calcHash

# === Visual identifications from review_sheets/sheet_00.png ===
TARGETS = {
    0x80729081: 'sprite "CLAY BALL/COLLECT 100/OF THESE FOR/A FREE LIFE" instruction text',
    0x88329285: 'sprite checkpoint arrow (InitCheckpointEntity)',
    0xc9387d8c: 'sprite 1970 hamster icon (TeleporterActivate context, but visually = hamster)',
    0x9389b8c4: 'sprite teleporter activate (Klogg ball / teleport orb)',
    0x0bbb70ac: 'sprite teleporter activate (purple oval)',
    0x4260c8b5: 'sprite Klay-debris small red clay blob',
    0x4262c6a0: 'sprite hand (red) PlayerCallback_8005cce0',
    0x46346040: 'sprite hand (blue) Callback_800570a4',
}

# Possible name candidates (organized by category)
NAMES = [
    # Clay Ball / 100 collectible text
    'CLAYBALL','CLAY_BALL','CLAYBALLS','CLAY_BALLS','CLAYBALLTEXT','CLAY_BALL_TEXT',
    'COLLECT100','COLLECT_100','COLLECT_100_TEXT','CLAYBALL_INSTR','CLAY_BALL_INSTR',
    'CLAY_BALL_INSTRUCTION','CLAY_BALL_INSTRUCTIONS','CLAYBALL_INSTRUCTION',
    'INSTR_CLAYBALL','CLAYBALL_TEXT','CLAY_TEXT','CLAYBALL_INFO','CLAY_INFO',
    'FREE_LIFE','FREELIFE','EXTRALIFE','EXTRA_LIFE','EXTRA_LIVES',
    'PICKUP_INSTRUCTIONS','COLLECT_CLAYBALL','COLLECT_CLAY_BALL',
    'COLLECT100_TEXT','COLLECT_100C','C100','C100T','C100_TEXT',
    # Checkpoint
    'CHECKPOINT','CHECKPOINT_TEXT','CHECKPOINT_ARROW','CHECKPOINT_SPRITE',
    'CHKPT','CKPT','SPRT_CHECKPOINT','CHECKPOINT_ICON',
    # Teleporter
    'TELEPORTER','TELEPORT','TELEPORT_BALL','TELEPORTBALL','TELE','TELE_BALL',
    'TELEORB','TELE_ORB','PORTAL','PORTAL_BALL','PORTAL_ORB','PORTAL_FX',
    'KLOGG_BALL','KLOGGBALL','KLOG_BALL','KLOGBALL','TREASURE_BALL','TREASUREBALL',
    'TELEPORT_FX','TELEPORTER_ORB','TELEPORTER_FX','EXIT','EXITORB','EXIT_ORB',
    'WARPBALL','WARP_BALL','WARP','WARPORB','WARP_ORB','PORTAL_PURPLE',
    'PURPLEPORTAL','PURPLE_PORTAL','PURPLE_ORB','PURPLEORB',
    # 1970 hamster
    '1970','HAMSTER','HAMSTER_ICON','HAMSTERICON','SHIELD','HAMSTER_SHIELD',
    'HAMSTERSHIELD','HAM','HAMS','SHRINE','SHIELD_ICON','SHIELDICON',
    # Hand (Phoenix hand projectile sprite?)
    'PHOENIX','PHOENIX_HAND','PHOENIXHAND','HAND','PHEONIX','PHEONIXHAND',
    'PHOENIX_PROJ','PHOENIXPROJ','PROJECTILE','PROJ','PROJ_PHOENIX',
    # Clay debris
    'CLAY','CLAYDEBRIS','CLAY_DEBRIS','DEBRIS','CHUNK','CLAY_CHUNK','RUBBLE',
    'GORE','GOO','SPLAT','PIECE','PARTICLE','CLAY_BLOB','CLAYBLOB','BLOB',
    'POOF','RED_BLOB','REDBLOB','RED_CLAY','REDCLAY',
]

print('Testing %d names against %d targets:' % (len(NAMES), len(TARGETS)))
for n in NAMES:
    h = calcHash(n)
    if h in TARGETS:
        print('  HIT: %s -> 0x%08x (%s)' % (n, h, TARGETS[h]))
print('done')
