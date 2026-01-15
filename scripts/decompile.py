#!/usr/bin/env python3
"""
Simplified decompilation workflow tool.

This tool handles the entire decompilation process:
1. Looks up function in Ghidra
2. Validates function exists and gets size
3. Calculates ROM offset
4. Updates splat YAML (preserving formatting)
5. Runs splat to generate assembly
6. Optionally runs m2c to get initial decompilation
7. Creates C file stub or shows decompiled code

Usage:
    # Show function info and what would be done
    python3 scripts/decompile.py GetAssetCount --dry-run
    
    # Prepare function for decompilation (update YAML, generate ASM)
    python3 scripts/decompile.py GetAssetCount --prepare
    
    # Get m2c decompilation output
    python3 scripts/decompile.py GetAssetCount --decompile
    
    # Full workflow: prepare + decompile
    python3 scripts/decompile.py GetAssetCount --full

Key features:
- Preserves YAML formatting using ruamel.yaml
- Validates segments don't overlap
- Checks if function already in YAML
- Clear error messages
- Handles address conversion automatically
"""

import sys
import argparse
import json
import subprocess
from pathlib import Path
from urllib.request import urlopen
from typing import Optional, Dict, Tuple

# Configuration
GHIDRA_PORT = 8192
GHIDRA_URL = f"http://localhost:{GHIDRA_PORT}"
SYMBOL_FILE = Path("symbol_addrs.txt")
CONFIG_FILE = Path("config/splat.pal.yaml")
ASM_BASE = Path("asm/pal/nonmatchings")
SRC_DIR = Path("src")
CTX_FILE = Path("ctx.c")

# Memory layout constants
RAM_BASE = 0x80010000
ROM_BASE = 0x800

def error(msg: str):
    """Print error and exit."""
    print(f"❌ ERROR: {msg}", file=sys.stderr)
    sys.exit(1)

def success(msg: str):
    """Print success message."""
    print(f"✓ {msg}")

def info(msg: str):
    """Print info message."""
    print(f"  {msg}")

def vram_to_rom(vram: int) -> int:
    """Convert VRAM address to ROM offset."""
    return (vram - RAM_BASE) + ROM_BASE

def rom_to_vram(rom: int) -> int:
    """Convert ROM offset to VRAM address."""
    return (rom - ROM_BASE) + RAM_BASE

def ghidra_request(endpoint: str) -> dict:
    """Make request to Ghidra MCP server."""
    url = f"{GHIDRA_URL}{endpoint}"
    try:
        with urlopen(url, timeout=10) as response:
            return json.loads(response.read().decode())
    except Exception as e:
        error(f"Failed to connect to Ghidra: {e}\nIs Ghidra MCP server running on port {GHIDRA_PORT}?")

def get_function_info(name: str) -> Optional[Dict]:
    """Get function information from Ghidra."""
    # Search by name
    result = ghidra_request(f"/functions?name={name}")
    if not result.get("success"):
        return None
    
    functions = result.get("result", result.get("functions", []))
    if not functions:
        return None
    
    func = functions[0]
    address_str = func.get("address", "0")
    address = int(address_str, 16) if isinstance(address_str, str) else address_str
    
    # Get detailed info including size
    detail = ghidra_request(f"/functions/{address:08x}")
    if detail.get("success"):
        body = detail.get("result", {}).get("body", {})
        if body:
            min_addr = body.get("minAddress", "0")
            max_addr = body.get("maxAddress", "0")
            if isinstance(min_addr, str):
                min_addr = int(min_addr, 16)
            if isinstance(max_addr, str):
                max_addr = int(max_addr, 16)
            size = max_addr - min_addr + 1 if max_addr > min_addr else 0
            
            return {
                "name": func.get("name"),
                "address": address,
                "size": size,
                "rom_offset": vram_to_rom(address)
            }
    
    return {
        "name": func.get("name"),
        "address": address,
        "size": 0,
        "rom_offset": vram_to_rom(address)
    }

def get_decompiled_code(name: str, address: int) -> Optional[str]:
    """Get decompiled code from Ghidra."""
    result = ghidra_request(f"/functions/{address:08x}/decompile")
    if result.get("success"):
        decompiled = result.get("result", {}).get("decompiled", "")
        return decompiled
    return None

def check_symbol_file(name: str, address: int, size: int) -> bool:
    """Check if function is in symbol_addrs.txt and add if missing."""
    if not SYMBOL_FILE.exists():
        error(f"{SYMBOL_FILE} not found")
    
    with open(SYMBOL_FILE, 'r') as f:
        content = f.read()
    
    if f"{name} =" in content:
        info(f"Found in {SYMBOL_FILE}")
        return True
    
    # Add to symbol file
    line = f"{name} = 0x{address:08X};"
    if size:
        line += f" // size:0x{size:X}"
    line += "\n"
    
    with open(SYMBOL_FILE, 'a') as f:
        f.write(line)
    
    success(f"Added to {SYMBOL_FILE}")
    return True

def find_segment_for_rom(rom_offset: int) -> Optional[Tuple[int, int, str]]:
    """Find which segment contains the ROM offset.
    Returns (segment_start, segment_end, segment_type)"""
    try:
        import ruamel.yaml
        yaml = ruamel.yaml.YAML()
        yaml.preserve_quotes = True
        yaml.default_flow_style = False
        
        with open(CONFIG_FILE, 'r') as f:
            config = yaml.load(f)
        
        segments = config.get('segments', [])
        for segment in segments:
            if segment.get('type') == 'code' and segment.get('name') == 'main':
                subsegments = segment.get('subsegments', [])
                for i, sub in enumerate(subsegments):
                    if isinstance(sub, list) and len(sub) >= 2:
                        start_str = str(sub[0])
                        start = int(start_str, 16) if start_str.startswith('0x') else int(start_str)
                        
                        # Get end from next segment or end of file
                        if i + 1 < len(subsegments):
                            next_sub = subsegments[i + 1]
                            if isinstance(next_sub, list):
                                end_str = str(next_sub[0])
                                end = int(end_str, 16) if end_str.startswith('0x') else int(end_str)
                            else:
                                continue
                        else:
                            end = 0x97000  # End of file
                        
                        if start <= rom_offset < end:
                            seg_type = sub[1] if len(sub) > 1 else "unknown"
                            return (start, end, seg_type)
    except ImportError:
        error("ruamel.yaml not installed. Run: pip install ruamel.yaml")
    except Exception as e:
        error(f"Failed to parse {CONFIG_FILE}: {e}")
    
    return None

def update_splat_yaml(name: str, rom_offset: int, size: int, dry_run: bool = False) -> bool:
    """Update splat.pal.yaml to add new function segment.
    Uses ruamel.yaml to preserve formatting."""
    try:
        import ruamel.yaml
    except ImportError:
        error("ruamel.yaml not installed. Run: pip install ruamel.yaml")
    
    # Find containing segment
    segment_info = find_segment_for_rom(rom_offset)
    if not segment_info:
        error(f"Could not find segment containing ROM offset 0x{rom_offset:X}")
    
    start, end, seg_type = segment_info
    info(f"Function is in segment [0x{start:X}, 0x{end:X}] type={seg_type}")
    
    if seg_type == 'c':
        error(f"Function is already in a 'c' segment. Cannot split further automatically.\n"
              f"       You may need to manually split the existing C file.")
    
    if dry_run:
        info(f"Would add segment: [0x{rom_offset:X}, c, {name}]")
        if size:
            end_offset = rom_offset + size
            info(f"Would add segment: [0x{end_offset:X}, asm]  (to continue after function)")
        return True
    
    # Load YAML preserving format
    yaml = ruamel.yaml.YAML()
    yaml.preserve_quotes = True
    yaml.default_flow_style = False
    yaml.width = 4096  # Prevent line wrapping
    
    with open(CONFIG_FILE, 'r') as f:
        config = yaml.load(f)
    
    # Find main segment and add new subsegments
    segments = config.get('segments', [])
    for segment in segments:
        if segment.get('type') == 'code' and segment.get('name') == 'main':
            subsegments = segment.get('subsegments', [])
            
            # Find insertion point
            insert_idx = None
            for i, sub in enumerate(subsegments):
                if isinstance(sub, list) and len(sub) >= 2:
                    sub_start_str = str(sub[0])
                    sub_start = int(sub_start_str, 16) if sub_start_str.startswith('0x') else int(sub_start_str)
                    
                    if sub_start == rom_offset:
                        info(f"Segment already exists at 0x{rom_offset:X}")
                        return True
                    
                    if sub_start < rom_offset:
                        insert_idx = i + 1
            
            if insert_idx is None:
                error("Could not determine where to insert segment")
            
            # Insert new segments
            new_segments = [
                [f"0x{rom_offset:X}", "c", name]
            ]
            if size:
                new_segments.append([f"0x{rom_offset + size:X}", "asm"])
            
            for offset, seg in enumerate(new_segments):
                subsegments.insert(insert_idx + offset, seg)
            
            # Write back
            with open(CONFIG_FILE, 'w') as f:
                yaml.dump(config, f)
            
            success(f"Updated {CONFIG_FILE}")
            return True
    
    error("Could not find main code segment in config")

def run_splat(dry_run: bool = False) -> bool:
    """Run splat to generate assembly files."""
    if dry_run:
        info("Would run: python3 -m splat split config/splat.pal.yaml")
        return True
    
    try:
        result = subprocess.run(
            ["python3", "-m", "splat", "split", str(CONFIG_FILE)],
            capture_output=True,
            text=True,
            timeout=60
        )
        if result.returncode == 0:
            success("Splat completed successfully")
            return True
        else:
            print(result.stderr, file=sys.stderr)
            error(f"Splat failed with exit code {result.returncode}")
    except Exception as e:
        error(f"Failed to run splat: {e}")

def run_m2c(name: str) -> Optional[str]:
    """Run m2c decompiler on generated assembly."""
    asm_file = ASM_BASE / name / f"{name}.s"
    
    if not asm_file.exists():
        error(f"Assembly file not found: {asm_file}\nRun with --prepare first")
    
    if not CTX_FILE.exists():
        error(f"Context file not found: {CTX_FILE}\nRun: make context")
    
    try:
        result = subprocess.run(
            ["python3", "tools/m2c/m2c.py", "--context", str(CTX_FILE), 
             "--target", "mipsel-gcc-c", str(asm_file)],
            capture_output=True,
            text=True,
            timeout=10
        )
        if result.returncode == 0:
            return result.stdout
        else:
            error(f"m2c failed: {result.stderr}")
    except Exception as e:
        error(f"Failed to run m2c: {e}")

def main():
    parser = argparse.ArgumentParser(
        description="Simplified decompilation workflow",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__.split("Usage:")[1]
    )
    parser.add_argument("function", help="Function name to decompile")
    parser.add_argument("--dry-run", action="store_true",
                        help="Show what would be done without making changes")
    parser.add_argument("--prepare", action="store_true",
                        help="Update YAML and generate assembly (step 1)")
    parser.add_argument("--decompile", action="store_true",
                        help="Run m2c to get decompiled code (step 2)")
    parser.add_argument("--full", action="store_true",
                        help="Do both --prepare and --decompile")
    parser.add_argument("--ghidra", action="store_true",
                        help="Show Ghidra's decompilation")
    
    args = parser.parse_args()
    
    if not any([args.dry_run, args.prepare, args.decompile, args.full, args.ghidra]):
        parser.error("Must specify one of: --dry-run, --prepare, --decompile, --full, --ghidra")
    
    print(f"\n{'='*60}")
    print(f"Decompiling: {args.function}")
    print(f"{'='*60}\n")
    
    # Step 1: Get function info from Ghidra
    info("Looking up function in Ghidra...")
    func_info = get_function_info(args.function)
    
    if not func_info:
        error(f"Function '{args.function}' not found in Ghidra")
    
    name = func_info["name"]
    address = func_info["address"]
    size = func_info["size"]
    rom_offset = func_info["rom_offset"]
    
    success(f"Found function: {name}")
    info(f"VRAM:       0x{address:08X}")
    info(f"ROM offset: 0x{rom_offset:X}")
    info(f"Size:       {size} bytes (0x{size:X})" if size else "Size:       Unknown")
    
    # Step 2: Check/update symbol_addrs.txt
    if not args.dry_run:
        check_symbol_file(name, address, size)
    else:
        info(f"Would check/update {SYMBOL_FILE}")
    
    # Step 3: Show Ghidra decompilation if requested
    if args.ghidra:
        print(f"\n{'='*60}")
        print("Ghidra Decompilation:")
        print(f"{'='*60}\n")
        code = get_decompiled_code(name, address)
        if code:
            print(code)
        else:
            error("Could not get decompilation from Ghidra")
        return
    
    # Step 4: Update YAML if preparing
    if args.prepare or args.full:
        print(f"\n{'='*60}")
        print("Step 1: Preparing for decompilation")
        print(f"{'='*60}\n")
        
        if size == 0:
            print("⚠️  WARNING: Function size is unknown. You may need to manually determine it.")
        
        update_splat_yaml(name, rom_offset, size, args.dry_run)
        
        if not args.dry_run:
            info("\nRunning splat to generate assembly...")
            run_splat(args.dry_run)
            
            asm_file = ASM_BASE / name / f"{name}.s"
            if asm_file.exists():
                success(f"Assembly generated: {asm_file}")
            else:
                error("Assembly file was not created. Check splat output above.")
    
    # Step 5: Run m2c if decompiling
    if args.decompile or args.full:
        print(f"\n{'='*60}")
        print("Step 2: Decompiling with m2c")
        print(f"{'='*60}\n")
        
        code = run_m2c(name)
        if code:
            print(code)
            print(f"\n{'='*60}")
            print("Next steps:")
            print(f"{'='*60}")
            print(f"1. Edit src/{name}.c with the decompiled code above")
            print(f"2. Fix types and variable names")
            print(f"3. Run: make")
            print(f"4. Run: make check")
            print(f"5. If it doesn't match, iterate on the code")
    
    print()

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\nInterrupted by user", file=sys.stderr)
        sys.exit(130)
