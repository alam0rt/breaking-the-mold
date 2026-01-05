#!/usr/bin/env python3
"""
Sprite Viewer for Skullmonkeys BLB files.

Interactive tool to browse sprites and examine field values for pattern analysis.

Usage:
    python3 scripts/sprite_viewer.py [BLB_PATH]
    
Controls:
    - Arrow keys or number input to navigate
    - 'q' to quit, 'b' to go back
"""

import sys
import struct
from pathlib import Path
from typing import Optional
from dataclasses import dataclass

# Add scripts directory to path
sys.path.insert(0, str(Path(__file__).parent))
from blb import BLBFile, parse_container_toc


@dataclass
class SpriteHeader:
    """24-byte sprite header."""
    magic: int
    header_size: int
    frame_meta_size: int
    padding: int
    pixel_size: int
    sprite_id: int
    frame_count: int
    reserved_12: int
    animation_mode: int
    clut_value: int
    
    @classmethod
    def from_bytes(cls, data: bytes) -> "SpriteHeader":
        return cls(
            magic=struct.unpack('<H', data[0x00:0x02])[0],
            header_size=struct.unpack('<H', data[0x02:0x04])[0],
            frame_meta_size=struct.unpack('<H', data[0x04:0x06])[0],
            padding=struct.unpack('<H', data[0x06:0x08])[0],
            pixel_size=struct.unpack('<I', data[0x08:0x0C])[0],
            sprite_id=struct.unpack('<I', data[0x0C:0x10])[0],
            frame_count=struct.unpack('<H', data[0x10:0x12])[0],
            reserved_12=struct.unpack('<H', data[0x12:0x14])[0],
            animation_mode=struct.unpack('<H', data[0x14:0x16])[0],
            clut_value=struct.unpack('<H', data[0x16:0x18])[0],
        )
    
    @property
    def clut_x(self) -> int:
        return (self.clut_value & 0x3F) << 4
    
    @property
    def clut_y(self) -> int:
        return self.clut_value >> 6
    
    def is_valid(self) -> bool:
        return self.magic == 1 and self.header_size == 24


@dataclass
class FrameMetadata:
    """36-byte frame metadata."""
    runtime_ptr: int
    flags: int
    render_x: int
    render_y: int
    render_width: int
    render_height: int
    offset_adjust_x: int
    offset_adjust_y: int
    anchor_x: int
    anchor_y: int
    clip_width: int
    clip_height: int
    reserved_1a: int
    collision_flag: int
    reserved_1e: int
    rle_offset: int
    
    @classmethod
    def from_bytes(cls, data: bytes) -> "FrameMetadata":
        return cls(
            runtime_ptr=struct.unpack('<I', data[0x00:0x04])[0],
            flags=struct.unpack('<H', data[0x04:0x06])[0],
            render_x=struct.unpack('<h', data[0x06:0x08])[0],
            render_y=struct.unpack('<h', data[0x08:0x0A])[0],
            render_width=struct.unpack('<H', data[0x0A:0x0C])[0],
            render_height=struct.unpack('<H', data[0x0C:0x0E])[0],
            offset_adjust_x=struct.unpack('<h', data[0x0E:0x10])[0],
            offset_adjust_y=struct.unpack('<h', data[0x10:0x12])[0],
            anchor_x=struct.unpack('<h', data[0x12:0x14])[0],
            anchor_y=struct.unpack('<h', data[0x14:0x16])[0],
            clip_width=struct.unpack('<H', data[0x16:0x18])[0],
            clip_height=struct.unpack('<H', data[0x18:0x1A])[0],
            reserved_1a=struct.unpack('<H', data[0x1A:0x1C])[0],
            collision_flag=struct.unpack('<H', data[0x1C:0x1E])[0],
            reserved_1e=struct.unpack('<H', data[0x1E:0x20])[0],
            rle_offset=struct.unpack('<I', data[0x20:0x24])[0],
        )


class SpriteViewer:
    def __init__(self, blb_path: str):
        self.blb_path = blb_path
        self.blb = BLBFile(blb_path)
        self.levels = list(self.blb.header.iter_levels())
        
    def clear_screen(self):
        print("\033[2J\033[H", end="")
    
    def print_header(self, title: str):
        print("=" * 70)
        print(f"  {title}")
        print("=" * 70)
    
    def get_level_sprites(self, level) -> list:
        """Get all valid sprites from a level's tertiary data."""
        tert_offset = level.tertiary_byte_offset
        tert_size = level.tertiary_byte_size
        
        if tert_offset == 0 or tert_size == 0:
            return []
        
        with open(self.blb_path, 'rb') as f:
            f.seek(tert_offset)
            tert = f.read(tert_size)
        
        # Find Asset 600
        toc = parse_container_toc(tert)
        asset_600 = None
        for entry in toc:
            if entry.asset_id == 600:
                asset_600 = tert[entry.offset:entry.offset + entry.size]
                break
        
        if not asset_600:
            return []
        
        sprites = []
        sprite_count = struct.unpack('<I', asset_600[0:4])[0]
        
        for i in range(sprite_count):
            entry_off = 4 + i * 8
            toc_id, sprite_off = struct.unpack('<II', asset_600[entry_off:entry_off+8])
            
            if sprite_off == 0 or sprite_off >= len(asset_600) - 24:
                continue
            
            hdr = SpriteHeader.from_bytes(asset_600[sprite_off:sprite_off+24])
            if not hdr.is_valid():
                continue
            
            # Parse frames
            frames = []
            frame_base = sprite_off + 24
            for f in range(hdr.frame_count):
                frame_off = frame_base + f * 36
                if frame_off + 36 > len(asset_600):
                    break
                frames.append(FrameMetadata.from_bytes(asset_600[frame_off:frame_off+36]))
            
            sprites.append({
                'index': i,
                'toc_id': toc_id,
                'offset': sprite_off,
                'header': hdr,
                'frames': frames,
            })
        
        return sprites
    
    def show_level_list(self):
        """Show list of all levels."""
        self.clear_screen()
        self.print_header("SPRITE VIEWER - Level Selection")
        print()
        
        # Group by first letter for easier navigation
        for i, level in enumerate(self.levels):
            # Check if level has sprites
            marker = "  "
            sprites = self.get_level_sprites(level)
            if sprites:
                marker = f"({len(sprites):2d})"
            else:
                marker = " -- "
            
            print(f"  {i:2d}. {level.level_id:6s} {marker}  {level.name}")
        
        print()
        print("Enter level number (or 'q' to quit): ", end="", flush=True)
    
    def show_sprite_list(self, level, sprites: list):
        """Show list of sprites in a level."""
        self.clear_screen()
        self.print_header(f"SPRITES IN {level.level_id} - {level.name}")
        print()
        
        if not sprites:
            print("  No sprites found in this level's Asset 600")
            print()
            print("Press Enter to go back: ", end="", flush=True)
            return
        
        print(f"  {'#':>3}  {'TOC_ID':>6}  {'Frames':>6}  {'Mode':>4}  {'CLUT':>6}  {'PixelSz':>8}")
        print("  " + "-" * 50)
        
        for sp in sprites:
            hdr = sp['header']
            print(f"  {sp['index']:3d}  {sp['toc_id']:6d}  {hdr.frame_count:6d}  {hdr.animation_mode:4d}  "
                  f"0x{hdr.clut_value:04X}  {hdr.pixel_size:8,}")
        
        print()
        print("Enter sprite # to view details (or 'b' to go back): ", end="", flush=True)
    
    def show_sprite_detail(self, level, sprite: dict):
        """Show detailed sprite info including all frames."""
        self.clear_screen()
        hdr = sprite['header']
        
        self.print_header(f"{level.level_id} - Sprite #{sprite['index']} (TOC ID: {sprite['toc_id']})")
        print()
        
        # Header info
        print("SPRITE HEADER (24 bytes)")
        print("-" * 40)
        print(f"  magic:           {hdr.magic}")
        print(f"  header_size:     {hdr.header_size}")
        print(f"  frame_meta_size: {hdr.frame_meta_size}")
        print(f"  padding:         {hdr.padding}")
        print(f"  pixel_size:      {hdr.pixel_size:,}")
        print(f"  sprite_id:       {hdr.sprite_id}")
        print(f"  frame_count:     {hdr.frame_count}")
        print(f"  reserved_12:     0x{hdr.reserved_12:04X}  {'⚠️ NON-ZERO!' if hdr.reserved_12 else '✓ zero'}")
        print(f"  animation_mode:  {hdr.animation_mode}  {'(mode 1)' if hdr.animation_mode == 1 else '(mode 0)' if hdr.animation_mode == 0 else '⚠️ UNEXPECTED!'}")
        print(f"  clut_value:      0x{hdr.clut_value:04X} -> VRAM({hdr.clut_x}, {hdr.clut_y})")
        
        print()
        print("FRAME METADATA (36 bytes each)")
        print("-" * 90)
        print(f"  {'Fr':>2}  {'Fl':>2}  {'Size':>9}  {'Render':>12}  {'OffAdj':>12}  {'Anchor':>12}  {'Clip':>9}  {'Col':>3}  {'RLE':>8}")
        print("-" * 90)
        
        for i, frame in enumerate(sprite['frames']):
            # Highlight non-zero values in offset_adjust and collision
            off_adj = f"({frame.offset_adjust_x:4d},{frame.offset_adjust_y:4d})"
            if frame.offset_adjust_x != 0 or frame.offset_adjust_y != 0:
                off_adj = f"\033[33m{off_adj}\033[0m"  # Yellow
            
            col_str = f"{frame.collision_flag:3d}"
            if frame.collision_flag != 0:
                col_str = f"\033[32m{col_str}\033[0m"  # Green
            
            rt_ptr = ""
            if frame.runtime_ptr != 0:
                rt_ptr = f" rt=0x{frame.runtime_ptr:08X}"
            
            print(f"  {i:2d}  {frame.flags:2d}  {frame.render_width:4d}x{frame.render_height:<4d}  "
                  f"({frame.render_x:4d},{frame.render_y:4d})  {off_adj}  "
                  f"({frame.anchor_x:4d},{frame.anchor_y:4d})  {frame.clip_width:4d}x{frame.clip_height:<4d}  "
                  f"{col_str}  0x{frame.rle_offset:05X}{rt_ptr}")
        
        # Check for patterns
        print()
        print("PATTERN ANALYSIS")
        print("-" * 40)
        
        # Check reserved fields
        has_nonzero_1a = any(f.reserved_1a != 0 for f in sprite['frames'])
        has_nonzero_1e = any(f.reserved_1e != 0 for f in sprite['frames'])
        has_nonzero_rt = any(f.runtime_ptr != 0 for f in sprite['frames'])
        has_offset_adj = any(f.offset_adjust_x != 0 or f.offset_adjust_y != 0 for f in sprite['frames'])
        has_collision = any(f.collision_flag != 0 for f in sprite['frames'])
        
        print(f"  reserved_1A non-zero: {'YES ⚠️' if has_nonzero_1a else 'no'}")
        print(f"  reserved_1E non-zero: {'YES ⚠️' if has_nonzero_1e else 'no'}")
        print(f"  runtime_ptr non-zero: {'YES' if has_nonzero_rt else 'no'}")
        print(f"  offset_adjust used:   {'YES (highlighted in yellow)' if has_offset_adj else 'no'}")
        print(f"  collision_flag set:   {'YES (highlighted in green)' if has_collision else 'no'}")
        
        if has_nonzero_1a:
            print(f"\n  reserved_1A values: {[f.reserved_1a for f in sprite['frames'] if f.reserved_1a != 0]}")
        if has_nonzero_1e:
            print(f"  reserved_1E values: {[f.reserved_1e for f in sprite['frames'] if f.reserved_1e != 0]}")
        
        print()
        print("Press Enter to go back: ", end="", flush=True)
    
    def run(self):
        """Main loop."""
        while True:
            self.show_level_list()
            choice = input().strip().lower()
            
            if choice == 'q':
                break
            
            try:
                level_idx = int(choice)
                if 0 <= level_idx < len(self.levels):
                    self.run_level(self.levels[level_idx])
            except ValueError:
                continue
    
    def run_level(self, level):
        """Level sprite browser."""
        sprites = self.get_level_sprites(level)
        
        while True:
            self.show_sprite_list(level, sprites)
            choice = input().strip().lower()
            
            if choice == 'b' or choice == '':
                break
            
            try:
                sprite_idx = int(choice)
                # Find sprite by index
                for sp in sprites:
                    if sp['index'] == sprite_idx:
                        self.show_sprite_detail(level, sp)
                        input()
                        break
            except ValueError:
                continue


def main():
    blb_path = sys.argv[1] if len(sys.argv) > 1 else "disks/blb/GAME.BLB"
    
    if not Path(blb_path).exists():
        print(f"Error: BLB file not found: {blb_path}")
        sys.exit(1)
    
    viewer = SpriteViewer(blb_path)
    viewer.run()


if __name__ == "__main__":
    main()
