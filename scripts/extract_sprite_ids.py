#!/usr/bin/env python3
"""
extract_sprite_ids.py - Extract sprite IDs from PAL and JP game binaries

Searches for calls to the sprite setup function and extracts the sprite ID 
parameter from the lui/ori instruction pattern.

Supports both:
  - PAL (SLES-01090): FUN_8001d080
  - JP (SLPS-01501): FUN_8001d484

Usage:
    python3 scripts/extract_sprite_ids.py [--pal PATH] [--jp PATH] [--output PATH]
    
Defaults:
    --pal: disks/pal/SLES_010.90
    --jp: disks/slps_015.01.bin (if exists)
    --output: tools/blb_viewer/config.js
"""

import struct
import json
import sys
import argparse
from pathlib import Path

# Region-specific addresses based on our analysis
# See docs/pal-jp-comparison.md for details
REGION_CONFIG = {
    'PAL': {
        'name': 'PAL (SLES-01090)',
        'sprite_setup_func': 0x8001d080,  # FUN_8001d080
        'init_entity_sprite': 0x8001c720,
        'game_state': 0x800ae3e8,
    },
    'JP': {
        'name': 'JP (SLPS-01501)', 
        'sprite_setup_func': 0x8001d484,  # FUN_8001d484 (+0x404 from PAL)
        'init_entity_sprite': 0x8001cb24,
        'game_state': 0x800af34c,
    }
}

# RAM/File offset conversion
RAM_BASE = 0x80010000
FILE_HEADER = 0x800


def find_jal_targets(data: bytes, target_addr: int) -> list[int]:
    """Find all JAL instructions that call target_addr."""
    target_encoded = (target_addr >> 2) & 0x03FFFFFF
    jal_instr = 0x0C000000 | target_encoded
    jal_bytes = struct.pack('<I', jal_instr)
    
    offsets = []
    pos = 0
    while True:
        pos = data.find(jal_bytes, pos)
        if pos == -1:
            break
        offsets.append(pos)
        pos += 4
    
    return offsets


def extract_sprite_id_from_callsite(data: bytes, call_offset: int, 
                                     register: int = 5) -> int | None:
    """
    Extract sprite ID passed to a function call.
    
    Looks backwards from call site for lui/ori pattern loading a 32-bit
    immediate into the specified register ($a1 = 5 by default).
    """
    lui_prefix = 0x3C00 | register
    ori_prefix = 0x3400 | (register << 5) | register
    
    found_lui = None
    found_ori = None
    
    # Look back up to 20 instructions (sometimes the load is further away)
    for i in range(1, 21):
        instr_offset = call_offset - (i * 4)
        if instr_offset < 0:
            break
        
        instr = struct.unpack('<I', data[instr_offset:instr_offset + 4])[0]
        instr_hi = instr >> 16
        
        if instr_hi == lui_prefix:
            found_lui = (instr & 0xFFFF) << 16
        elif instr_hi == ori_prefix:
            found_ori = instr & 0xFFFF
    
    if found_lui is not None and found_ori is not None:
        return found_lui | found_ori
    
    return None


def detect_region(data: bytes) -> str | None:
    """Auto-detect region by checking for known function addresses."""
    for region, config in REGION_CONFIG.items():
        call_offsets = find_jal_targets(data, config['sprite_setup_func'])
        if len(call_offsets) > 50:  # Should find many calls
            return region
    return None


def extract_sprite_ids_from_binary(binary_path: Path, region: str = None) -> dict:
    """
    Extract all sprite IDs from a binary.
    
    Args:
        binary_path: Path to the binary file
        region: 'PAL' or 'JP', or None to auto-detect
    
    Returns:
        Dict with sprite_ids and metadata
    """
    with open(binary_path, 'rb') as f:
        data = f.read()
    
    # Auto-detect region if not specified
    if region is None:
        region = detect_region(data)
        if region is None:
            print(f"Warning: Could not auto-detect region for {binary_path}")
            region = 'PAL'  # Default
    
    config = REGION_CONFIG[region]
    target_addr = config['sprite_setup_func']
    
    # Find all calls to the sprite setup function
    call_offsets = find_jal_targets(data, target_addr)
    
    # Extract sprite IDs from each call site
    sprite_ids = {}
    
    for file_offset in call_offsets:
        sprite_id = extract_sprite_id_from_callsite(data, file_offset)
        if sprite_id is not None:
            ram_addr = RAM_BASE + file_offset - FILE_HEADER
            hex_id = f"0x{sprite_id:08x}"
            
            if hex_id not in sprite_ids:
                sprite_ids[hex_id] = {
                    'id': sprite_id,
                    'hex': hex_id,
                    'call_sites': [],
                    'regions': set()
                }
            sprite_ids[hex_id]['call_sites'].append(f"0x{ram_addr:08x}")
            sprite_ids[hex_id]['regions'].add(region)
    
    return {
        'region': region,
        'config': config,
        'sprite_ids': sprite_ids,
        'call_count': len(call_offsets)
    }


def merge_sprite_ids(results: list[dict]) -> dict:
    """Merge sprite IDs from multiple regions."""
    merged = {}
    
    for result in results:
        region = result['region']
        for hex_id, data in result['sprite_ids'].items():
            if hex_id not in merged:
                merged[hex_id] = {
                    'id': data['id'],
                    'hex': hex_id,
                    'call_sites': {},
                    'regions': set()
                }
            merged[hex_id]['call_sites'][region] = data['call_sites']
            merged[hex_id]['regions'].update(data['regions'])
    
    # Convert sets to lists for JSON serialization
    for entry in merged.values():
        entry['regions'] = sorted(entry['regions'])
    
    return merged


def generate_config_js(sprite_ids: dict, output_path: Path, 
                       regions_found: list[str]):
    """Generate config.js with sprite ID mappings."""
    
    # Sort by sprite ID value
    sorted_ids = sorted(sprite_ids.values(), key=lambda x: x['id'])
    
    # Determine which regions have data
    region_str = ' and '.join(regions_found)
    
    lines = [
        "/**",
        " * config.js - Sprite ID mappings extracted from game binaries",
        " * ",
        " * Generated by scripts/extract_sprite_ids.py",
        f" * Sources: {region_str}",
        " * ",
        " * These sprite IDs are hardcoded in the game code and passed to",
        " * the sprite setup function. Each entity type has a specific sprite ID",
        " * that maps to sprites in the BLB file.",
        " * ",
        " * Sprite IDs are IDENTICAL between PAL and JP versions - only the",
        " * code addresses differ (see docs/pal-jp-comparison.md).",
        " */",
        "",
        "// Region-specific addresses for sprite setup function",
        "const REGION_ADDRESSES = {",
        "    PAL: {",
        "        spriteSetupFunc: 0x8001d080,  // FUN_8001d080",
        "        initEntitySprite: 0x8001c720,",
        "        gameState: 0x800ae3e8,",
        "    },",
        "    JP: {",
        "        spriteSetupFunc: 0x8001d484,  // FUN_8001d484 (+0x404)",
        "        initEntitySprite: 0x8001cb24,",
        "        gameState: 0x800af34c,",
        "    }",
        "};",
        "",
        f"// All sprite IDs found in binaries ({len(sorted_ids)} unique IDs)",
        "const SPRITE_IDS = {",
    ]
    
    for entry in sorted_ids:
        regions = entry['regions']
        # Get first call site from any region
        first_sites = []
        for region in regions:
            if region in entry['call_sites']:
                first_sites.extend(entry['call_sites'][region][:2])
        sites_str = ', '.join(first_sites[:3])
        if len(first_sites) > 3:
            sites_str += ', ...'
        
        region_marker = 'BOTH' if len(regions) > 1 else regions[0]
        lines.append(f"    {entry['hex']}: {{ id: {entry['id']}, regions: '{region_marker}' }},")
    
    lines.append("};")
    lines.append("")
    
    # Add known entity type mappings
    lines.extend([
        "// Entity type number → sprite ID mapping (used by engine.js for rendering)",
        "// These are the numeric entity types from Asset 501 entity data",
        "// See docs/entity-system.md for full entity documentation",
        "const ENTITY_SPRITE_MAP = {",
        "    // Type 2: Clayball collectible",
        "    2: 0x09406d8a,",
        "    ",
        "    // Type 8: Item/collectible (flying object)",
        "    8: 0x0c34aa22,",
        "    ",
        "    // Type 25: Enemy type 1",
        "    25: 0x1e1000b3,",
        "    ",
        "    // Type 27: Enemy type 2",
        "    27: 0x182d840c,",
        "    ",
        "    // Type 42: Portal/warp point",  
        "    42: 0xb01c25f0,",
        "    ",
        "    // Type 45: Message/save box",
        "    45: 0xa89d0ad0,",
        "    ",
        "    // Type 50: Boss main entity",
        "    50: 0x181c3854,",
        "    ",
        "    // Type 51: Boss sub-entity/part",
        "    51: 0x8818a018,",
        "    ",
        "    // Type 60: Particle effect",
        "    60: 0x168254b5,",
        "    ",
        "    // Type 61: Sparkle effect",
        "    61: 0x6a351094,",
        "};",
        "",
        "// Named entity lookup (for UI/debugging)",
        "const KNOWN_ENTITIES = {",
        "    // Player",
        "    'player': { spriteId: 0x21842018, name: 'Klaymen (Player)' },",
        "    ",
        "    // Boss entities",
        "    'boss_main': { spriteId: 0x181c3854, name: 'Boss Main Body' },",
        "    'boss_part': { spriteId: 0x8818a018, name: 'Boss Sub-Part' },",
        "    'boss_detail': { spriteId: 0x0244655d, name: 'Boss Detail' },",
        "    ",
        "    // Menu/UI",
        "    'menu_frame': { spriteId: 0xb8700ca1, name: 'Menu Frame' },",
        "    'menu_text': { spriteId: 0x00e2f188, name: 'Menu Text' },",
        "    'button': { spriteId: 0xa9240484, name: 'Button' },",
        "    'icon': { spriteId: 0x88a28194, name: 'Icon' },",
        "    ",
        "    // Collectibles",
        "    'clayball': { spriteId: 0x09406d8a, name: 'Clay Ball' },",
        "};",
        "",
        "// Sprite ID to friendly name lookup",
        "const SPRITE_NAMES = {};",
        "for (const [key, val] of Object.entries(KNOWN_ENTITIES)) {",
        "    const hexId = '0x' + val.spriteId.toString(16).padStart(8, '0');",
        "    SPRITE_NAMES[hexId] = val.name;",
        "}",
        "",
        "// Export for use in other modules",
        "if (typeof window !== 'undefined') {",
        "    window.SpriteConfig = { ",
        "        REGION_ADDRESSES, ",
        "        SPRITE_IDS, ",
        "        ENTITY_SPRITE_MAP,",
        "        KNOWN_ENTITIES, ",
        "        SPRITE_NAMES ",
        "    };",
        "}",
        "",
        "// For Node.js/CommonJS",
        "if (typeof module !== 'undefined') {",
        "    module.exports = { REGION_ADDRESSES, SPRITE_IDS, ENTITY_SPRITE_MAP, KNOWN_ENTITIES, SPRITE_NAMES };",
        "}",
        "",
    ])
    
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with open(output_path, 'w') as f:
        f.write('\n'.join(lines))
    
    print(f"Generated {output_path}")
    print(f"  - {len(sprite_ids)} unique sprite IDs")
    print(f"  - Regions: {region_str}")


def main():
    parser = argparse.ArgumentParser(
        description='Extract sprite IDs from PAL and/or JP game binaries'
    )
    parser.add_argument('--pal', type=Path, 
                        default=Path('disks/pal/SLES_010.90'),
                        help='Path to PAL binary')
    parser.add_argument('--jp', type=Path,
                        default=Path('disks/slps_015.01.bin'),
                        help='Path to JP binary')
    parser.add_argument('--output', type=Path,
                        default=Path('tools/blb_viewer/config.js'),
                        help='Output config.js path')
    args = parser.parse_args()
    
    results = []
    regions_found = []
    
    # Process PAL binary
    if args.pal.exists():
        print(f"Processing PAL: {args.pal}")
        result = extract_sprite_ids_from_binary(args.pal, 'PAL')
        print(f"  Found {len(result['sprite_ids'])} sprite IDs from {result['call_count']} call sites")
        results.append(result)
        regions_found.append('PAL')
    else:
        print(f"PAL binary not found: {args.pal}")
    
    # Process JP binary
    if args.jp.exists():
        print(f"Processing JP: {args.jp}")
        result = extract_sprite_ids_from_binary(args.jp, 'JP')
        print(f"  Found {len(result['sprite_ids'])} sprite IDs from {result['call_count']} call sites")
        results.append(result)
        regions_found.append('JP')
    else:
        print(f"JP binary not found: {args.jp} (skipping)")
    
    if not results:
        print("Error: No binaries found to process")
        sys.exit(1)
    
    # Merge results from all regions
    merged = merge_sprite_ids(results)
    
    print(f"\nMerged: {len(merged)} unique sprite IDs total")
    
    # Count IDs found in both vs one region only
    both = sum(1 for v in merged.values() if len(v['regions']) > 1)
    pal_only = sum(1 for v in merged.values() if v['regions'] == ['PAL'])
    jp_only = sum(1 for v in merged.values() if v['regions'] == ['JP'])
    print(f"  - In both regions: {both}")
    if pal_only: print(f"  - PAL only: {pal_only}")
    if jp_only: print(f"  - JP only: {jp_only}")
    
    # Generate output files
    json_path = args.output.with_suffix('.json')
    with open(json_path, 'w') as f:
        # Prepare JSON-serializable data
        json_data = {}
        for hex_id, entry in merged.items():
            json_data[hex_id] = {
                'id': entry['id'],
                'regions': entry['regions'],
                'call_sites': entry['call_sites']
            }
        json.dump({
            'sprite_ids': json_data,
            'count': len(merged),
            'regions': regions_found
        }, f, indent=2)
    print(f"Generated {json_path}")
    
    generate_config_js(merged, args.output, regions_found)


if __name__ == '__main__':
    main()
