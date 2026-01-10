"""Handler for asset type 101 - Unknown level variant flags.

Asset 101 is always 12 bytes. The first u32 appears to be a variant/mode
indicator (values 1-4 observed), with the remaining 8 bytes as zeros.

This asset only appears in 8 levels, suggesting it marks levels with
special behavior variations.
"""
import json
import struct
from pathlib import Path
from . import register_handler


@register_handler(101)
def handle_unknown_101(data: bytes, asset_info, output_dir: Path, context: dict) -> list[Path]:
    """Extract asset 101 - level variant flags."""
    output_dir.mkdir(parents=True, exist_ok=True)
    
    if len(data) != 12:
        # Fall back to binary dump
        bin_path = output_dir / "101_unknown_101.bin"
        bin_path.write_bytes(data)
        return [bin_path]
    
    # Parse the 12-byte structure
    variant = struct.unpack_from('<I', data, 0)[0]
    reserved1 = struct.unpack_from('<I', data, 4)[0]
    reserved2 = struct.unpack_from('<I', data, 8)[0]
    
    result = {
        "asset_type": 101,
        "variant": variant,
        "reserved1": reserved1,
        "reserved2": reserved2,
        "raw_hex": data.hex(),
        "level": asset_info.level,
        "segment": asset_info.segment
    }
    
    # Save as JSON
    json_path = output_dir / "101_unknown_101.json"
    with open(json_path, 'w') as f:
        json.dump(result, f, indent=2)
    
    return [json_path]
