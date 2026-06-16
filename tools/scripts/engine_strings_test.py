#!/usr/bin/env python3
"""Test Neverhood ENGINE-LEVEL strings against Skullmonkeys IDs.

Neverhood and Skullmonkeys both use ToolX. The engine itself uses these strings:
- Font metadata: asRecFont, meNumRows, meFirstChar, meCharWidth, meCharHeight, meTracking
- Default sequence: sqDefault
- Class prefixes: as, ss, sc, mo, km

These are ENGINE-LEVEL keys passed to calcHash() directly, not asset-name level.
They might appear in Skullmonkeys' engine code as well.
"""

def calc_hash(s):
    h = 0; sh = 0
    for c in s.upper():
        if 'A' <= c <= 'Z':
            sh = (sh + ord(c) - 64) & 31
            h ^= 1 << sh
        elif '0' <= c <= '9':
            sh = (sh + ord(c) + 22 - 64) & 31
            h ^= 1 << sh
    return h

def rotl(x, n):
    return ((x << n) | (x >> (32 - n))) & 0xFFFFFFFF

SEED = 0x28c0e011
def asset_id(s):
    return SEED ^ rotl(calc_hash(s), 27)

with open('/tmp/all_ids.txt') as f:
    ALL_IDS = set(int(line.strip(), 16) for line in f if line.strip())

# Engine-level strings observed in Neverhood ScummVM source
ENGINE_STRINGS = [
    "asRecFont", "sqDefault",
    "meNumRows", "meFirstChar", "meCharWidth", "meCharHeight", "meTracking",
    "meBaseLine", "meKerning", "meLeading",
    # Possible variants
    "asDefault", "asMain", "asIdle", "asAnim",
    "ssDefault", "ssMain",
    "sqMain", "sqIdle", "sqAnim", "sqIntro", "sqOutro",
    "scDefault", "scMain", "scIntro",
    "moDefault", "moMain", "moIntro",
    "kmDefault", "kmMain", "kmIdle",
    # Font-related
    "asFont", "asSmallFont", "asBigFont", "asScoreFont", "asHudFont",
    "asMenuFont", "asTitleFont", "asNumberFont", "asNumbers", "asDigits",
    # Common assets
    "asCursor", "asMenuCursor", "asArrow",
    "asLogo", "asTitle", "asMainMenu",
    "asDoor", "asMonkey", "asBoss", "asEnemy",
    # Skullmonkeys-specific guesses
    "asMega", "asWizz", "asKlogg", "asEvil",
    "asMenu", "asMain",
    # Try hash-table keys
    "calcHash", "ToolX",
    # Misc engine
    "init", "main", "default", "loop", "anim",
    "primary", "secondary", "common", "shared",
    "level0", "level1", "stage", "scene",
    # Color/palette
    "palette", "clut", "tex", "tile",
    # Sound
    "music", "sfx", "snd", "sound",
    # Other typical Neverhood
    "klaymen", "willie", "klogger",
]

# Test each string and variants
hits = {}
for base in ENGINE_STRINGS:
    cand_set = {base, base.lower(), base.upper(),
                base + "0", base + "1", base + "2", base + "3",
                base + "Idle", base + "Main", base + "Loop",
                base + "Active", base + "Anim",
                "as" + base[0].upper() + base[1:] if len(base) > 1 else "",
                }
    for c in cand_set:
        if not c: continue
        aid = asset_id(c)
        if aid in ALL_IDS:
            hits.setdefault(aid, []).append(c)

print(f"\n=== Hits ===")
for aid in sorted(hits):
    print(f"\n0x{aid:08x}:")
    for n in hits[aid]:
        print(f"  {n!r}")

# Also: Test combinations of LEVELS as numeric scenes (Neverhood uses Scene1101, Scene2208, etc.)
print("\n\n=== Scene/Module numeric patterns ===")
scene_hits = {}
for base in range(1000, 4000, 100):
    for prefix in ["Scene", "Module", "scene", "module", "asScene", "ssScene"]:
        cand = f"{prefix}{base}"
        aid = asset_id(cand)
        if aid in ALL_IDS:
            scene_hits.setdefault(aid, []).append(cand)
        # also +1 to base
        cand = f"{prefix}{base+1}"
        aid = asset_id(cand)
        if aid in ALL_IDS:
            scene_hits.setdefault(aid, []).append(cand)

for aid in sorted(scene_hits):
    print(f"0x{aid:08x}: {scene_hits[aid]}")
print(f"Scene hits: {sum(len(v) for v in scene_hits.values())}")
