"""Handler for asset type 101 - VRAM Slot Configuration.

VERIFIED 2026-01-20 via Ghidra (GetAsset101Entry @ 0x8007b3fc, InitVRAMSlotTable @ 0x80013b1c)

Asset 101 is always 12 bytes and controls texture page slot allocation.
Only present in ~14 levels that need custom VRAM layouts.

Structure:
  - bank_a_count (u32): Texture slots in upper VRAM (y=0xF0), values 1-4
  - bank_b_count (u32): Texture slots in lower VRAM (y=0x1F0), values 0-1  
  - reserved (u32): Always 0

InitVRAMSlotTable creates (bank_a_count + bank_b_count) * 3 texture page slots.
When Asset 101 is absent, the engine defaults to bank_a=0, bank_b=2.

Observed values:
  - bank_a=1, bank_b=1: HEAD only (Joe-Head-Joe boss level)
  - bank_a=2, bank_b=0: Most levels (38 occurrences)
  - bank_a=2, bank_b=1: FOOD only (special rendering)
  - bank_a=3, bank_b=0: EVIL stage1/secondary1 (more sprites)
  - bank_a=4, bank_b=0: BOIL, EGGS, MEGA (max sprite complexity)
"""
import json
import struct
from pathlib import Path
from . import register_handler


@register_handler(101)
def handle_vram_slot_config(data: bytes, asset_info, output_dir: Path, context: dict) -> list[Path]:
    """Extract asset 101 - VRAM slot configuration."""
    output_dir.mkdir(parents=True, exist_ok=True)
    
    if len(data) != 12:
        # Fall back to binary dump
        bin_path = output_dir / "101_vram_slot_config.bin"
        bin_path.write_bytes(data)
        return [bin_path]
    
    # Parse the 12-byte structure (verified via Ghidra)
    bank_a_count = struct.unpack_from('<I', data, 0)[0]
    bank_b_count = struct.unpack_from('<I', data, 4)[0]
    reserved = struct.unpack_from('<I', data, 8)[0]
    
    # Calculate derived values
    total_slots = (bank_a_count + bank_b_count) * 3
    
    result = {
        "asset_type": 101,
        "bank_a_count": bank_a_count,
        "bank_b_count": bank_b_count,
        "reserved": reserved,
        "total_texture_slots": total_slots,
        "raw_hex": data.hex(),
        "level": asset_info.level,
        "segment": asset_info.segment
    }
    
    # Save as JSON
    json_path = output_dir / "101_vram_slot_config.json"
    with open(json_path, 'w') as f:
        json.dump(result, f, indent=2)
    
    return [json_path]
