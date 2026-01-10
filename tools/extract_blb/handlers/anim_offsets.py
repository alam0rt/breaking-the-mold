"""Handler for asset type 503 - Animation offset table (ToolX sequence data).

Asset 503 contains animation frame offset data organized as a TOC followed
by variable-length data sections for each animation.

Format:
  - count (u32): number of animation entries
  - TOC entries (12 bytes each):
    - index (u32): animation index (sequential 0..n-1)
    - data_size (u32): size of animation data (excludes 2-byte terminator)
    - data_offset (u32): byte offset from file start to animation data
  - Data sections: each contains frame offset values as u16 pairs (x, y or timing)
"""
import json
import struct
from pathlib import Path
from . import register_handler


@register_handler(503)
def handle_anim_offsets(data: bytes, asset_info, output_dir: Path, context: dict) -> list[Path]:
    """Extract asset 503 - animation offset table."""
    output_dir.mkdir(parents=True, exist_ok=True)
    
    if len(data) < 4:
        bin_path = output_dir / "503_anim_offsets.bin"
        bin_path.write_bytes(data)
        return [bin_path]
    
    count = struct.unpack_from('<I', data, 0)[0]
    
    # Validate TOC fits
    toc_size = 4 + count * 12
    if len(data) < toc_size:
        bin_path = output_dir / "503_anim_offsets.bin"
        bin_path.write_bytes(data)
        return [bin_path]
    
    animations = []
    
    for i in range(count):
        toc_offset = 4 + i * 12
        idx, data_size, data_offset = struct.unpack_from('<III', data, toc_offset)
        
        # Calculate actual data section boundaries
        if i + 1 < count:
            next_offset = struct.unpack_from('<I', data, 4 + (i + 1) * 12 + 8)[0]
            section_size = next_offset - data_offset
        else:
            section_size = len(data) - data_offset
        
        # Parse frame data as pairs of s16 (signed - negative values are offsets)
        frames = []
        if data_offset < len(data):
            for j in range(0, min(section_size, data_size), 4):
                if data_offset + j + 4 <= len(data):
                    x, y = struct.unpack_from('<hh', data, data_offset + j)  # signed!
                    frames.append({"x": x, "y": y})
        
        anim = {
            "index": idx,
            "data_size": data_size,
            "data_offset": data_offset,
            "section_size": section_size,
            "frame_count": len(frames),
            "frames": frames
        }
        animations.append(anim)
    
    result = {
        "asset_type": 503,
        "count": count,
        "toc_end_offset": toc_size,
        "level": asset_info.level,
        "segment": asset_info.segment,
        "animations": animations
    }
    
    # Save as JSON
    json_path = output_dir / "503_anim_offsets.json"
    with open(json_path, 'w') as f:
        json.dump(result, f, indent=2)
    
    return [json_path]
