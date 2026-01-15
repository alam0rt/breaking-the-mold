#!/usr/bin/env python3
"""
Update splat configuration with functions from Ghidra.

This script:
1. Reads the current splat config (config/splat.pal.yaml)
2. Queries Ghidra for all defined functions
3. Automatically inserts/updates C segment entries [offset, c, FunctionName]
4. Preserves existing manual segments (rodata, data, asm)
5. Validates segment alignment and gaps

Usage:
    # Update config with all Ghidra functions
    python3 scripts/update_splat_config.py

    # Dry run to see changes
    python3 scripts/update_splat_config.py --dry-run

    # Update only specific functions
    python3 scripts/update_splat_config.py --filter "EntityType"

    # Force overwrite existing C segments
    python3 scripts/update_splat_config.py --force
"""

import argparse
import json
import re
import sys
import yaml
from pathlib import Path
from typing import Dict, List, Tuple, Optional, Any
from urllib.request import urlopen, Request
from urllib.error import URLError

DEFAULT_PORT = 8192
CONFIG_FILE = "config/splat.pal.yaml"
RAM_BASE = 0x80010000
ROM_BASE = 0x800


def ghidra_request(endpoint: str, port: int = DEFAULT_PORT, method: str = "GET", data: dict = None) -> dict:
    """Make a request to the Ghidra MCP bridge."""
    url = f"http://localhost:{port}{endpoint}"
    try:
        if data:
            req = Request(
                url,
                data=json.dumps(data).encode('utf-8'),
                headers={'Content-Type': 'application/json'},
                method=method
            )
        else:
            req = Request(url, method=method)
        with urlopen(req, timeout=30) as response:
            return json.loads(response.read().decode())
    except URLError as e:
        return {"error": str(e)}


def get_all_functions(port: int, limit: int = 10000) -> List[dict]:
    """Get all functions from Ghidra."""
    result = ghidra_request(f"/functions?limit={limit}", port)
    if result.get("success"):
        # MCP API returns functions in 'result', not 'functions'
        return result.get("result", result.get("functions", []))
    return []


def vram_to_rom(vram_addr: int, vram_base: int = RAM_BASE, rom_base: int = ROM_BASE) -> int:
    """Convert VRAM address to ROM offset."""
    return (vram_addr - vram_base) + rom_base


def rom_to_vram(rom_offset: int, vram_base: int = RAM_BASE, rom_base: int = ROM_BASE) -> int:
    """Convert ROM offset to VRAM address."""
    return (rom_offset - rom_base) + vram_base


class SplatSegment:
    """Represents a subsegment in splat config."""
    
    def __init__(self, offset: int, seg_type: str, name: Optional[str] = None, **kwargs):
        self.offset = offset
        self.seg_type = seg_type
        self.name = name
        self.extra = kwargs  # Additional fields (vram, etc.)
    
    def to_list(self) -> list:
        """Convert to splat YAML list format."""
        result = [f"0x{self.offset:X}", self.seg_type]
        if self.name:
            result.append(self.name)
        return result
    
    def __repr__(self):
        name_str = f", {self.name}" if self.name else ""
        return f"SplatSegment(0x{self.offset:X}, {self.seg_type}{name_str})"
    
    @staticmethod
    def from_list(item: list) -> 'SplatSegment':
        """Parse from splat YAML list format."""
        if len(item) < 2:
            raise ValueError(f"Invalid segment: {item}")
        
        offset = item[0]
        if isinstance(offset, str):
            offset = int(offset, 16) if offset.startswith('0x') else int(offset)
        
        seg_type = item[1]
        name = item[2] if len(item) > 2 else None
        
        return SplatSegment(offset, seg_type, name)


class SplatConfig:
    """Represents a splat configuration file."""
    
    def __init__(self, config_path: str):
        self.config_path = config_path
        self.config = {}
        self.segments = []
        self.main_segment = None
        self.main_segment_idx = -1
        self.load()
    
    def load(self):
        """Load the YAML configuration."""
        with open(self.config_path, 'r') as f:
            self.config = yaml.safe_load(f)
        
        # Find the main code segment
        for idx, segment in enumerate(self.config.get('segments', [])):
            if segment.get('type') == 'code' and segment.get('name') == 'main':
                self.main_segment = segment
                self.main_segment_idx = idx
                
                # Parse subsegments
                subsegments = segment.get('subsegments', [])
                self.segments = []
                for item in subsegments:
                    if isinstance(item, list):
                        self.segments.append(SplatSegment.from_list(item))
                    elif isinstance(item, dict):
                        # Handle dictionary-style entries (if any)
                        offset = item.get('start', 0)
                        seg_type = item.get('type', 'asm')
                        name = item.get('name')
                        self.segments.append(SplatSegment(offset, seg_type, name))
                break
        
        if not self.main_segment:
            raise ValueError("Could not find main code segment in config")
    
    def get_vram_base(self) -> int:
        """Get the VRAM base address for the main segment."""
        return self.main_segment.get('vram', RAM_BASE)
    
    def get_rom_start(self) -> int:
        """Get the ROM start offset for the main segment."""
        return self.main_segment.get('start', ROM_BASE)
    
    def find_segment_at_offset(self, offset: int) -> Optional[SplatSegment]:
        """Find segment at exact offset."""
        for seg in self.segments:
            if seg.offset == offset:
                return seg
        return None
    
    def find_segment_containing(self, offset: int) -> Optional[Tuple[SplatSegment, int]]:
        """Find segment containing offset, return (segment, index)."""
        for idx, seg in enumerate(self.segments):
            next_offset = self.segments[idx + 1].offset if idx + 1 < len(self.segments) else float('inf')
            if seg.offset <= offset < next_offset:
                return seg, idx
        return None, -1
    
    def insert_or_update_segment(self, offset: int, seg_type: str, name: str, 
                                  force: bool = False) -> Tuple[str, Optional[SplatSegment]]:
        """
        Insert or update a segment at the given offset.
        Returns: (action, old_segment) where action is 'inserted', 'updated', 'skipped'
        """
        # Check if segment already exists
        existing = self.find_segment_at_offset(offset)
        
        if existing:
            if existing.seg_type == 'c' and existing.name == name:
                return 'skipped', existing
            
            if not force and existing.seg_type == 'c':
                # Don't overwrite existing C segment unless forced
                return 'skipped', existing
            
            # Update existing segment
            old_seg = SplatSegment(existing.offset, existing.seg_type, existing.name)
            existing.seg_type = seg_type
            existing.name = name
            return 'updated', old_seg
        
        # Find insertion point (maintain sorted order)
        insert_idx = 0
        for idx, seg in enumerate(self.segments):
            if seg.offset < offset:
                insert_idx = idx + 1
            else:
                break
        
        # Insert new segment
        new_seg = SplatSegment(offset, seg_type, name)
        self.segments.insert(insert_idx, new_seg)
        return 'inserted', None
    
    def save(self, output_path: Optional[str] = None):
        """Save the configuration back to YAML."""
        if output_path is None:
            output_path = self.config_path
        
        # Convert segments back to list format
        subsegments = []
        for seg in self.segments:
            subsegments.append(seg.to_list())
        
        # Update main segment
        self.main_segment['subsegments'] = subsegments
        self.config['segments'][self.main_segment_idx] = self.main_segment
        
        # Write YAML
        with open(output_path, 'w') as f:
            yaml.dump(self.config, f, default_flow_style=False, sort_keys=False, width=120)
        
        print(f"Saved config to {output_path}", file=sys.stderr)


def update_config_from_ghidra(config: SplatConfig, port: int, 
                               filter_pattern: Optional[str] = None,
                               force: bool = False, dry_run: bool = False):
    """Update splat config with functions from Ghidra."""
    print("Fetching functions from Ghidra...", file=sys.stderr)
    functions = get_all_functions(port)
    
    if not functions:
        print("Error: No functions found in Ghidra", file=sys.stderr)
        return False
    
    print(f"Found {len(functions)} functions in Ghidra", file=sys.stderr)
    
    # Filter functions
    valid_functions = []
    for func in functions:
        address = func.get("entryPoint", 0)
        if isinstance(address, str):
            address = int(address, 16)
        
        name = func.get("name", "")
        
        # Skip auto-generated names
        if name.startswith("FUN_") or name.startswith("LAB_"):
            continue
        
        # Apply filter if specified
        if filter_pattern and filter_pattern not in name:
            continue
        
        # Calculate ROM offset
        rom_offset = vram_to_rom(address, config.get_vram_base(), config.get_rom_start())
        
        valid_functions.append({
            'name': name,
            'vram': address,
            'rom_offset': rom_offset
        })
    
    print(f"Processing {len(valid_functions)} valid functions...", file=sys.stderr)
    
    if dry_run:
        print("\n(DRY RUN - no changes will be made)\n", file=sys.stderr)
    
    inserted = 0
    updated = 0
    skipped = 0
    
    for func in sorted(valid_functions, key=lambda f: f['rom_offset']):
        action, old_seg = config.insert_or_update_segment(
            func['rom_offset'],
            'c',
            func['name'],
            force=force
        )
        
        if action == 'inserted':
            print(f"  [+] 0x{func['rom_offset']:X} (0x{func['vram']:08X}): {func['name']}")
            inserted += 1
        elif action == 'updated':
            print(f"  [*] 0x{func['rom_offset']:X}: {old_seg.seg_type}/{old_seg.name} -> c/{func['name']}")
            updated += 1
        elif action == 'skipped':
            skipped += 1
    
    print(f"\nSummary:", file=sys.stderr)
    print(f"  Inserted: {inserted}", file=sys.stderr)
    print(f"  Updated: {updated}", file=sys.stderr)
    print(f"  Skipped: {skipped}", file=sys.stderr)
    
    if not dry_run and (inserted > 0 or updated > 0):
        config.save()
    
    return True


def validate_config(config: SplatConfig):
    """Validate segment alignment and report potential issues."""
    print("Validating configuration...", file=sys.stderr)
    
    issues = []
    
    # Check for overlaps
    for idx in range(len(config.segments) - 1):
        curr = config.segments[idx]
        next_seg = config.segments[idx + 1]
        
        if curr.offset >= next_seg.offset:
            issues.append(f"Overlap: {curr} >= {next_seg}")
    
    # Check for very small segments (might be errors)
    for idx in range(len(config.segments) - 1):
        curr = config.segments[idx]
        next_seg = config.segments[idx + 1]
        size = next_seg.offset - curr.offset
        
        if curr.seg_type == 'c' and size < 4:
            issues.append(f"Very small C segment: {curr} (size: {size} bytes)")
    
    if issues:
        print("\n=== Validation Issues ===")
        for issue in issues:
            print(f"  {issue}")
        return False
    else:
        print("✓ Configuration is valid", file=sys.stderr)
        return True


def main():
    parser = argparse.ArgumentParser(
        description="Update splat configuration with functions from Ghidra"
    )
    parser.add_argument("--port", type=int, default=DEFAULT_PORT,
                        help=f"Ghidra MCP port (default: {DEFAULT_PORT})")
    parser.add_argument("--config", type=str, default=CONFIG_FILE,
                        help=f"Splat config file (default: {CONFIG_FILE})")
    parser.add_argument("--dry-run", action="store_true",
                        help="Show what would change without modifying files")
    parser.add_argument("--filter", type=str, default=None,
                        help="Only process functions matching this pattern")
    parser.add_argument("--force", action="store_true",
                        help="Overwrite existing C segments")
    parser.add_argument("--validate", action="store_true",
                        help="Only validate the configuration (don't update)")
    
    args = parser.parse_args()
    
    try:
        # Load config
        print(f"Loading {args.config}...", file=sys.stderr)
        config = SplatConfig(args.config)
        print(f"Loaded {len(config.segments)} subsegments", file=sys.stderr)
        
        if args.validate:
            success = validate_config(config)
        else:
            success = update_config_from_ghidra(
                config,
                args.port,
                args.filter,
                args.force,
                args.dry_run
            )
            
            if success and not args.dry_run:
                validate_config(config)
        
        sys.exit(0 if success else 1)
    
    except KeyboardInterrupt:
        print("\nInterrupted by user", file=sys.stderr)
        sys.exit(130)
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == "__main__":
    main()
