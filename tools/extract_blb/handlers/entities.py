"""
Entity Handler - Asset Type 501 (0x1F5)

Extracts entity placement data (collectibles, enemies, triggers, etc.)
Each entity is a 24-byte structure containing position, type, and variant info.

Entity structure (verified via Ghidra):
  Offset  Size  Field
  0x00    u16   x1 (bounding box left)
  0x02    u16   y1 (bounding box top)
  0x04    u16   x2 (bounding box right)
  0x06    u16   y2 (bounding box bottom)
  0x08    u16   x_center
  0x0A    u16   y_center
  0x0C    u16   variant (animation frame/subtype selector)
  0x0E    u16   padding
  0x10    u16   padding
  0x12    u16   entity_type
  0x14    u16   layer (1, 2, or 3 typically)
  0x16    u16   padding

Known entity types:
  Type 2  = Clayballs (coins/collectibles)
  Type 3  = Ammo pickup
  Type 8  = Item pickup
  Type 24 = Special ammo pickup
  Type 25, 27 = Enemies
  Type 28, 48 = Moving platforms / directional objects
  Type 45 = Message box
"""

from pathlib import Path
import json
import struct
from . import register_handler

# Entity type name lookup
ENTITY_TYPE_NAMES = {
    2: "clayball",
    3: "ammo",
    8: "item",
    24: "special_ammo",
    25: "enemy_a",
    27: "enemy_b",
    28: "platform_a",
    45: "message",
    48: "platform_b",
}


def parse_entity(data: bytes, offset: int = 0) -> dict:
    """Parse a single 24-byte entity structure."""
    if len(data) < offset + 24:
        return None
    
    fields = struct.unpack_from('<HHHHHHHHHHHH', data, offset)
    x1, y1, x2, y2 = fields[0:4]
    x_center, y_center = fields[4:6]
    variant = fields[6]
    # fields[7], [8] are padding
    entity_type = fields[9]
    layer = fields[10]
    # fields[11] is padding
    
    return {
        "bbox": {"x1": x1, "y1": y1, "x2": x2, "y2": y2},
        "center": {"x": x_center, "y": y_center},
        "variant": variant,
        "entity_type": entity_type,
        "entity_name": ENTITY_TYPE_NAMES.get(entity_type, f"type_{entity_type}"),
        "layer": layer,
        # Derived values
        "tile_x": x_center // 16,
        "tile_y": y_center // 16,
        "width": x2 - x1 if x2 > x1 else 0,
        "height": y2 - y1 if y2 > y1 else 0,
    }


def parse_entities(data: bytes) -> list[dict]:
    """Parse all entities from asset 501 data."""
    entities = []
    entity_size = 24
    
    for offset in range(0, len(data) - entity_size + 1, entity_size):
        entity = parse_entity(data, offset)
        if entity:
            entity["index"] = len(entities)
            entities.append(entity)
    
    return entities


@register_handler(501)
def entity_handler(data: bytes, asset_info, output_dir: Path, context: dict) -> list[Path]:
    """
    Handler for entity data (asset type 501).
    
    Creates:
    - entities/entities.json: Full entity list with all fields
    - entities/by_type.json: Entities grouped by type
    - entities/summary.json: Statistics about entity types
    """
    from . import default_handler
    
    # Run default handler first (saves raw .bin)
    output_paths = default_handler(data, asset_info, output_dir, context)
    
    # Parse entities
    entities = parse_entities(data)
    
    if not entities:
        return output_paths
    
    # Create entities subdirectory
    entities_dir = output_dir / "entities"
    entities_dir.mkdir(parents=True, exist_ok=True)
    
    # Full entity list
    entities_path = entities_dir / "entities.json"
    entities_path.write_text(json.dumps(entities, indent=2))
    output_paths.append(entities_path)
    
    # Group by type
    by_type = {}
    for entity in entities:
        etype = entity["entity_type"]
        if etype not in by_type:
            by_type[etype] = {
                "type_id": etype,
                "type_name": entity["entity_name"],
                "count": 0,
                "entities": []
            }
        by_type[etype]["count"] += 1
        by_type[etype]["entities"].append({
            "index": entity["index"],
            "center": entity["center"],
            "tile": {"x": entity["tile_x"], "y": entity["tile_y"]},
            "variant": entity["variant"],
            "layer": entity["layer"],
        })
    
    by_type_path = entities_dir / "by_type.json"
    by_type_path.write_text(json.dumps(by_type, indent=2))
    output_paths.append(by_type_path)
    
    # Summary statistics
    type_counts = {}
    for entity in entities:
        etype = entity["entity_type"]
        name = entity["entity_name"]
        key = f"{etype}_{name}"
        type_counts[key] = type_counts.get(key, 0) + 1
    
    # Find bounding box of all entities
    if entities:
        min_x = min(e["center"]["x"] for e in entities)
        max_x = max(e["center"]["x"] for e in entities)
        min_y = min(e["center"]["y"] for e in entities)
        max_y = max(e["center"]["y"] for e in entities)
    else:
        min_x = max_x = min_y = max_y = 0
    
    summary = {
        "level": asset_info.level,
        "segment": asset_info.segment,
        "entity_count": len(entities),
        "type_counts": type_counts,
        "bounds": {
            "min_x": min_x, "max_x": max_x,
            "min_y": min_y, "max_y": max_y,
            "width_pixels": max_x - min_x,
            "height_pixels": max_y - min_y,
        },
        "layers_used": sorted(set(e["layer"] for e in entities)),
    }
    
    summary_path = entities_dir / "summary.json"
    summary_path.write_text(json.dumps(summary, indent=2))
    output_paths.append(summary_path)
    
    return output_paths
