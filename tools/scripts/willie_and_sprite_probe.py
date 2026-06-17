"""Test Willie Trombone variants against 0x902c0002 sprite, plus related Neverhood characters."""
import sys, os
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

# Load all uncracked IDs
uncracked = set()
with open('docs/analysis/asset-identification/cracked_names.csv') as f:
    next(f)
    for line in f:
        parts = line.split(',')
        if len(parts) >= 6 and parts[5] == 'uncracked':
            try:
                uncracked.add(int(parts[0], 16))
            except ValueError:
                pass

# Willie Trombone variants (the Neverhood Klaymen-rescue character)
targets_by_id = {}
def test(name, hint=''):
    h = calcHash(name)
    targets_by_id.setdefault(h, []).append((name, hint))

# Focus targets
FOCUS = {
    0x902c0002: "Willie Trombone sprite",
    0xe48744c4: "FX_PICKUP_SUPER_WILLIE audio (verified)",
    0x6a351094: "UniverseEnema sprite",
    0x88a28194: "1970Icon sprite (Earthworm Jim hamster)",
    0x80e85ea0: "HamsterShield / Swirly-Q sprite",
    0x8c30008c: "Scale Reset / Grow sprite",
    0x08624580: "SingleFrameDecor / 100C sprite",
    0xbb781481: "wake decor sprite",
    0xb01c25f0: "death-grow sprite",
    0x0e3889be: "JoeHeadJoe landed/charging sprite",
    0x069580a0: "JoeHeadJoe ball splat sprite",
    0xbe68d0c6: "death debris large",
    0xb868d0c6: "death debris small",
    0x09406d8a: "Ma-Bird checkpoint sprite",
    0xa9240484: "1-up sprite",
    0xb8700ca1: "Green-bullets sprite",
    0x9158a0f6: "Phoenix-Hand sprite",
    0x8c510186: "Phart-Head sprite",
    0xe8628689: "green-bullet variant",
    0x21842018: "Klaymen player sprite (FINN)",
    0x168254b5: "FINN projectile sprite",
}

# === Willie Trombone variants ===
WILLIE = [
    "WILLIE", "WILLIETROMBONE", "WILLIE_TROMBONE", "TROMBONE", "WILLY", "WILLYTROMBONE",
    "WILLY_TROMBONE", "WILT", "WILLI", "WIL_TROM", "TROMBONE_WILLIE", "TROMBONE_WIL",
    "WILLIE_T", "W_TROMBONE", "WT", "WLT", "WILLY_T",
    "ASWILLIE", "ASWILLIETROMBONE", "SSWILLIE", "SS_WILLIE",
    "WILLIE_HEAD", "WILLIE_BODY", "WILLIE_ICON", "WILLIE_SPR", "WILLIE_ANIM",
    "TROMBONE_PWR", "WILLIE_PWR", "WILLIE_PICKUP", "TROMBONE_PICKUP",
    "TROMB", "TROMP", "TROM", "TROMBO", "TROMBN",
    "JINGLE_WILLIE", "WILLIE_GO", "WILLIE_NEW", "WILLIE_OK", "WILLIE_PLR",
    # Super Willie / power form
    "SUPERWILLIE", "SUPER_WILLIE", "SUPERWILL", "SUPER_WILL", "SUPERW",
    "SUPER_WILLY", "SUPERWILLY", "SUPER_TROMBONE", "SUPERTROMBONE",
    "WILLIE_SUPER", "WILLIE_POWER", "POWER_WILLIE",
    # From Neverhood lore
    "WILLIE_NEV", "NEV_WILLIE", "TROMBONIST", "PIPER", "FLOATY_WILLIE",
]
for n in WILLIE: test(n, "Willie Trombone")

# === UniverseEnema sprite variants ===
ENEMA = [
    "UNIVERSEENEMA","UNIVERSE_ENEMA","ENEMA","UNIVERSE","UE","UNV","UNIV","COSMIC",
    "COSMIC_ENEMA","COSMICENEMA","SCREEN_CLEAR","WIPEOUT","KILLALL","KILL_ALL",
    "UE_ICON","UE_PWR","UE_SPR","UE_PICKUP","UE_HEAD","UE_BODY","UE_BAG","UE_NOZZLE",
    "BAG","BAG_ICON","NOZZLE","TIP","ENEMABAG","ENEMA_BAG","CANISTER","UE_BLB",
    "BLAST","UNI_BLAST","UNIBLAST","SUPERBLAST","SUPER_BLAST","NUKE","MEGABOMB",
    "WORLD_END","ARMAGEDDON","APOCALYPSE","REND","RAPTURE",
]
for n in ENEMA: test(n, "UniverseEnema")

# === 1970 / Hamster icon (Earthworm Jim shout-out: "1970"=year of birth?) ===
ONE_NINE = [
    "1970","NINETEEN70","NINETEEN_70","NINETEENSEVENTY","1970_ICON","ICON_1970",
    "HAMSTER","HAMS","HAM","HAMI","HAMSTR","HAMSTERICON",
    "JIM","WORM","WORMJIM","EARTHWORM","EARTHWORMJIM","EARTHWORM_JIM",
    "SECRET","BONUS","BONUS_PICKUP","BONUS_ICON","BONUSICON","BONUSWARP",
    "GATEKEEPER","GATE","GATEWAY","WARP","WARPGATE","WARP_GATE",
    # Neverhood "Nineteen Seventy" reference?
    "PSYGNOSIS","RETRO","CLASSIC","OLD","OLDIE","SEVENTIES","70S",
]
for n in ONE_NINE: test(n, "1970")

# === HamsterShield / SwirlyQ (the orbiting hamsters that block damage) ===
SHIELD = [
    "SWIRLYQ","SWIRLY_Q","SWIRLY","Q","SQ","Q_SWIRL","Q_SPIN",
    "HAMSTERSHIELD","HAMSTER_SHIELD","HAM_SHIELD","HAMSHIELD","HAMSTERSHEILD",
    "SHIELD","SHIELDHAM","SHIELDHAMSTER","ORBITHAM","ORBIT_HAM","ORBIT","SPINHAM",
    "TRIPLE","TRIPLET","TRIPLE_HAM","TRIPLE_HAMSTER","THREE_HAM","THREEHAM",
    "RING","RING_HAM","RINGHAM","CIRCLEHAM","CIRCLE","CIRCLE_HAM",
    "ROUND","ROUNDHAM","SHIELD_ANIM","SHIELD_ROT","SHIELD_ICON","SHIELD_FX",
    "WARDOFFRING","WARDS","WARD","PROTECT","PROTECTOR","ARMOR","GUARDIAN",
    # Less obvious - cute variants
    "FURBAB","FURBABY","SOFTBAB","ROUND1","ROUNDONE","R1",
]
for n in SHIELD: test(n, "SwirlyQ/HamsterShield")

# === Scale Reset / Grow sprite ===
GROW = [
    "GROW","BIG","RESCALE","SCALE","SCALERESET","SCALE_RESET","RESET","UNSHRINK",
    "GROWPICKUP","GROW_PICKUP","BIG_PICKUP","BIGPICKUP","GROW_BACK","REGROW",
    "GROW_ICON","BIG_ICON","SCALE_ICON","RESETICON","RESET_ICON","UNSHRINK_ICON",
    "BIG_AGAIN","BIGAGAIN","FULL","FULLSIZE","FULL_SIZE","FULL_SCALE","GROWFX",
    "FATTEN","PLUMP","PLUMPER","FATTER","ENLARGE","ENBIGGEN","BUFF","BUFFED",
    # From Neverhood Klaymen — the spinning "GROW" pickup may have a specific name
    "GROW_PWR","GROWPWR","KLAY_GROW","GROW_KLAY","REGEN",
]
for n in GROW: test(n, "Grow")

# === 100C / SingleFrameDecor sprite (Phart Head common visual) ===
HUNDRED = [
    "100C","100_C","HUNDRED","HUNDREDC","HUNDRED_C","C_100","ONE_HUNDRED","ONEHUNDRED",
    "C","CENT","CENTURY","100COIN","100_COIN","100POINT","100_POINT","SCORE_100",
    "PHART","PHARTHEAD","PHART_HEAD","PHARTHEADICON","PHART_ICON","PHIcON",
    "PH","PHEAD","P_HEAD","PHARTSPR","PHART_SPR","PHARTPWR","PHART_PWR",
    "FART","FARTHEAD","FART_HEAD","FARTICON","FARTSPR","FARTPWR","FART_PWR",
    "BUTT","BUTTHEAD","BUTT_HEAD","BUTTICON","HEAD_FART","HEAD_PHART","HEADFART",
    "POOP","POOPHEAD","POO","POOICON","CRAP","CRAPHEAD","CRAP_HEAD",
    "GAS","GAS_HEAD","GASHEAD","TOOTHEAD","TOOT","TOOTICON","TOOT_ICON",
    "FLOATY","FLOATYC","FLOATY_C","FLOATYCOIN","FLOATY_COIN","FLOATCOIN",
    "NEON","NEONC","NEON_C","NEONNUMBER","NEON_NUMBER","NUMBERICON",
    # Phart-head extras counter
    "EXTRAS","EXTRA","EXTRAS_ICON","EXTRA_ICON","EXTRAS_PWR","EXTRA_PWR",
]
for n in HUNDRED: test(n, "100C/PhartHead")

# === Common / standard atoms (cross-test all targets) ===
EXTRA = [
    "BLB","BLOB","ITEM","BONUS","PICKUP","COLLECT","REWARD","GIFT","DROP","LOOT",
    "TRIGGER","FLAG","TOKEN","BUTTON","SWITCH","KEY","DOOR","GATE",
    "DEBRIS","CHUNK","BIT","FRAG","GIB","PIECE","SHARD","SLIVER","SPLINTER",
    "DEATH","DIE","DIED","DYING","DEATHFX","DEATH_FX","DEATH_ANIM","DEATHANIM",
    "EXPLODE","EXPLOSION","BURST","BLAST","BOOM","BLOWUP","BLOW_UP",
    "BLOOD","BLOODY","SPLAT","SPLATTER","SMASH","SQUISH","SQUASH","CRUSH",
    "SPAWN","SUMMON","CREATE","BIRTH","BORN","RISE","APPEAR","MATERIALIZE",
    "WAKE","WAKING","WOKEN","STIR","STARTLE","ALERT","ALERTED","NOTICE",
    "ENRAGE","ENRAGED","RAGE","ANGRY","MAD","MAD_ON","FURIOUS","HOSTILE",
]
for n in EXTRA: test(n, "")

# === Print only hits on focus targets ===
print("=== Hits on focus targets ===")
hits_found = False
for tid, label in FOCUS.items():
    if tid in targets_by_id:
        names = targets_by_id[tid]
        print(f"\n0x{tid:08x} ({label})")
        for n, hint in names:
            print(f"  {n!r:30s} hint={hint}")
        hits_found = True

# === Print any other uncracked hits ===
print("\n\n=== Other uncracked-ID hits ===")
for hid, names in sorted(targets_by_id.items()):
    if hid in FOCUS:
        continue
    if hid in uncracked:
        print(f"0x{hid:08x}")
        for n, hint in names:
            print(f"  {n!r:30s} hint={hint}")

if not hits_found:
    print("\nNo focus hits.")

# === Show counts ===
total_names = sum(len(v) for v in targets_by_id.values())
print(f"\n\nTotal candidates tested: {total_names}, unique hashes: {len(targets_by_id)}")
print(f"Focus targets defined: {len(FOCUS)}")
