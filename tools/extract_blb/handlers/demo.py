"""
Demo / Attract-Mode Replay Handler (Asset Type 700)

Asset 700 is NOT audio. It was previously mis-handled as an SPU sample bank,
which decoded the RLE input stream as ADPCM and produced ~1KB of garbage WAV.
It was resolved on 2026-01-19 as demo/attract-mode controller-input replay data
(docs/blb/asset-types.md, docs/systems/demo-attract-mode.md). It appears in 9 of
26 levels (MENU, SCIE, TMPL, BOIL, FOOD, BRG1, GLID, CAVE, WEED) -- only those
can be played in attract mode.

Layout (verified):
    0x00  u32   entry_count   (always 1)
    0x04  u32   entry_id      (varies per level)
    0x08  u32   data_size     (bytes of replay stream)
    0x0C  u32   data_offset   (always 16)
    0x10+ 4*N   replay entries, run-length encoded:
              u16 buttons     PSX controller bitmask (held this span)
              u16 duration    number of consecutive frames to hold it

Consumed at runtime by UpdateInputState @0x800259d4: in playback mode it walks
the entries, holding `buttons` for `duration` frames each, until the stream ends
or a real button press interrupts the demo.

This handler decodes the stream to an inspectable JSON sidecar (raw button hex +
decoded names + duration) alongside the raw .bin. The "0x80/0xC0" bytes that look
SPU-like are just button bitmasks (Left=0x80, etc.), which is what fooled the old
audio handler.
"""

import json
import struct
from pathlib import Path

from . import register_handler, default_handler
from ..asset_names import known_stem


# PSX controller bitmask -> button name.
# Source: docs/systems/demo-attract-mode.md (authoritative for replay data; the
# standard PSX hardware layout, matched by that doc's worked example 0x0080=Left).
PSX_BUTTONS = [
    (0x0001, "Select"),
    (0x0002, "L3"),
    (0x0004, "R3"),
    (0x0008, "Start"),
    (0x0010, "Up"),
    (0x0020, "Right"),
    (0x0040, "Down"),
    (0x0080, "Left"),
    (0x0100, "L2"),
    (0x0200, "R2"),
    (0x0400, "L1"),
    (0x0800, "R1"),
    (0x1000, "Triangle"),
    (0x2000, "Circle"),
    (0x4000, "Cross"),
    (0x8000, "Square"),
]


def decode_buttons(mask: int) -> list[str]:
    """Decode a PSX controller bitmask into a list of pressed button names."""
    return [name for bit, name in PSX_BUTTONS if mask & bit]


@register_handler(700)
def demo_replay_handler(
    data: bytes,
    asset_info,
    output_dir: Path,
    context: dict,
) -> list[Path]:
    """
    Handler for asset type 700 (demo/attract-mode input replay).

    Writes the raw bytes (via default handler) plus a decoded JSON sidecar of
    the RLE controller-input stream. Returns the raw bytes only if the header
    doesn't look like a valid replay block, so a malformed asset never crashes
    the run.
    """
    output_files = default_handler(data, asset_info, output_dir, context)

    if len(data) < 16:
        return output_files

    entry_count, entry_id, data_size, data_offset = struct.unpack_from("<IIII", data, 0)

    # Validate the fixed-shape header before trusting it (count is always 1,
    # offset always 16; the stream must fit inside the asset).
    if entry_count != 1 or data_offset != 16:
        return output_files
    if data_offset + data_size > len(data) or data_size < 4:
        return output_files

    # Walk the RLE stream: 4 bytes per entry (u16 buttons, u16 duration).
    entries = []
    total_frames = 0
    pos = data_offset
    end = data_offset + data_size
    while pos + 4 <= end:
        buttons, duration = struct.unpack_from("<HH", data, pos)
        entries.append({
            "buttons": f"0x{buttons:04X}",
            "names": decode_buttons(buttons),
            "duration_frames": duration,
        })
        total_frames += duration
        pos += 4

    demo_dir = output_dir / "demo"
    demo_dir.mkdir(parents=True, exist_ok=True)

    # Name the sidecar after the entry's known name if catalogued, else its id.
    stem = known_stem(entry_id, "demo", f"demo_{entry_id:08x}")
    replay = {
        "asset_type": 700,
        "level": asset_info.level,
        "segment": asset_info.segment,
        "entry_id": f"0x{entry_id:08X}",
        "entry_count": entry_count,
        "data_size": data_size,
        "frame_count": len(entries),
        "total_duration_frames": total_frames,
        # PAL attract demos run at 50Hz; NTSC at 60Hz.
        "approx_seconds_pal": round(total_frames / 50.0, 2),
        "entries": entries,
    }
    out = demo_dir / f"{stem}.json"
    out.write_text(json.dumps(replay, indent=2))
    output_files.append(out)

    return output_files
