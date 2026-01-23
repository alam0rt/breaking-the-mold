#!/usr/bin/env python3
"""
Entity Type Analyzer - Aggregates entity type usage across all BLB levels.

Scans extracted entity data from extract_blb tool and produces:
1. Frequency table of all entity types
2. Level distribution for each type
3. Layer distribution analysis
4. Identification of unknown/unanalyzed types

Usage:
    python3 tools/scripts/analyze_entity_types.py [--blb-dir PATH] [--output PATH]

Requires extract_blb to have been run first:
    python3 -m tools.extract_blb disks/blb/GAME.BLB --output extracted/

Output:
    - entity_analysis.json: Full analysis data
    - entity_analysis.md: Human-readable report
"""

import argparse
import json
from pathlib import Path
from collections import defaultdict
import struct

# Entity type callback table (from docs/reference/entity-types.md)
ENTITY_CALLBACKS = {
    0: ("0x8007efd0", "Pickup/Collectible"),
    1: ("0x8007f730", "Unknown"),
    2: ("0x80080328", "Clayball"),
    3: ("0x8007efd0", "Pickup/Collectible"),
    4: ("0x8007efd0", "Pickup/Collectible"),
    5: ("0x8007f7b0", "Unknown (Flying?)"),
    6: ("0x8007f830", "Unknown (Flying?)"),
    7: ("0x80080408", "Unknown"),
    8: ("0x80081504", "Item"),
    9: ("0x800804e8", "Unknown"),
    10: ("0x8007f244", "Patrol Enemy?"),
    11: ("0x80080478", "Unknown"),
    12: ("0x8007f8b0", "Unknown"),
    # 13-16 unused
    17: ("0x8007f930", "Unknown"),
    18: ("0x8007f9b0", "Unknown"),
    19: ("0x8007fa30", "Unknown"),
    20: ("0x8007faac", "Unknown"),
    21: ("0x8007fb28", "Unknown"),
    22: ("0x80080398", "Unknown"),
    23: ("0x80080558", "Unknown"),
    24: ("0x8007f460", "SpecialAmmo"),
    25: ("0x800805c8", "EnemyA (Walker)"),
    26: ("0x8007f2cc", "Unknown (Enemy?)"),
    27: ("0x8007f354", "EnemyB (Fast)"),
    28: ("0x80080638", "PlatformA"),
    42: ("0x80080ddc", "Portal"),
    43: ("0x80080ddc", "Portal"),
    44: ("0x80080ddc", "Portal"),
    45: ("0x80080f1c", "Message Box"),
    48: ("0x80080e4c", "PlatformB"),
    50: ("0x8007fc20", "Boss"),
    51: ("0x8007fc9c", "Boss Part"),
    60: ("0x80080ddc", "Particle"),
    61: ("0x80080718", "Sparkle"),
}

# Known entity categories
KNOWN_CATEGORIES = {
    "Collectible": [0, 2, 3, 4, 8, 24],
    "Enemy": [25, 27],
    "Platform": [28, 48],
    "Interactive": [42, 43, 44, 45],
    "Boss": [50, 51],
    "Effect": [60, 61],
}


def parse_entity_bin(data: bytes) -> list[dict]:
    """Parse raw entity binary data (24 bytes per entity)."""
    entities = []
    for offset in range(0, len(data) - 23, 24):
        fields = struct.unpack_from('<HHHHHHHHHHHH', data, offset)
        entities.append({
            "x1": fields[0], "y1": fields[1],
            "x2": fields[2], "y2": fields[3],
            "x_center": fields[4], "y_center": fields[5],
            "variant": fields[6],
            "entity_type": fields[9],
            "layer": fields[10],
        })
    return entities


def scan_extracted_dir(extracted_dir: Path) -> dict:
    """Scan extracted directory for entity data."""
    all_entities = []
    levels_scanned = []
    
    for level_dir in sorted(extracted_dir.iterdir()):
        if not level_dir.is_dir():
            continue
        
        level_name = level_dir.name
        
        # Look for entity data in various locations
        entity_paths = list(level_dir.rglob("**/entities.json"))
        entity_bins = list(level_dir.rglob("**/asset_501*.bin"))
        
        for json_path in entity_paths:
            try:
                with open(json_path) as f:
                    entities = json.load(f)
                    for e in entities:
                        e["level"] = level_name
                        e["stage"] = json_path.parent.parent.name if "stage" in str(json_path) else "primary"
                    all_entities.extend(entities)
                    levels_scanned.append(f"{level_name}/{json_path.parent.parent.name}")
            except (json.JSONDecodeError, KeyError):
                pass
        
        # Also try raw bin files if no JSON found
        if not entity_paths:
            for bin_path in entity_bins:
                try:
                    data = bin_path.read_bytes()
                    entities = parse_entity_bin(data)
                    for e in entities:
                        e["level"] = level_name
                        e["stage"] = bin_path.parent.name
                    all_entities.extend(entities)
                    levels_scanned.append(f"{level_name}/{bin_path.parent.name}")
                except Exception:
                    pass
    
    return {
        "entities": all_entities,
        "levels_scanned": levels_scanned,
    }


def scan_blb_file(blb_path: Path) -> dict:
    """Directly scan BLB file for entity data (Asset 501)."""
    # This is a fallback if extracted data isn't available
    # Would need to implement BLB parsing
    return {"entities": [], "levels_scanned": []}


def analyze_entities(entities: list[dict]) -> dict:
    """Analyze entity type distribution."""
    
    # Type frequency
    type_counts = defaultdict(int)
    for e in entities:
        type_counts[e["entity_type"]] += 1
    
    # Level distribution per type
    type_levels = defaultdict(lambda: defaultdict(int))
    for e in entities:
        type_levels[e["entity_type"]][e.get("level", "unknown")] += 1
    
    # Layer distribution per type
    type_layers = defaultdict(lambda: defaultdict(int))
    for e in entities:
        type_layers[e["entity_type"]][e.get("layer", 0)] += 1
    
    # Variant analysis per type
    type_variants = defaultdict(set)
    for e in entities:
        type_variants[e["entity_type"]].add(e.get("variant", 0))
    
    # Categorize types
    type_info = {}
    for etype, count in sorted(type_counts.items()):
        callback_addr, desc = ENTITY_CALLBACKS.get(etype, ("unknown", "Unknown"))
        
        # Determine category
        category = "Unknown"
        for cat, types in KNOWN_CATEGORIES.items():
            if etype in types:
                category = cat
                break
        
        is_known = category != "Unknown" or "Unknown" not in desc
        
        type_info[etype] = {
            "type_id": etype,
            "count": count,
            "callback": callback_addr,
            "description": desc,
            "category": category,
            "is_known": is_known,
            "levels": dict(type_levels[etype]),
            "layers": dict(type_layers[etype]),
            "variants_used": sorted(type_variants[etype]),
            "variant_count": len(type_variants[etype]),
        }
    
    # Summary stats
    total_entities = len(entities)
    known_count = sum(1 for e in entities if type_info[e["entity_type"]]["is_known"])
    unknown_types = [t for t, info in type_info.items() if not info["is_known"]]
    
    return {
        "total_entities": total_entities,
        "total_types": len(type_counts),
        "known_entity_count": known_count,
        "unknown_entity_count": total_entities - known_count,
        "unknown_types": sorted(unknown_types),
        "unknown_type_count": len(unknown_types),
        "types": type_info,
    }


def generate_report(analysis: dict, levels_scanned: list[str]) -> str:
    """Generate human-readable markdown report."""
    lines = [
        "# Entity Type Analysis Report",
        "",
        f"**Generated**: {__import__('datetime').datetime.now().isoformat()}",
        f"**Levels Scanned**: {len(set(levels_scanned))}",
        "",
        "## Summary",
        "",
        f"| Metric | Value |",
        f"|--------|-------|",
        f"| Total Entities | {analysis['total_entities']:,} |",
        f"| Unique Types | {analysis['total_types']} |",
        f"| Known Types | {analysis['total_types'] - analysis['unknown_type_count']} |",
        f"| Unknown Types | {analysis['unknown_type_count']} |",
        f"| Known Entity % | {100 * analysis['known_entity_count'] / max(1, analysis['total_entities']):.1f}% |",
        "",
        "## Unknown Types (Priority for Analysis)",
        "",
        "| Type | Count | Callback | Levels | Layers |",
        "|------|-------|----------|--------|--------|",
    ]
    
    # Sort unknown types by frequency
    unknown_sorted = sorted(
        [(t, analysis['types'][t]) for t in analysis['unknown_types']],
        key=lambda x: -x[1]['count']
    )
    
    for etype, info in unknown_sorted:
        levels = ', '.join(sorted(info['levels'].keys())[:3])
        if len(info['levels']) > 3:
            levels += f" (+{len(info['levels'])-3})"
        layers = ', '.join(str(l) for l in sorted(info['layers'].keys()))
        lines.append(f"| {etype} | {info['count']:,} | {info['callback']} | {levels} | {layers} |")
    
    lines.extend([
        "",
        "## All Types by Frequency",
        "",
        "| Type | Count | Category | Description | Layers | Variants |",
        "|------|-------|----------|-------------|--------|----------|",
    ])
    
    # Sort all types by frequency
    all_sorted = sorted(
        analysis['types'].items(),
        key=lambda x: -x[1]['count']
    )
    
    for etype, info in all_sorted:
        layers = ', '.join(str(l) for l in sorted(info['layers'].keys()))
        var_str = f"{info['variant_count']}" if info['variant_count'] <= 5 else f"{info['variant_count']} variants"
        known_marker = "" if info['is_known'] else " ❓"
        lines.append(f"| {etype} | {info['count']:,} | {info['category']}{known_marker} | {info['description']} | {layers} | {var_str} |")
    
    lines.extend([
        "",
        "## Types by Category",
        "",
    ])
    
    # Group by category
    by_category = defaultdict(list)
    for etype, info in analysis['types'].items():
        by_category[info['category']].append((etype, info))
    
    for category in ["Collectible", "Enemy", "Platform", "Interactive", "Boss", "Effect", "Unknown"]:
        if category not in by_category:
            continue
        types_in_cat = by_category[category]
        total_in_cat = sum(info['count'] for _, info in types_in_cat)
        lines.append(f"### {category} ({len(types_in_cat)} types, {total_in_cat:,} entities)")
        lines.append("")
        for etype, info in sorted(types_in_cat, key=lambda x: -x[1]['count']):
            lines.append(f"- Type {etype}: {info['count']:,} × {info['description']}")
        lines.append("")
    
    lines.extend([
        "## Level Distribution",
        "",
        "Types per level (top 10 by type count):",
        "",
    ])
    
    # Level entity counts
    level_types = defaultdict(set)
    for etype, info in analysis['types'].items():
        for level in info['levels'].keys():
            level_types[level].add(etype)
    
    for level, types in sorted(level_types.items(), key=lambda x: -len(x[1]))[:10]:
        lines.append(f"- **{level}**: {len(types)} types")
    
    return '\n'.join(lines)


def main():
    parser = argparse.ArgumentParser(description="Analyze entity types across BLB levels")
    parser.add_argument("--extracted-dir", type=Path, 
                        default=Path("tools/extract_blb/extracted"),
                        help="Directory with extracted BLB data")
    parser.add_argument("--blb-dir", type=Path,
                        default=Path("disks/blb"),
                        help="Directory with BLB files (fallback)")
    parser.add_argument("--output", type=Path,
                        default=Path("docs/analysis"),
                        help="Output directory for analysis files")
    args = parser.parse_args()
    
    # Try extracted data first
    print(f"Scanning extracted data from {args.extracted_dir}...")
    result = scan_extracted_dir(args.extracted_dir)
    
    if not result["entities"]:
        # Try alternative paths
        alt_paths = [
            Path("extracted"),
            Path("../evil-engine/extracted"),
            Path("/home/sam/projects/evil-engine/extracted"),
        ]
        for alt in alt_paths:
            if alt.exists():
                print(f"Trying alternative path: {alt}")
                result = scan_extracted_dir(alt)
                if result["entities"]:
                    break
    
    if not result["entities"]:
        print("No entity data found. Run extract_blb first:")
        print("  python3 -m tools.extract_blb disks/blb/GAME.BLB --output extracted/")
        return 1
    
    print(f"Found {len(result['entities'])} entities from {len(result['levels_scanned'])} level/stages")
    
    # Analyze
    print("Analyzing entity types...")
    analysis = analyze_entities(result["entities"])
    
    print(f"Total types: {analysis['total_types']}")
    print(f"Unknown types: {analysis['unknown_type_count']}")
    
    # Output
    args.output.mkdir(parents=True, exist_ok=True)
    
    json_path = args.output / "entity_analysis.json"
    json_path.write_text(json.dumps(analysis, indent=2, default=list))
    print(f"Wrote {json_path}")
    
    report = generate_report(analysis, result["levels_scanned"])
    md_path = args.output / "entity_analysis.md"
    md_path.write_text(report)
    print(f"Wrote {md_path}")
    
    # Print summary
    print("\n--- Quick Summary ---")
    print(f"Top 10 unknown types by frequency:")
    unknown_sorted = sorted(
        [(t, analysis['types'][t]) for t in analysis['unknown_types']],
        key=lambda x: -x[1]['count']
    )[:10]
    for etype, info in unknown_sorted:
        print(f"  Type {etype:3d}: {info['count']:5,} entities ({info['callback']})")
    
    return 0


if __name__ == "__main__":
    exit(main())
