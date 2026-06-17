"""Test player-themed names against 0x21842018 (PLAYER BASE SPRITE)."""
import sys; sys.path.insert(0, 'tools/scripts')
from calchash import calcHash

def rotr(x, n): return ((x >> n) | (x << (32-n))) & 0xFFFFFFFF
SEED = 0x28C0E011

# Targets with their code-context
TARGETS = {
    0x21842018: 'PLAYER BASE (InitPlayerEntity)',
    0x2c182010: 'MEGA BOSS (SetEntitySpriteId)',
    0x68c01218: 'MENU sprite (InitEntitySprite at z=2000)',
    0x38a0c119: 'MENU sister at z=2000 (paired with 0x68c01218)',
    0x28c080df: 'DEMO global (z=30000 = top layer)',
    0x10882010: 'BonusClayball EVIL (InitBonusClayballEntity)',
    0x04084011: 'throw-animation sprite (in code comment)',
    0x29808408: 'global audio (22 levels)',
    0x1847c001: 'type700 unknown',
    0x28a0c119: 'duckling MENU fl=5',
    0x30a0c119: 'duckling 76-frame MENU fl=7',
    0x088c5011: 'FINN sprite fl=7',
    0x0c01c014: 'GLEN sprite fl=8',
}

# All candidate name patterns
candidates = []

# Player base candidates
candidates += ['KLAYMAN','KLAYMEN','KLAY','KLAY_BASE','KLAYBASE','PLAYER','PLAYER_BASE',
               'PLAYERBASE','PLAYR','PROTAG','PROTO','KLY','KMEN','KMAN',
               'KLAY_PLAYER','KLAY_HERO','HERO','BASE','PROT','MAIN','MAINMAN',
               'P1','P2','PLAYER1','PLAYER01','PLR','PLR1','BASE_KLAY','BASEKLAY',
               'KLAYIDLE','IDLE','STAND','STANDING','READY','BASE_IDLE','IDLEPOSE',
               'KLY_BASE','PLAYER_IDLE','KLAYMEN_BASE','KLAYMENBASE',
               'PLAYER_IDLE','HERO_IDLE','HEROIDLE','HEROBASE','PROTAGBASE']

# MEGA boss body candidates
candidates += ['MEGA','MEGA_BODY','MEGABODY','MEGA_BOSS','MEGABOSS','BOSS_MEGA',
               'BOSSMEGA','BIG','BIGBODY','BIGBOSS','MEGABDY','MEGA1','MEGA01',
               'MEGABOOM','MEGAGUY','MEGAMAN','MEGA_MAN','MEGAGOD','MEGAUGLY']

# MENU candidates
candidates += ['MENU','MENU_BG','MENUBG','MENU_BANNER','MENUBANNER','MENULOGO',
               'MENU_LOGO','MENUFRAME','MENU_FRAME','MENU_TITLE','MENUTITLE',
               'TITLE','TITLEBG','SPLASH','LOGO','BANNER','BACKDROP','PLATE',
               'FOREGROUND','BG','BGPIECE','MENU_PIECE','MENU_BAR','MENUBAR',
               'MENU_TAB','MENUTAB','MENU_BTN','MENUBUTTON','MENUBOX']

# DEMO/global candidates
candidates += ['DEMO','DEMO_TXT','DEMOTXT','DEMO_BANNER','DEMOBANNER','DEMOTAG',
               'DEMO_TAG','DEMO_LOGO','DEMOLOGO','DEMO_OVERLAY','DEMOOVERLAY',
               'DEMOMARK','DEMO_MARK','DEMOSTAMP','DEMO_STAMP','OVERLAY',
               'WATERMARK','WATER_MARK','STAMP','MARK','TAG','BANNER',
               'INTRO','OUTRO','PROMO','PROMOTAG']

# Bonus clayball candidates  
candidates += ['CLAYBALL','CLAY_BALL','CLAY','BALL','BONUSBALL','BONUS_BALL',
               'BONUS','BONUSCLAY','BIG_BALL','BIGBALL','POWERUP','POWER_UP',
               'PICKUP','PICK_UP','POWERBALL','POWER_BALL','MEGACLAY']

# Throw animation candidates
candidates += ['THROW','THROW_ANIM','THROWANIM','TOSS','HURL','HUCK','LOB',
               'BALL_THROW','THROW_BALL','THROWBALL','BALLTHROW','PITCH',
               'CHUCK','CAST','FLING','LAUNCH','SHOOT','SHOOTBALL']

# Duckling candidates (76-frame MENU, "Three crowned ducklings")
candidates += ['DUCK','DUCKS','DUCKLING','DUCKLINGS','BABY','BABIES','TRIPLE','TRIO',
               'CHICKS','CHICK','BIRDS','BIRD','THREE','3DUCKS','3KING','3KINGS',
               '3PRINCE','3PRINCES','PRIN','PRINCE','PRINCES','REGAL','ROYAL',
               'CROWN','CROWNS','CROWNED','TROOP','SQUAD','TROUPE','GANG','BAND',
               'PALS','BUDS','BROS','BUDDIES','BUDDY','GUYS','PRINCEX3','TRIPRINCE',
               'BIGTHREE','3GUYS','THREEGUYS','SUMMER','OF', 'JOEY','JOEYS','JOE_HEAD',
               'JOEHEAD','JOEHEADJOE']

# Audio candidates (global, 22 levels)
candidates += ['UI_BEEP','BEEP','BLIP','PING','CHIME','CLICK','TAP','TICK',
               'HUD_BEEP','PAUSE_SND','PAUSESND','MENUSND','MENU_SND',
               'POPUP','POP_UP','PROMPT','ALERT','NOTICE','BUTTON_PRESS',
               'BTNPRESS','UI_CONFIRM','CONFIRM','UI_PICK','PICK_SND',
               'NAVIGATE','NAV_SND','NAVSND','OPTION_SND','OPT_SND',
               'PAUSE_BEEP','PAUSEBEEP','RESUME','UNPAUSE',
               'GLOBAL_FX','GLOBAL_SFX','UI_SFX','MENU_FX','MENU_SFX',
               'EXPLODE','HURT','OUCH','HIT','CRACK','SMASH','BOOM','POW','SLAM']

# Add SFX_, FX_, S_, M_, MENU_ prefix variants
prefixes = ['', 'FX_', 'SFX_', 'S_', 'M_', 'MENU_', 'UI_', 'HUD_', 'GAME_', 'GLOBAL_']
augmented = set()
for c in candidates:
    cu = c.upper()
    for p in prefixes:
        augmented.add(p + cu)
        augmented.add(cu + '_FX')
        augmented.add(cu + '_SFX')

# Specific Klaymen/Skullmonkeys naming guesses
augmented.update([
    'KLAY_IDLE_BASE','KLAY_BASE_IDLE','KLAYMEN_IDLE','KLAYMEN_BASE','BASE_PLAYER',
    'PLAYER_BASE_IDLE','BASE_KLAY_IDLE','HERO_IDLE_BASE',
    'KLY_IDLE','KLY_BASE','KLY_BASE_IDLE','KLY_HERO',
    'KLAY_RUN_BASE','BASEKLY','HEROBASE_KLY',
    'MEGA_BODY_BASE','MEGABODYBASE','MEGABOSS_BODY','MEGAGUY_BODY',
    'MEGA_FRAME','MEGA_LIMB','MEGA_TORSO',
])

# Test
print(f'Testing {len(augmented)} candidates against {len(TARGETS)} targets...')
print()
hit_total = 0
for tid, label in TARGETS.items():
    # Both namespaces (we'll see which lights up)
    for ns_label, brute_h in [('wrap', rotr(tid ^ SEED, 27)), ('raw', tid)]:
        hits = [c for c in augmented if calcHash(c) == brute_h]
        if hits:
            for h in hits:
                print(f'  HIT 0x{tid:08x} ({ns_label}): {h!r:<25s} <- {label}')
                hit_total += 1
print()
print(f'Total: {hit_total} hits')
