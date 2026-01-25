#!/usr/bin/env python3
"""
Generate HTML report of all entity types with sprite previews.

Uses sprite_mapping.json to correlate entity sprite IDs with
extracted sprite files.

Data source: docs/reference/entity-types.md, entity-sprite-id-mapping.md

Usage:
    python tools/scripts/generate_entity_report.py --output /tmp/entity_report.html
    
    # With pre-built sprite mapping (faster):
    python tools/scripts/build_sprite_mapping.py /tmp/extracted_blb
    python tools/scripts/generate_entity_report.py --mapping /tmp/extracted_blb/sprite_mapping.json
"""

import json
import os
import re
import base64
from pathlib import Path
from collections import defaultdict

# Entity type callback table - 121 entries from g_EntityTypeCallbackTable @ 0x8009d5f8
# Source: docs/reference/entity-types.md
# Sprite IDs extracted from Ghidra decompilation of entity init callbacks
# Note: Some types use sprite tables (marked with "table" in notes), first entry shown
ENTITY_TYPES = {
    # --- Player Sprites (not entities, but useful reference) ---
    # Player main: 0x21842018, Jump: 0x092B8480, Fall: 0x0B2084D0
    # Player sprite table at 0x8009c174 (16 entries)
    
    # --- Type 0-12: Basic/Default Types ---
    # VERIFIED 2026-01-25 via Ghidra + visual inspection
    # SPRITE IDs verified by tracing InitEntitySprite calls in each init chain
    0:  {"name": "Default Handler", "callback": "0x8007efd0", "category": "Object"},
    1:  {"name": "Boss Entity Start", "callback": "0x8007f730", "sprite_id": 0x98f8221e, "category": "Boss"},
    2:  {"name": "Green Bullets", "callback": "0x80080328", "sprite_id": 0xe8628689, "category": "Collectible", "verified": True, "count": 5727, "notes": "InitGreenBulletsCollectible @ 0x8002d474"},
    3:  {"name": "Ammo (Default)", "callback": "0x8007efd0", "category": "Collectible", "count": 308},
    4:  {"name": "Default Handler", "callback": "0x8007efd0", "category": "Object"},
    5:  {"name": "Moving Platform A", "callback": "0x8007f7b0", "sprite_id": 0x88783718, "category": "Platform", "verified": True, "notes": "NOT enemy - hovering platform"},
    6:  {"name": "Moving Platform B", "callback": "0x8007f830", "sprite_id": 0x8818a018, "category": "Platform", "verified": True, "notes": "NOT enemy - different platform visual"},
    7:  {"name": "Clayball (Orange)", "callback": "0x80080408", "sprite_id": 0xb8700ca1, "category": "Collectible", "verified": True, "notes": "100 = 1 extra life. InitClayballWithRandomColor @ 0x8002dbdc"},
    8:  {"name": "Klogg Catchable Ball", "callback": "0x80081504", "sprite_id": 0x0c34aa22, "category": "Boss", "verified": True, "notes": "KLOG boss fight only - catch with pot"},
    9:  {"name": "Platform Decor", "callback": "0x800804e8", "sprite_id": 0x8818a018, "category": "Decor", "notes": "table @ 0x8009b214"},
    10: {"name": "Egg-Beater (Enemy)", "callback": "0x8007f244", "sprite_ids": [0x0428801e, 0x0408c01e], "category": "Enemy", "verified": True, "notes": "Weapon-only, table @ 0x8009da50"},
    11: {"name": "Extra Life (1-Up)", "callback": "0x80080478", "sprite_id": 0xa9240484, "category": "Collectible", "verified": True, "notes": "Klaymen's Head - AddPlayerLives(+1)"},
    12: {"name": "Moving Platform C", "callback": "0x8007f8b0", "sprite_id": 0x9299c307, "category": "Platform", "verified": True, "notes": "MOSS-style platform"},
    
    # --- Type 13-16: Unused ---
    13: {"name": "(Unused)", "callback": "0x00000000", "category": "Unused"},
    14: {"name": "(Unused)", "callback": "0x00000000", "category": "Unused"},
    15: {"name": "(Unused)", "callback": "0x00000000", "category": "Unused"},
    16: {"name": "(Unused)", "callback": "0x00000000", "category": "Unused"},
    
    # --- Type 17-27: Clock Platforms, Phoenix Hand, Enemies ---
    # VERIFIED 2026-01-25: Types 19-21 are Clock Platforms, Type 23 is Phoenix Hand
    # Note: Types 17, 18 use sprite IDs 0x93c9a20f and 0x9ab9a209 which don't exist in BLB
    17: {"name": "Enemy Cluster A", "callback": "0x8007f930", "sprite_id": 0x93c9a20f, "category": "Enemy", "notes": "sprite not in BLB"},
    18: {"name": "Enemy Cluster B", "callback": "0x8007f9b0", "sprite_id": 0x9ab9a209, "category": "Enemy", "notes": "sprite not in BLB"},
    19: {"name": "Clock Platform A", "callback": "0x8007fa30", "sprite_id": 0x93043811, "category": "Platform", "verified": True, "notes": "Timer countdown, disappears"},
    20: {"name": "Clock Platform B", "callback": "0x8007faac", "sprite_id": 0xd2801814, "category": "Platform", "verified": True, "notes": "Timer countdown, disappears"},
    21: {"name": "Clock Platform C", "callback": "0x8007fb28", "sprite_id": 0x12800031, "category": "Platform", "verified": True, "notes": "Timer countdown, disappears"},
    22: {"name": "Decor With HUD Icon", "callback": "0x80080398", "sprite_id": 0xb8700ca1, "category": "Decor"},
    23: {"name": "Phoenix Hand (The Bird)", "callback": "0x80080558", "sprite_id": 0x9158a0f6, "category": "Collectible", "verified": True, "notes": "L1 weapon - seeks enemies"},
    24: {"name": "Special Ammo", "callback": "0x8007f460", "sprite_id": 0x93c9a20f, "category": "Collectible", "count": 227, "notes": "sprite not in BLB"},
    25: {"name": "Phart Head", "callback": "0x800805c8", "sprite_id": 0x8c510186, "category": "Collectible", "notes": "L2 weapon - gas clone checkpoint", "verified": True, "count": 152},
    26: {"name": "Egg-Beater (Flying)", "callback": "0x8007f2cc", "sprite_ids": [0x004c9138, 0x40489938], "category": "Enemy", "verified": True, "notes": "table @ 0x8009da5c - weapon-only enemy"},
    27: {"name": "Flapper (Path Enemy)", "callback": "0x8007f354", "sprite_ids": [0x004a981c, 0x024e981c, 0x425a399c], "category": "Enemy", "verified": True, "notes": "table @ 0x8009da68", "count": 60},
    
    # --- Type 28-48: Platforms, Objects, Portals ---
    28: {"name": "Platform (Single Frame)", "callback": "0x80080638", "sprite_id": 0x08624580, "category": "Platform", "count": 99},
    29: {"name": "Scale Reset (Yellow Chevron)", "callback": "0x800806a8", "sprite_id": 0x8c30008c, "category": "Collectible", "verified": True, "notes": "InitScaleResetCollectible - sends 0x100d msg to reset player scale"},
    30: {"name": "Interactive Decor", "callback": "0x80080a98", "sprite_id": 0x88210498, "category": "Decor", "notes": "InitCollectibleEntity_Alt"},
    31: {"name": "Moving Platform (Scaled) A", "callback": "0x80080af8", "sprite_id": 0x84004480, "category": "Platform", "notes": "Uses g_SpriteList_Type063_MovingPlatform+1"},
    32: {"name": "Moving Platform (Scaled) B", "callback": "0x80080af8", "sprite_id": 0x84004480, "category": "Platform", "notes": "Uses g_SpriteList_Type063_MovingPlatform+1"},
    33: {"name": "Moving Platform (Scaled) C", "callback": "0x80080af8", "sprite_id": 0x84004480, "category": "Platform", "notes": "Uses g_SpriteList_Type063_MovingPlatform+1"},
    34: {"name": "Trigger Platform A", "callback": "0x80080b60", "sprite_id": 0x84004480, "category": "Platform", "notes": "Uses g_SpriteList_Type064_Trigger+1, scaled for types 0x22-0x24"},
    35: {"name": "Trigger Platform B", "callback": "0x80080b60", "sprite_id": 0x84004480, "category": "Platform", "notes": "Uses g_SpriteList_Type064_Trigger+1, scaled for types 0x22-0x24"},
    36: {"name": "Trigger Platform C", "callback": "0x80080b60", "sprite_id": 0x84004480, "category": "Platform", "notes": "Uses g_SpriteList_Type064_Trigger+1, scaled for types 0x22-0x24"},
    37: {"name": "Directional Scaled A", "callback": "0x80080bc8", "sprite_id": 0x93c9a20f, "category": "Object", "notes": "InitDirectionalScaledEntity, sprite not in BLB"},
    38: {"name": "Directional Scaled B", "callback": "0x80080bc8", "sprite_id": 0x93c9a20f, "category": "Object", "notes": "InitDirectionalScaledEntity, sprite not in BLB"},
    39: {"name": "Floating Platform A", "callback": "0x80080c8c", "sprite_id": 0xb4011101, "category": "Platform", "notes": "InitFloatingPlatformEntity"},
    40: {"name": "Path Following (Alt)", "callback": "0x80080cfc", "sprite_id": 0x8c30008c, "category": "Object", "notes": "InitPathFollowingEntity_Alt"},
    41: {"name": "Clay Keeper Enemy", "callback": "0x80080d6c", "sprite_id": 0x800B4AEC, "category": "Enemy", "notes": "Walking collectible enemy (drops clayballs). Uses InitWalkingCollectibleEnemy"},
    42: {"name": "Sno-Blo A", "callback": "0x80080ddc", "sprite_id": 0xa0299a4d, "category": "Enemy", "verified": True, "notes": "Snow enemy from SNO levels. Sprite table @ 0x8009b508"},
    43: {"name": "Sno-Blo B", "callback": "0x80080ddc", "sprite_id": 0xa0299a4d, "category": "Enemy", "verified": True, "notes": "Snow enemy from SNO levels. Sprite table @ 0x8009b508"},
    44: {"name": "Sno-Blo C", "callback": "0x80080ddc", "sprite_id": 0xa0299a4d, "category": "Enemy", "verified": True, "notes": "Snow enemy from SNO levels. Sprite table @ 0x8009b508"},
    45: {"name": "Bounceable Clay", "callback": "0x80080f1c", "sprite_ids": [0xf175320e, 0x657c322c], "category": "Platform", "verified": True, "notes": "Bounce to reach higher areas, destroyed on contact"},
    46: {"name": "Scaled Moving Object", "callback": "0x80080c2c", "sprite_id": 0x9b2204a0, "category": "Object", "notes": "InitScaledMovingEntity, z=900"},
    47: {"name": "Moving Platform A", "callback": "0x80080e4c", "sprite_id": 0x08624580, "category": "Platform"},
    48: {"name": "Moving Platform B", "callback": "0x80080e4c", "sprite_id": 0x08624580, "category": "Platform", "count": 297},
    
    # --- Type 49-61: Boss, Effects ---
    49: {"name": "Boss Spawner", "callback": "0x8007fba4", "sprite_id": 0x98f8221e, "category": "Boss"},
    50: {"name": "Boss Main", "callback": "0x8007fc20", "sprite_id": 0x181c3854, "category": "Boss", "verified": True},
    51: {"name": "Boss Part", "callback": "0x8007fc9c", "sprite_id": 0x8818a018, "category": "Boss", "verified": True},
    52: {"name": "Klaydog", "callback": "0x80080c8c", "sprite_id": 0xb4011101, "category": "Enemy", "verified": True, "notes": "Harmless 'enemy' - cannot hurt player. Uses InitFloatingPlatformEntity"},
    53: {"name": "Sno-Blo D", "callback": "0x80080ddc", "sprite_id": 0xa0299a4d, "category": "Enemy", "verified": True, "notes": "Snow enemy variant (sets +0x110 flag). Sprite table @ 0x8009b508"},
    54: {"name": "Sno-Blo E", "callback": "0x80080ddc", "sprite_id": 0xa0299a4d, "category": "Enemy", "verified": True, "notes": "Snow enemy variant (sets +0x110 flag). Sprite table @ 0x8009b508"},
    55: {"name": "Sno-Blo F", "callback": "0x80080ddc", "sprite_id": 0xa0299a4d, "category": "Enemy", "verified": True, "notes": "Snow enemy variant (sets +0x110 flag). Sprite table @ 0x8009b508"},
    56: {"name": "(Unused)", "callback": "0x00000000", "category": "Unused"},
    57: {"name": "Flying Enemy A", "callback": "0x8007fd18", "sprite_id": 0x88783718, "category": "Enemy"},
    58: {"name": "Flying Enemy B", "callback": "0x8007fd94", "sprite_id": 0x88783718, "category": "Enemy"},
    59: {"name": "Flying Enemy C", "callback": "0x8007fe10", "sprite_id": 0x88783718, "category": "Enemy"},
    60: {"name": "Sno-Blo G", "callback": "0x80080ddc", "sprite_id": 0xa0299a4d, "category": "Enemy", "verified": True, "notes": "Snow enemy - sets facing direction. Sprite table @ 0x8009b508"},
    61: {"name": "Universe Enema (Sparkle)", "callback": "0x80080718", "sprite_id": 0x6a351094, "category": "Collectible", "verified": True, "notes": "R1 weapon - kills all enemies on screen. Max 7. Storage: PlayerState+0x16. Uses InitSparkleEntity → CollectibleUniverseEnemaTickCallback"},
    
    # --- Type 62-76: Various ---
    62: {"name": "Ground Enemy A", "callback": "0x8007fe8c", "sprite_id": 0x8818a018, "category": "Enemy"},
    63: {"name": "Ground Enemy B", "callback": "0x8007fefc", "sprite_id": 0x8818a018, "category": "Enemy"},
    64: {"name": "Ground Enemy C", "callback": "0x8007ff6c", "sprite_id": 0x8818a018, "category": "Enemy"},
    65: {"name": "Password Entity", "callback": "0x80080f8c", "sprite_id": 0x28CE0610, "category": "UI", "notes": "Password entry screen. Creates 5 child symbol entities. Uses InitPasswordEntity"},
    66: {"name": "Projectile Emitter A", "callback": "0x8007ffdc", "sprite_id": 0x168254b5, "category": "Object"},
    67: {"name": "Projectile Emitter B", "callback": "0x80080050", "sprite_id": 0x168254b5, "category": "Object"},
    68: {"name": "Projectile Emitter C", "callback": "0x800800c4", "sprite_id": 0x168254b5, "category": "Object"},
    69: {"name": "1970 Icon (Bonus)", "callback": "0x80080788", "sprite_id": 0x88a28194, "category": "Collectible", "verified": True, "notes": "Only 3 in entire game. Collect all 3 to unlock portal to SEVN bonus level. Uses Collectible1970IconTickCallback"},
    70: {"name": "Hamster Shield", "callback": "0x800807f8", "sprite_id": 0x80e85ea0, "category": "Collectible", "verified": True, "notes": "Grants orbiting hamster shields (3 max) that kill enemies on contact. Storage: PlayerState+0x1A. Uses CollectibleHamsterShieldTickCallback → AdjustPlayerStats"},
    71: {"name": "Timer Trigger", "callback": "0x80080fec", "sprite_id": 0x8c30008c, "category": "Trigger"},
    72: {"name": "Super Willie (The Head)", "callback": "0x80080868", "sprite_id": 0x902c0002, "category": "Collectible", "verified": True, "notes": "R2 weapon - spins to auto-gather all visible power-ups. Max 7. Storage: PlayerState+0x1c"},
    73: {"name": "(Unused)", "callback": "0x00000000", "category": "Unused"},
    74: {"name": "(Unused)", "callback": "0x00000000", "category": "Unused"},
    75: {"name": "Yellow Bird (Glidey Bird)", "callback": "0x800808d8", "sprite_id": 0xc87ca082, "category": "Collectible", "verified": True, "notes": "Grants GLIDE ability (hold X while falling). Sets PlayerState+0x17 bit 0x02. NOT the shield halo - that's bit 0x01 from first orb. Uses InitYellowBirdCollectible → CollectibleYellowBirdTickCallback"},
    76: {"name": "Path Enemy", "callback": "0x8007f3dc", "sprite_id": 0x040a2110, "category": "Enemy", "notes": "table @ 0x8009da78"},
    
    # --- Type 77-99: Various ---
    77: {"name": "(Unused)", "callback": "0x00000000", "category": "Unused"},
    78: {"name": "(Unused)", "callback": "0x00000000", "category": "Unused"},
    79: {"name": "Entity Spawner", "callback": "0x8008121c", "sprite_id": 0x8c30008c, "category": "Spawner"},
    80: {"name": "Platform Trigger", "callback": "0x80080ebc", "sprite_id": 0x08624580, "category": "Platform"},
    81: {"name": "Animated Object A", "callback": "0x80080948", "sprite_id": 0x8c30008c, "category": "Object"},
    82: {"name": "Animated Object B (BOIL)", "callback": "0x8008127c", "sprite_id": 0x8c30008c, "category": "Object"},
    83: {"name": "Interactive Object", "callback": "0x800809b8", "sprite_id": 0x8c30008c, "category": "Interactive"},
    84: {"name": "Collectible Spawner", "callback": "0x8007f5b0", "sprite_id": 0xe8628689, "category": "Spawner"},
    85: {"name": "Collision Trigger A", "callback": "0x800812ec", "sprite_id": 0x8c30008c, "category": "Trigger"},
    86: {"name": "Invisible Hazard A", "callback": "0x8007f050", "category": "Hazard"},
    87: {"name": "Invisible Hazard B", "callback": "0x8007f050", "category": "Hazard"},
    88: {"name": "Invisible Hazard C", "callback": "0x8007f050", "category": "Hazard"},
    89: {"name": "Level Event A", "callback": "0x8008134c", "sprite_id": 0x8c30008c, "category": "Trigger"},
    90: {"name": "Homing Projectile", "callback": "0x80080138", "sprite_id": 0x168254b5, "category": "Object"},
    91: {"name": "Sine Projectile", "callback": "0x800801b4", "sprite_id": 0x168254b5, "category": "Object"},
    92: {"name": "Arc Projectile", "callback": "0x80080230", "sprite_id": 0x168254b5, "category": "Object"},
    93: {"name": "Straight Projectile", "callback": "0x800802ac", "sprite_id": 0x168254b5, "category": "Object"},
    94: {"name": "Indexed Sprite Entity", "callback": "0x80081428", "sprite_id": 0x8c30008c, "category": "Object", "notes": "Variable sprite from table @ 0x8009b430. Uses InitIndexedSpriteEntity"},
    95: {"name": "Sound Emitter (Panning)", "callback": "0x800814a4", "sprite_id": 0x8c30008c, "category": "Audio", "verified": True, "notes": "INVISIBLE - plays positional audio that pans with camera. Uses InitCameraTrackingEntity + SoundEmitterWithPanningTick"},
    96: {"name": "Special Collectible", "callback": "0x8007f638", "sprite_id": 0x9299c307, "category": "Collectible"},
    97: {"name": "Level Event B", "callback": "0x8008134c", "sprite_id": 0x8c30008c, "category": "Trigger"},
    98: {"name": "Level Event C", "callback": "0x8008134c", "sprite_id": 0x8c30008c, "category": "Trigger"},
    99: {"name": "Level State Collectible", "callback": "0x8007f4d0", "sprite_id": 0x6c289c1c, "category": "Collectible", "notes": "table @ 0x8009b53c"},
    
    # --- Type 100-120: Bosses, Triggers, Decor ---
    # Boss sprites from g_*BossSprites tables, NOT the HP bar HUD sprites
    100: {"name": "Glenn Yntis (Boss)", "callback": "0x8008105c", "sprite_id": 0x8068815C, "category": "Boss", "notes": "HP=5, sprites @ g_GlennYntisBossSprites[9]"},
    101: {"name": "Shriney Guard (Boss)", "callback": "0x800810cc", "sprite_id": 0x4C106054, "category": "Boss", "notes": "HP=3, sprites @ g_ShrineyGuardBossSprites[9]"},
    102: {"name": "Joe-Head-Joe (Boss)", "callback": "0x8008113c", "sprite_id": 0x0B290BA2, "category": "Boss", "notes": "HP=5, sprites @ g_JoeHeadJoeBossSprites[8]"},
    103: {"name": "Klogg (Boss)", "callback": "0x800811ac", "sprite_id": 0x193CA112, "category": "Boss", "notes": "HP=5, sprite @ 0x8009BAE4"},
    104: {"name": "Collision Trigger B", "callback": "0x800812ec", "sprite_id": 0x8c30008c, "category": "Trigger"},
    105: {"name": "Collision Trigger C", "callback": "0x800812ec", "sprite_id": 0x8c30008c, "category": "Trigger"},
    106: {"name": "Falling Platform A", "callback": "0x8007f0d0", "sprite_id": 0x08624580, "category": "Platform"},
    107: {"name": "Falling Platform B", "callback": "0x8007f0d0", "sprite_id": 0x08624580, "category": "Platform"},
    108: {"name": "Falling Platform C", "callback": "0x8007f0d0", "sprite_id": 0x08624580, "category": "Platform"},
    109: {"name": "Enemy Spawner", "callback": "0x8007f540", "sprite_id": 0x88783718, "category": "Spawner"},
    110: {"name": "Level Event D", "callback": "0x8008134c", "sprite_id": 0x8c30008c, "category": "Trigger"},
    111: {"name": "Level Event E", "callback": "0x8008134c", "sprite_id": 0x8c30008c, "category": "Trigger"},
    112: {"name": "Animated Decor D", "callback": "0x8007f140", "sprite_id": 0x8c30008c, "category": "Decor"},
    113: {"name": "Animated Decor E", "callback": "0x8007f140", "sprite_id": 0x8c30008c, "category": "Decor"},
    114: {"name": "Animated Decor F", "callback": "0x8007f140", "sprite_id": 0x8c30008c, "category": "Decor"},
    115: {"name": "Parallax Decor A", "callback": "0x8007f1c0", "sprite_id": 0x8c30008c, "category": "Decor"},
    116: {"name": "Parallax Decor B", "callback": "0x8007f1c0", "sprite_id": 0x8c30008c, "category": "Decor"},
    117: {"name": "Parallax Decor C", "callback": "0x8007f1c0", "sprite_id": 0x8c30008c, "category": "Decor"},
    118: {"name": "Special Pickup", "callback": "0x8007f460", "sprite_id": 0x60b8bc9c, "category": "Collectible", "notes": "table @ 0x8009b4dc"},
    119: {"name": "Checkpoint Entity", "callback": "0x80080a28", "sprite_id": 0x80729081, "category": "Interactive", "notes": "shadow: 0x88329285"},
    120: {"name": "Death Effect", "callback": "0x8007f6c0", "sprite_id": 0x1e28e0d4, "category": "Effect"},
}

# Player sprite table for reference (not entity types, but useful)
# From g_PlayerSpriteTable @ 0x8009c174
PLAYER_SPRITES = {
    0:  {"sprite_id": 0x08208902, "name": "Idle"},
    1:  {"sprite_id": 0x48204012, "name": "Walk"},
    2:  {"sprite_id": 0x8569a090, "name": "Jump"},
    3:  {"sprite_id": 0x0708a4a0, "name": "Fall"},
    4:  {"sprite_id": 0x052aa082, "name": "Unknown 4"},
    5:  {"sprite_id": 0x393c80c2, "name": "Unknown 5"},
    6:  {"sprite_id": 0x1cf99931, "name": "Unknown 6"},
    7:  {"sprite_id": 0x00388110, "name": "Damage"},
    8:  {"sprite_id": 0x1c3aa013, "name": "Pickup"},
    9:  {"sprite_id": 0x1c395196, "name": "Idle Variant 1"},
    10: {"sprite_id": 0x3838801a, "name": "Idle Variant 2"},
    11: {"sprite_id": 0x04084011, "name": "Unknown 11"},
    12: {"sprite_id": 0x092b8480, "name": "Jump Alt"},
    13: {"sprite_id": 0x0b2084d0, "name": "Fall Alt"},
    14: {"sprite_id": 0x292e8480, "name": "Walk Right"},
    15: {"sprite_id": 0x282b8491, "name": "Unknown 15"},
}

# Additional verified sprite IDs (debris, projectiles, effects)
EFFECT_SPRITES = {
    "Projectile":   0x168254b5,
    "Debris 1":     0xbe68d0c6,
    "Debris 2":     0xb868d0c6,
    "Debris 3":     0xb468d0c6,
    "Debris 4":     0x3d348056,
    "Death":        0x1e28e0d4,
}

# Category colors
CATEGORY_COLORS = {
    "Collectible": "#4CAF50",
    "Enemy": "#f44336",
    "Boss": "#9C27B0",
    "Platform": "#2196F3",
    "Interactive": "#FF9800",
    "Decor": "#795548",
    "Object": "#607D8B",
    "Mechanism": "#00BCD4",
    "Effect": "#E91E63",
    "UI": "#9E9E9E",
    "Spawner": "#FF5722",
    "Hazard": "#F44336",
    "Trigger": "#3F51B5",
    "Unused": "#333333",
}

def load_sprite_mapping(mapping_path: Path) -> dict:
    """Load sprite mapping file. Returns {hex_id: info, ...}"""
    if not mapping_path.exists():
        return {}
    
    data = json.loads(mapping_path.read_text())
    
    # Build hex->info mapping
    result = {}
    hex_to_dec = data.get("hex_to_decimal", {})
    sprites = data.get("sprites", {})
    
    for hex_id, dec_id in hex_to_dec.items():
        if str(dec_id) in sprites:
            result[hex_id] = sprites[str(dec_id)]
    
    return result


def get_image_base64(path: str, max_size: int = 64) -> str:
    """Get base64 encoded image for embedding in HTML."""
    try:
        with open(path, "rb") as f:
            data = f.read()
        return base64.b64encode(data).decode("utf-8")
    except Exception:
        return ""


def generate_html_report(extracted_dir: Path, output_path: Path, mapping_path: Path = None):
    """Generate the HTML report."""
    
    # Load sprite mapping if available
    if mapping_path and mapping_path.exists():
        print(f"Loading sprite mapping from {mapping_path}...")
        sprite_map = load_sprite_mapping(mapping_path)
    else:
        # Try default location
        default_mapping = extracted_dir / "sprite_mapping.json"
        if default_mapping.exists():
            print(f"Loading sprite mapping from {default_mapping}...")
            sprite_map = load_sprite_mapping(default_mapping)
        else:
            print("No sprite mapping found. Run build_sprite_mapping.py first.")
            sprite_map = {}
    
    print(f"Loaded {len(sprite_map)} sprite IDs from mapping")
    
    # Check for duplicate sprite IDs across entity types
    sprite_to_types = defaultdict(list)
    for type_id, info in ENTITY_TYPES.items():
        if "sprite_id" in info:
            sprite_to_types[info["sprite_id"]].append((type_id, info.get("name", "?")))
        elif "sprite_ids" in info:
            for sid in info["sprite_ids"]:
                sprite_to_types[sid].append((type_id, info.get("name", "?")))
    
    duplicates = {sid: types for sid, types in sprite_to_types.items() if len(types) > 1}
    if duplicates:
        print(f"\n⚠️  WARNING: {len(duplicates)} sprite IDs are used by multiple entity types:")
        for sid, types in sorted(duplicates.items()):
            types_str = ", ".join([f"Type {t[0]} ({t[1]})" for t in types])
            print(f"   0x{sid:08x}: {types_str}")
        print()
    
    # Build HTML
    html = """<!DOCTYPE html>
<html>
<head>
    <title>Skullmonkeys Entity Type Report</title>
    <style>
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: #1a1a2e;
            color: #eee;
            margin: 0;
            padding: 20px;
        }
        h1 {
            color: #fff;
            border-bottom: 2px solid #444;
            padding-bottom: 10px;
        }
        h2 {
            color: #aaa;
            margin-top: 40px;
        }
        .filters {
            background: #252542;
            padding: 15px;
            border-radius: 8px;
            margin-bottom: 20px;
            position: sticky;
            top: 0;
            z-index: 100;
        }
        .filters label {
            margin-right: 15px;
            cursor: pointer;
        }
        .filters input[type="checkbox"] {
            margin-right: 5px;
        }
        .search-box {
            padding: 8px 12px;
            border: 1px solid #444;
            border-radius: 4px;
            background: #1a1a2e;
            color: #fff;
            width: 300px;
            margin-right: 20px;
        }
        .entity-grid {
            display: grid;
            grid-template-columns: repeat(auto-fill, minmax(300px, 1fr));
            gap: 15px;
        }
        .entity-card {
            background: #252542;
            border-radius: 8px;
            padding: 15px;
            border: 1px solid #333;
        }
        .entity-card.verified {
            border-color: #4CAF50;
            box-shadow: 0 0 10px rgba(76, 175, 80, 0.3);
        }
        .entity-card:hover {
            border-color: #666;
        }
        .entity-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 10px;
        }
        .entity-type {
            font-size: 24px;
            font-weight: bold;
            color: #fff;
        }
        .entity-name {
            font-size: 14px;
            color: #aaa;
        }
        .category-badge {
            padding: 3px 8px;
            border-radius: 12px;
            font-size: 11px;
            font-weight: bold;
            text-transform: uppercase;
        }
        .verified-badge {
            background: #4CAF50;
            color: #fff;
            padding: 2px 6px;
            border-radius: 4px;
            font-size: 10px;
            margin-left: 8px;
        }
        .sprite-container {
            display: flex;
            flex-wrap: wrap;
            gap: 5px;
            margin: 10px 0;
            min-height: 48px;
            background: #1a1a2e;
            padding: 10px;
            border-radius: 4px;
        }
        .sprite-frame {
            background: #000;
            padding: 2px;
            border-radius: 2px;
            image-rendering: pixelated;
        }
        .sprite-frame img {
            display: block;
            image-rendering: pixelated;
            max-height: 48px;
        }
        .no-sprite {
            color: #666;
            font-style: italic;
        }
        .callback {
            font-family: monospace;
            font-size: 12px;
            color: #888;
        }
        .sprite-id {
            font-family: monospace;
            font-size: 11px;
            color: #666;
        }
        .levels {
            font-size: 11px;
            color: #888;
            margin-top: 5px;
        }
        .hidden {
            display: none !important;
        }
        .stats {
            background: #252542;
            padding: 15px;
            border-radius: 8px;
            margin-bottom: 20px;
            display: flex;
            gap: 30px;
        }
        .stat-item {
            text-align: center;
        }
        .stat-value {
            font-size: 28px;
            font-weight: bold;
            color: #4CAF50;
        }
        .stat-label {
            font-size: 12px;
            color: #888;
        }
    </style>
</head>
<body>
    <h1>🎮 Skullmonkeys Entity Type Report</h1>
    <p>Generated from Ghidra analysis and extracted sprite data. Click sprites to see full size.</p>
    
    <div class="stats">
        <div class="stat-item">
            <div class="stat-value" id="total-count">0</div>
            <div class="stat-label">Total Types</div>
        </div>
        <div class="stat-item">
            <div class="stat-value" id="verified-count">0</div>
            <div class="stat-label">Verified</div>
        </div>
        <div class="stat-item">
            <div class="stat-value" id="with-sprites-count">0</div>
            <div class="stat-label">With Sprites</div>
        </div>
        <div class="stat-item">
            <div class="stat-value" id="sprite-count">0</div>
            <div class="stat-label">Unique Sprites</div>
        </div>
    </div>
    
    <div class="filters">
        <input type="text" class="search-box" id="search" placeholder="Search by name, type, or sprite ID...">
        <label><input type="checkbox" class="cat-filter" value="Enemy" checked> Enemies</label>
        <label><input type="checkbox" class="cat-filter" value="Collectible" checked> Collectibles</label>
        <label><input type="checkbox" class="cat-filter" value="Boss" checked> Bosses</label>
        <label><input type="checkbox" class="cat-filter" value="Platform" checked> Platforms</label>
        <label><input type="checkbox" class="cat-filter" value="Decor" checked> Decor</label>
        <label><input type="checkbox" class="cat-filter" value="other" checked> Other</label>
        <label><input type="checkbox" id="verified-only"> Verified Only</label>
    </div>
    
    <div class="entity-grid" id="entity-grid">
"""
    
    # Sort entity types by number
    sorted_types = sorted(ENTITY_TYPES.items(), key=lambda x: x[0])
    
    total_count = 0
    verified_count = 0
    with_sprites_count = 0
    
    for type_id, info in sorted_types:
        total_count += 1
        
        name = info.get("name", f"Unknown Type {type_id}")
        callback = info.get("callback", "?")
        category = info.get("category", "Unknown")
        verified = info.get("verified", False)
        
        if verified:
            verified_count += 1
        
        # Get sprite IDs (convert to hex string for lookup)
        sprite_ids = []
        if "sprite_id" in info:
            sprite_ids = [info["sprite_id"]]
        elif "sprite_ids" in info:
            sprite_ids = info["sprite_ids"]
        
        # Get sprite images from mapping
        sprite_html = ""
        sprite_levels = set()
        
        for sid in sprite_ids:
            hex_id = f"0x{sid:08x}"
            if hex_id in sprite_map:
                with_sprites_count += 1
                sprite_info = sprite_map[hex_id]
                
                # Get sample file path
                sample_file = sprite_info.get("sample_file")
                if sample_file:
                    full_path = extracted_dir / sample_file
                    b64 = get_image_base64(str(full_path))
                    if b64:
                        levels_str = ", ".join(sprite_info.get("levels", [])[:3])
                        sprite_html += f'<div class="sprite-frame"><img src="data:image/png;base64,{b64}" title="Sprite {hex_id} ({sid}) - {levels_str}"></div>'
                
                # Add levels
                for lvl in sprite_info.get("levels", []):
                    sprite_levels.add(lvl)
        
        if not sprite_html:
            sprite_html = '<span class="no-sprite">No sprite extracted</span>'
        
        # Category color
        cat_color = CATEGORY_COLORS.get(category, "#666")
        
        # Verified badge
        verified_badge = '<span class="verified-badge">✓ VERIFIED</span>' if verified else ''
        
        # Sprite ID display
        sprite_id_html = ""
        if sprite_ids:
            sprite_id_html = "<br>".join([f'<span class="sprite-id">Sprite: 0x{sid:08x} ({sid})</span>' for sid in sprite_ids])
        
        # Levels display
        levels_html = f'<div class="levels">Levels: {", ".join(sorted(sprite_levels))}</div>' if sprite_levels else ''
        
        card_class = "entity-card verified" if verified else "entity-card"
        
        html += f'''
        <div class="{card_class}" data-category="{category}" data-type="{type_id}" data-name="{name}" data-verified="{str(verified).lower()}">
            <div class="entity-header">
                <div>
                    <span class="entity-type">Type {type_id}</span>
                    {verified_badge}
                </div>
                <span class="category-badge" style="background: {cat_color}">{category}</span>
            </div>
            <div class="entity-name">{name}</div>
            <div class="sprite-container">
                {sprite_html}
            </div>
            <div class="callback">Callback: {callback}</div>
            {sprite_id_html}
            {levels_html}
        </div>
'''
    
    html += f"""
    </div>
    
    <h2>🎮 Player Sprite Table</h2>
    <p>Player sprites from g_PlayerSpriteTable @ 0x8009c174 (16 entries)</p>
    <div class="entity-grid">
"""
    
    # Add player sprites
    for idx, player_info in PLAYER_SPRITES.items():
        sprite_id = player_info["sprite_id"]
        name = player_info["name"]
        hex_id = f"0x{sprite_id:08x}"
        
        sprite_html = ""
        if hex_id in sprite_map:
            sprite_info = sprite_map[hex_id]
            sample_file = sprite_info.get("sample_file")
            if sample_file:
                full_path = extracted_dir / sample_file
                b64 = get_image_base64(str(full_path))
                if b64:
                    sprite_html = f'<div class="sprite-frame"><img src="data:image/png;base64,{b64}" title="Player sprite {idx}: {name}"></div>'
        
        if not sprite_html:
            sprite_html = '<span class="no-sprite">Not extracted</span>'
        
        html += f'''
        <div class="entity-card verified" data-category="Player" data-type="P{idx}" data-name="{name}" data-verified="true">
            <div class="entity-header">
                <div>
                    <span class="entity-type">Index {idx}</span>
                    <span class="verified-badge">✓ VERIFIED</span>
                </div>
                <span class="category-badge" style="background: #E91E63">Player</span>
            </div>
            <div class="entity-name">{name}</div>
            <div class="sprite-container">
                {sprite_html}
            </div>
            <span class="sprite-id">Sprite: {hex_id} ({sprite_id})</span>
        </div>
'''
    
    html += """
    </div>
    
    <h2>💥 Effect & Debris Sprites</h2>
    <p>Known effect sprites from game code</p>
    <div class="entity-grid">
"""
    
    # Add effect sprites
    for name, sprite_id in EFFECT_SPRITES.items():
        hex_id = f"0x{sprite_id:08x}"
        
        sprite_html = ""
        if hex_id in sprite_map:
            sprite_info = sprite_map[hex_id]
            sample_file = sprite_info.get("sample_file")
            if sample_file:
                full_path = extracted_dir / sample_file
                b64 = get_image_base64(str(full_path))
                if b64:
                    sprite_html = f'<div class="sprite-frame"><img src="data:image/png;base64,{b64}" title="Effect: {name}"></div>'
        
        if not sprite_html:
            sprite_html = '<span class="no-sprite">Not extracted</span>'
        
        html += f'''
        <div class="entity-card" data-category="Effect" data-type="FX" data-name="{name}" data-verified="true">
            <div class="entity-header">
                <div>
                    <span class="entity-type">{name}</span>
                </div>
                <span class="category-badge" style="background: #E91E63">Effect</span>
            </div>
            <div class="sprite-container">
                {sprite_html}
            </div>
            <span class="sprite-id">Sprite: {hex_id} ({sprite_id})</span>
        </div>
'''
    
    html += f"""
    </div>
    
    <script>
        // Update stats
        document.getElementById('total-count').textContent = '{total_count}';
        document.getElementById('verified-count').textContent = '{verified_count}';
        document.getElementById('with-sprites-count').textContent = '{with_sprites_count}';
        document.getElementById('sprite-count').textContent = '{len(sprite_map)}';
        
        // Filter functionality
        const cards = document.querySelectorAll('.entity-card');
        const searchBox = document.getElementById('search');
        const catFilters = document.querySelectorAll('.cat-filter');
        const verifiedOnly = document.getElementById('verified-only');
        
        function applyFilters() {{
            const searchTerm = searchBox.value.toLowerCase();
            const enabledCats = new Set();
            catFilters.forEach(cb => {{
                if (cb.checked) enabledCats.add(cb.value);
            }});
            const showVerifiedOnly = verifiedOnly.checked;
            
            cards.forEach(card => {{
                const category = card.dataset.category;
                const name = card.dataset.name.toLowerCase();
                const type = card.dataset.type;
                const verified = card.dataset.verified === 'true';
                const text = card.textContent.toLowerCase();
                
                let catMatch = enabledCats.has(category) || 
                    (enabledCats.has('other') && !['Enemy', 'Collectible', 'Boss', 'Platform', 'Decor'].includes(category));
                let searchMatch = !searchTerm || text.includes(searchTerm) || type.includes(searchTerm);
                let verifiedMatch = !showVerifiedOnly || verified;
                
                card.classList.toggle('hidden', !(catMatch && searchMatch && verifiedMatch));
            }});
        }}
        
        searchBox.addEventListener('input', applyFilters);
        catFilters.forEach(cb => cb.addEventListener('change', applyFilters));
        verifiedOnly.addEventListener('change', applyFilters);
    </script>
</body>
</html>
"""
    
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(html)
    print(f"Report written to {output_path}")


def main():
    import argparse
    parser = argparse.ArgumentParser(description="Generate entity type HTML report")
    parser.add_argument("--extracted", "-e", default="/tmp/extracted_blb",
                        help="Path to extracted BLB directory")
    parser.add_argument("--mapping", "-m", default=None,
                        help="Path to sprite_mapping.json (default: <extracted>/sprite_mapping.json)")
    parser.add_argument("--output", "-o", default="/tmp/entity_report.html",
                        help="Output HTML file path")
    args = parser.parse_args()
    
    mapping_path = Path(args.mapping) if args.mapping else None
    generate_html_report(Path(args.extracted), Path(args.output), mapping_path)


if __name__ == "__main__":
    main()
