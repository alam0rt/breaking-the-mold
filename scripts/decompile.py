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

Additional features:
- Export struct definitions from Ghidra to C headers
- Sync verified structs between Ghidra and source

Usage:
    # Show function info and what would be done
    python3 scripts/decompile.py GetAssetCount --dry-run
    
    # Prepare function for decompilation (update YAML, generate ASM)
    python3 scripts/decompile.py GetAssetCount --prepare
    
    # Get m2c decompilation output
    python3 scripts/decompile.py GetAssetCount --decompile
    
    # Full workflow: prepare + decompile
    python3 scripts/decompile.py GetAssetCount --full
    
    # Export struct from Ghidra to C header format
    python3 scripts/decompile.py --export-struct PlayerState

Key features:
- Preserves YAML formatting using ruamel.yaml
- Validates segments don't overlap
- Checks if function already in YAML
- Clear error messages
- Handles address conversion automatically
- Exports struct definitions from Ghidra
"""

import sys
import argparse
import json
import re
import subprocess
import struct
from pathlib import Path
from urllib.request import urlopen
from typing import Optional, Dict, Tuple, List
from datetime import datetime

# Configuration
GHIDRA_PORT = 8192
GHIDRA_URL = f"http://localhost:{GHIDRA_PORT}"
SYMBOL_FILE = Path("symbol_addrs.txt")
CONFIG_FILE = Path("config/splat.pal.yaml")
ASM_BASE = Path("asm/pal/nonmatchings")
SRC_DIR = Path("src")
CTX_FILE = Path("ctx.c")
BINARY_PATH = Path("disks/pal/SLES_010.90")

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

def analyze_gap(start_addr: int, size: int, analyze_content: bool = True) -> Dict:
    """
    Analyze a gap in the symbol table to determine what kind of data it contains.
    
    Returns a dict with:
        - type: 'pointers', 'strings', 'zeros', 'code', 'mixed', 'unknown'
        - description: human-readable description
        - pointer_count: number of valid PSX pointers found
        - string_fragments: list of string snippets found
        - suggested_type: suggested Ghidra data type
    """
    result = {
        'type': 'unknown',
        'description': '',
        'pointer_count': 0,
        'string_fragments': [],
        'suggested_type': None,
        'zero_percent': 0,
    }
    
    if not analyze_content or not BINARY_PATH.exists():
        return result
    
    # Read from binary
    rom_offset = vram_to_rom(start_addr)
    if rom_offset < 0:
        return result
    
    try:
        with open(BINARY_PATH, 'rb') as f:
            f.seek(rom_offset)
            data = f.read(min(size, 1024))  # Read up to 1KB for analysis
    except Exception:
        return result
    
    if not data:
        return result
    
    # Count zeros
    zero_count = data.count(b'\x00')
    result['zero_percent'] = (zero_count * 100) // len(data)
    
    # Check for pointers (0x80xxxxxx pattern)
    pointer_count = 0
    pointer_targets = []
    for i in range(0, len(data) - 3, 4):
        word = struct.unpack('<I', data[i:i+4])[0]
        if 0x80010000 <= word <= 0x80200000:
            pointer_count += 1
            pointer_targets.append(word)
    result['pointer_count'] = pointer_count
    
    # Check for ASCII strings
    string_fragments = []
    current_string = b''
    for byte in data:
        if 0x20 <= byte <= 0x7E:  # Printable ASCII
            current_string += bytes([byte])
        else:
            if len(current_string) >= 4:
                try:
                    string_fragments.append(current_string.decode('ascii'))
                except:
                    pass
            current_string = b''
    if len(current_string) >= 4:
        try:
            string_fragments.append(current_string.decode('ascii'))
        except:
            pass
    result['string_fragments'] = string_fragments[:5]  # Keep first 5
    
    # Check for MIPS code patterns (common opcodes)
    code_like = 0
    for i in range(0, len(data) - 3, 4):
        word = struct.unpack('<I', data[i:i+4])[0]
        opcode = (word >> 26) & 0x3F
        # Common MIPS opcodes: addiu(9), lw(35), sw(43), lui(15), ori(13), beq(4), bne(5), jal(3), jr(0+8)
        if opcode in [9, 35, 43, 15, 13, 4, 5, 3, 0]:
            code_like += 1
    
    # Determine type
    total_words = len(data) // 4
    
    if result['zero_percent'] > 90:
        result['type'] = 'zeros'
        result['description'] = 'Mostly zeros (BSS or padding)'
        result['suggested_type'] = f'undefined[0x{size:X}]'
    elif pointer_count > total_words * 0.3:
        result['type'] = 'pointers'
        # Check if it's a regular pattern (vtable-like)
        if pointer_count >= 4 and len(set(pointer_targets[:8])) > 1:
            result['description'] = f'Pointer table ({pointer_count} ptrs)'
            result['suggested_type'] = f'pointer[{size // 4}]'
        else:
            result['description'] = f'Contains {pointer_count} pointers'
            result['suggested_type'] = f'pointer[{size // 4}]'
    elif string_fragments:
        result['type'] = 'strings'
        preview = string_fragments[0][:30]
        result['description'] = f'String data: "{preview}..."'
        result['suggested_type'] = 'char[]'
    elif code_like > total_words * 0.4:
        result['type'] = 'code'
        result['description'] = 'Likely code (undefined function?)'
        result['suggested_type'] = 'function'
    else:
        result['type'] = 'mixed'
        result['description'] = f'Mixed data ({result["zero_percent"]}% zeros, {pointer_count} ptrs)'
        result['suggested_type'] = f'undefined[0x{size:X}]'
    
    return result

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
    
    # Try to get size from disassembly (count instructions × 4 bytes)
    size = 0
    try:
        disasm = ghidra_request(f"/functions/{address:08x}/disassembly")
        if disasm.get("success"):
            instructions = disasm.get("result", {}).get("instructions", [])
            if instructions:
                size = len(instructions) * 4
    except:
        pass
    
    return {
        "name": func.get("name"),
        "address": address,
        "size": size,
        "rom_offset": vram_to_rom(address)
    }

def get_decompiled_code(name: str, address: int) -> Optional[str]:
    """Get decompiled code from Ghidra."""
    result = ghidra_request(f"/functions/{address:08x}/decompile")
    if result.get("success"):
        decompiled = result.get("result", {}).get("decompiled", "")
        return decompiled
    else:
        # Print detailed error from Ghidra
        error_msg = result.get("error", "Unknown error")
        print(f"❌ Ghidra decompilation failed: {error_msg}", file=sys.stderr)
        # Also print full result for debugging
        if result:
            print(f"   Full response: {json.dumps(result, indent=2)}", file=sys.stderr)
    return None

def export_struct_from_ghidra(struct_name: str) -> str:
    """
    Export a struct definition from Ghidra to C header format.
    
    Returns the C code as a string.
    """
    print(f"Fetching struct '{struct_name}' from Ghidra...")
    
    # Get struct details from Ghidra
    result = ghidra_request(f"/structs?name={struct_name}")
    
    if not result.get("success"):
        error(f"Struct '{struct_name}' not found in Ghidra")
    
    data = result["result"]
    fields = data.get("fields", [])
    size = data.get("size", 0)
    description = data.get("description", "")
    
    # Generate C code
    lines = []
    lines.append(f"/* {'-' * 77}")
    lines.append(f" * {struct_name}")
    if description:
        lines.append(f" * {description}")
    lines.append(f" * Size: {size} bytes (0x{size:X})")
    lines.append(f" * Exported from Ghidra")
    lines.append(f" * {'-' * 77} */")
    lines.append("")
    lines.append(f"typedef struct {{")
    
    # Track used offsets to detect padding
    last_offset = 0
    
    for field in fields:
        offset = field["offset"]
        name = field["name"] or f"field_{offset:02X}"
        field_type = field["type"]
        length = field["length"]
        comment = field.get("comment", "")
        
        # Add padding if needed
        if offset > last_offset:
            padding_size = offset - last_offset
            lines.append(f"    /* 0x{last_offset:02X} */ u8      padding{last_offset:02X}[{padding_size}];")
        
        # Map Ghidra types to C types
        type_map = {
            "byte": "u8",
            "word": "u16",
            "dword": "u32",
            "qword": "u64",
            "undefined": "u8",
        }
        c_type = type_map.get(field_type, field_type)
        
        # Format the field
        comment_str = f"  // {comment}" if comment else ""
        lines.append(f"    /* 0x{offset:02X} */ {c_type:<8} {name};{comment_str}")
        
        last_offset = offset + length
    
    lines.append(f"}} {struct_name};  // Size: 0x{size:02X} ({size} bytes)")
    lines.append("")
    
    return "\n".join(lines)


def export_struct_to_lua(struct_name: str) -> str:
    """
    Export a struct definition from Ghidra to Lua table format.
    
    Output is compatible with game_watcher.lua's ADDR table and
    can be directly included or require()'d.
    
    Returns the Lua code as a string.
    """
    print(f"-- Fetching struct '{struct_name}' from Ghidra...", file=sys.stderr)
    
    # Get struct details from Ghidra
    result = ghidra_request(f"/structs?name={struct_name}")
    
    if not result.get("success"):
        error(f"Struct '{struct_name}' not found in Ghidra")
    
    data = result["result"]
    fields = data.get("fields", [])
    size = data.get("size", 0)
    description = data.get("description", "")
    
    # Generate Lua code
    lines = []
    lines.append(f"-- {'-' * 77}")
    lines.append(f"-- {struct_name}")
    if description:
        lines.append(f"-- {description}")
    lines.append(f"-- Size: {size} bytes (0x{size:X})")
    lines.append(f"-- Exported from Ghidra via: python3 scripts/decompile.py --export-lua {struct_name}")
    lines.append(f"-- {'-' * 77}")
    lines.append("")
    
    # Create prefix from struct name (e.g., PlayerState -> PS_, Entity -> ENT_)
    # Use common abbreviations or first letters of camelCase words
    abbrev_map = {
        "PlayerState": "PS",
        "Entity": "ENT",
        "GameState": "GS",
        "LevelDataContext": "LDC",
    }
    prefix = abbrev_map.get(struct_name, struct_name[:3].upper())
    
    lines.append(f"local {struct_name}_offsets = {{")
    lines.append(f"    _size = 0x{size:02X},  -- Total struct size")
    
    for field in fields:
        offset = field["offset"]
        name = field["name"] or f"field_{offset:02X}"
        field_type = field["type"]
        length = field["length"]
        comment = field.get("comment", "")
        
        # Map Ghidra types to Lua comments
        type_comment_map = {
            "byte": "u8",
            "word": "u16",
            "dword": "u32",
            "qword": "u64",
            "undefined": "u8",
            "pointer": "ptr",
        }
        type_hint = type_comment_map.get(field_type, field_type)
        
        # Format array types
        if length > 1 and field_type in ("byte", "undefined"):
            type_hint = f"u8[{length}]"
        elif length > 4:
            type_hint = f"[{length}]"
        
        # Build comment string
        comment_parts = [type_hint]
        if comment:
            comment_parts.append(comment)
        comment_str = " -- " + ", ".join(comment_parts)
        
        # Use snake_case field name for Lua
        lua_name = name
        lines.append(f"    {lua_name} = 0x{offset:02X},{comment_str}")
    
    lines.append("}")
    lines.append("")
    
    # Add convenience accessor comment
    lines.append(f"-- Usage in game_watcher.lua:")
    lines.append(f"-- local value = read_u8(struct_addr + {struct_name}_offsets.field_name)")
    lines.append("")
    lines.append(f"return {struct_name}_offsets")
    lines.append("")
    
    return "\n".join(lines)


def get_function_size_from_disasm(addr_str: str) -> int:
    """Get function size by parsing disassembly instructions."""
    try:
        disasm = ghidra_request(f"/functions/{addr_str}/disassembly")
        if not disasm.get("success"):
            return 0
        
        result = disasm.get("result", {})
        
        # Try instructions list first (newer API format)
        instructions = result.get("instructions", [])
        if instructions:
            first_addr = int(addr_str, 16)
            last_addr = first_addr
            
            for instr in instructions:
                addr = int(instr.get("address", "0"), 16)
                if addr > last_addr:
                    last_addr = addr
            
            # Size = (last instruction address - first) + 4 (instruction size)
            return (last_addr - first_addr) + 4
        
        # Fall back to disassembly_text if available
        text = result.get("disassembly_text", "")
        if text:
            lines = text.strip().split('\n')
            first_addr = int(addr_str, 16)
            last_addr = first_addr
            
            for line in lines:
                if ':' in line:
                    addr_part = line.split(':')[0].strip()
                    try:
                        addr = int(addr_part, 16)
                        if addr > last_addr:
                            last_addr = addr
                    except ValueError:
                        continue
            
            return (last_addr - first_addr) + 4
        
        return 0
    except Exception:
        return 0


def export_symbols_from_ghidra(name_filter: Optional[str] = None,
                               include_data: bool = True,
                               fast_mode: bool = False,
                               include_unnamed_funcs: bool = False,
                               include_unnamed_data: bool = False,
                               analyze_gaps: bool = False) -> str:
    """
    Export symbols from Ghidra to symbol_addrs.txt format.
    
    Format matches splat expectations with size:0xXX annotations.
    Similar to https://github.com/fmil95/soul-re/blob/main/symbol_addrs.txt
    
    Also validates continuity - reports gaps and overlaps between symbols.
    
    Args:
        name_filter: Optional substring filter for symbol names
        include_data: Include data symbols (not just functions)
        fast_mode: Skip fetching individual function sizes (much faster)
        include_unnamed_funcs: Include FUN_* auto-named functions
        include_unnamed_data: Include DAT_*, null_*, etc. auto-named data
        analyze_gaps: Analyze gap contents from binary (slower but informative)
        
    Returns:
        String in symbol_addrs.txt format
    """
    
    # Unified symbol entry: {name, addr, size, ignore, kind}
    all_symbols = []
    
    # ==================== COLLECT FUNCTIONS ====================
    print(f"-- Fetching functions from Ghidra...", file=sys.stderr)
    
    if name_filter:
        result = ghidra_request(f"/functions?name_contains={name_filter}&limit=3000")
    else:
        result = ghidra_request("/functions?limit=3000")
    
    if not result.get("success"):
        error("Failed to fetch functions from Ghidra")
    
    functions = result.get("result", [])
    print(f"-- Found {len(functions)} functions", file=sys.stderr)
    
    # Sort functions by address
    sorted_funcs = sorted(functions, key=lambda f: int(f.get("address", "0"), 16))
    
    for i, func in enumerate(sorted_funcs):
        name = func.get("name", "")
        addr_str = func.get("address", "0")
        
        # Skip unnamed/auto-generated functions unless flag is set
        if not name:
            continue
        if (name.startswith("FUN_") or name.startswith("LAB_")) and not include_unnamed_funcs:
            continue
        
        addr = int(addr_str, 16)
        
        # Determine address type and if it should be ignored by splat
        ignore = False
        
        # GTE coprocessor functions (0x20000000 range) - keep original addr
        if 0x20000000 <= addr < 0x21000000:
            ignore = True  # splat should ignore GTE macros
        # Hardware registers (0x1F800000 range)
        elif 0x1F800000 <= addr < 0x20000000:
            ignore = True
        # Normal RAM functions - add 0x80000000 prefix if needed
        elif addr < 0x80000000 and addr >= 0x00010000:
            addr |= 0x80000000
        
        # Get size from disassembly
        size = 0
        if not fast_mode:
            # Progress indicator
            if (i + 1) % 100 == 0:
                print(f"-- Processing function {i + 1}/{len(sorted_funcs)}...", file=sys.stderr)
            size = get_function_size_from_disasm(addr_str)
        
        all_symbols.append({
            'name': name,
            'addr': addr,
            'size': size,
            'ignore': ignore,
            'kind': 'func',
        })
    
    # ==================== COLLECT DATA SYMBOLS ====================
    if include_data:
        print(f"-- Fetching data symbols from Ghidra...", file=sys.stderr)
        
        data_result = ghidra_request("/data?limit=5000")
        
        if data_result.get("success"):
            data_items = data_result.get("result", [])
            print(f"-- Found {len(data_items)} data items", file=sys.stderr)
            
            # Type to size mapping
            type_sizes = {
                "byte": 1,
                "undefined1": 1,
                "word": 2,
                "undefined2": 2,
                "short": 2,
                "dword": 4,
                "undefined4": 4,
                "int": 4,
                "uint": 4,
                "pointer": 4,
                "float": 4,
                "qword": 8,
                "undefined8": 8,
                "double": 8,
            }
            
            for item in data_items:
                label = item.get("label", "")
                addr_str = item.get("address", "0")
                data_type = item.get("dataType", "")
                
                # Skip unnamed labels
                if not label:
                    continue
                if label == "(unnamed)" or label.startswith("("):
                    continue
                    
                # Skip auto-generated labels unless flag is set
                if not include_unnamed_data:
                    if label.startswith("DAT_") or label.startswith("null_"):
                        continue
                    # Skip switch data tables (auto-generated)
                    if label.startswith("switchdata"):
                        continue
                    # Skip Ghidra auto-names like "s_string_80010be8"
                    if label.startswith("s_") and "_800" in label:
                        continue
                
                addr = int(addr_str, 16)
                
                # Only include game address ranges
                if not (0x80000000 <= addr <= 0x80200000 or  # Main RAM
                        0x1F800000 <= addr <= 0x1F810000):    # Hardware registers
                    continue
                
                # Determine if this is a hardware address (ignore:true)
                ignore = (0x1F800000 <= addr < 0x20000000)
                
                # Get size from data type
                size = 0
                
                # Check for array types like "pointer[144]" or "SpriteTypeCallbackEntry[481]"
                array_match = re.match(r'(.+)\[(\d+)\]$', data_type)
                if array_match:
                    base_type = array_match.group(1)
                    array_count = int(array_match.group(2))
                    # Determine element size
                    if base_type in type_sizes:
                        elem_size = type_sizes[base_type]
                    elif base_type.endswith("*") or base_type == "pointer" or base_type == "addr":
                        elem_size = 4
                    else:
                        # Query Ghidra for struct size
                        elem_size = 4  # Default
                        try:
                            struct_resp = urlopen(f"{GHIDRA_URL}/structs?name={base_type}", timeout=5)
                            struct_data = json.loads(struct_resp.read().decode())
                            if struct_data.get("success") and struct_data.get("result"):
                                elem_size = struct_data["result"].get("size", 4)
                        except:
                            pass  # Use default
                    size = elem_size * array_count
                elif data_type in type_sizes:
                    size = type_sizes[data_type]
                elif data_type.endswith("*") or data_type.startswith("pointer"):
                    size = 4  # All pointers are 4 bytes on PSX
                elif data_type.startswith("undefined"):
                    # Try to extract size from undefinedN
                    try:
                        size = int(data_type.replace("undefined", ""))
                    except ValueError:
                        pass
                
                all_symbols.append({
                    'name': label,
                    'addr': addr,
                    'size': size,
                    'ignore': ignore,
                    'kind': 'data',
                    'data_type': data_type,
                })
    
    # ==================== SORT ALL SYMBOLS BY ADDRESS ====================
    all_symbols.sort(key=lambda s: s['addr'])
    
    # Calculate max name length for alignment
    max_name_len = max((len(s['name']) for s in all_symbols), default=40)
    max_name_len = min(max_name_len, 50)  # Cap at 50 chars
    
    # ==================== OUTPUT WITH CONTINUITY CHECKING ====================
    lines = []
    lines.append("// Symbol addresses exported from Ghidra")
    lines.append(f"// Generated by: python3 scripts/decompile.py --export-symbols" + 
                 (f" --symbol-filter {name_filter}" if name_filter else ""))
    lines.append(f"// Date: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    lines.append("//")
    lines.append("// Format: name = address; // size:0xXX")
    lines.append("//         GAP/OVERLAP comments show discontinuities")
    lines.append("")
    
    # Track continuity
    expected_next_addr = None
    prev_symbol_name = None
    prev_symbol_size = 0
    prev_symbol_type = None
    gap_count = 0
    overlap_count = 0
    func_count = 0
    data_count = 0
    
    for entry in all_symbols:
        name = entry['name']
        addr = entry['addr']
        size = entry['size']
        ignore = entry['ignore']
        kind = entry['kind']
        
        # Only check continuity for game RAM (0x80xxxxxx) with sizes
        is_game_ram = 0x80000000 <= addr < 0x80200000
        
        if is_game_ram and expected_next_addr is not None and size > 0:
            if addr > expected_next_addr:
                gap = addr - expected_next_addr
                gap_start = expected_next_addr
                
                if analyze_gaps and gap >= 0x10:  # Only analyze gaps >= 16 bytes
                    analysis = analyze_gap(gap_start, gap)
                    lines.append(f"// GAP: 0x{gap:X} bytes @ 0x{gap_start:08X} after {prev_symbol_name}")
                    lines.append(f"//      Type: {analysis['type']} - {analysis['description']}")
                    if analysis['suggested_type']:
                        lines.append(f"//      Suggested: {analysis['suggested_type']}")
                    # Check if previous symbol was a single pointer and gap is pointers
                    # This suggests they should be merged into one array
                    if (analysis['type'] == 'pointers' and 
                        prev_symbol_size == 4 and 
                        prev_symbol_type in ('pointer', 'addr', 'dword')):
                        total_count = (gap + 4) // 4
                        lines.append(f"//      HINT: Consider merging with {prev_symbol_name} as addr[{total_count}] at 0x{expected_next_addr - 4:08X}")
                else:
                    lines.append(f"// GAP: 0x{gap:X} bytes after {prev_symbol_name}")
                gap_count += 1
            elif addr < expected_next_addr:
                overlap = expected_next_addr - addr
                lines.append(f"// OVERLAP: 0x{overlap:X} bytes with {prev_symbol_name}")
                overlap_count += 1
        
        # Build annotation string
        annotations = []
        if size > 0:
            annotations.append(f"size:0x{size:X}")
        if ignore:
            annotations.append("ignore:true")
        
        # Format with alignment
        padded_name = name.ljust(max_name_len)
        if annotations:
            annotation_str = " // " + " ".join(annotations)
            lines.append(f"{padded_name} = 0x{addr:08X};{annotation_str}")
        else:
            lines.append(f"{padded_name} = 0x{addr:08X};")
        
        # Update expected next address
        if is_game_ram and size > 0:
            expected_next_addr = addr + size
            prev_symbol_name = name
            prev_symbol_size = size
            prev_symbol_type = entry.get('data_type', '')
        
        if kind == 'func':
            func_count += 1
        else:
            data_count += 1
    
    lines.append("")
    
    # Summary
    lines.append(f"// Total: {func_count} functions, {data_count} data symbols")
    lines.append(f"// Continuity: {gap_count} gaps, {overlap_count} overlaps")
    lines.append("")
    
    return "\n".join(lines)


def check_symbol_file(name: str, address: int, size: int, dry_run: bool = False) -> bool:
    """Check if function is in symbol_addrs.txt and add if missing."""
    if not SYMBOL_FILE.exists():
        error(f"{SYMBOL_FILE} not found")
    
    with open(SYMBOL_FILE, 'r') as f:
        lines = f.readlines()
    
    # Look for exact match or just name
    found_line = None
    found_idx = None
    for idx, line in enumerate(lines):
        if f"{name} =" in line:
            found_line = line.strip()
            found_idx = idx
            break
    
    if found_line:
        # Check if it matches what we expect
        expected = f"{name} = 0x{address:08X};"
        if size:
            expected += f" // size:0x{size:X}"
        
        if expected in found_line:
            info(f"Found in {SYMBOL_FILE} (line {found_idx + 1})")
            return True
        else:
            # Show difference
            info(f"Found in {SYMBOL_FILE} (line {found_idx + 1}) but differs:")
            print(f"    Existing: {found_line}")
            print(f"    Expected: {expected}")
            
            if not dry_run:
                # Update the line
                lines[found_idx] = expected + "\n"
                with open(SYMBOL_FILE, 'w') as f:
                    f.writelines(lines)
                success(f"Updated in {SYMBOL_FILE}")
            else:
                info("Would update this line")
            return True
    
    # Not found - need to add
    line = f"{name} = 0x{address:08X};"
    if size:
        line += f" // size:0x{size:X}"
    
    if dry_run:
        info(f"Not found in {SYMBOL_FILE}")
        print(f"    Would add: {line}")
        return True
    
    # Add to symbol file
    with open(SYMBOL_FILE, 'a') as f:
        f.write(line + "\n")
    
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
            
            # Insert new segments (use integers, not hex strings)
            # IMPORTANT: Splat requires integer offsets, not hex strings
            # Using hex strings like "0x161D4" will cause splat to fail with:
            # "ValueError: invalid literal for int() with base 10: '0x161D4'"
            new_segments = [
                [rom_offset, "c", name]
            ]
            if size:
                new_segments.append([rom_offset + size, "asm"])
            
            # Validate that we're not creating overlapping segments
            next_segment_start = None
            for i in range(insert_idx, len(subsegments)):
                if isinstance(subsegments[i], list) and len(subsegments[i]) >= 1:
                    next_str = str(subsegments[i][0])
                    next_segment_start = int(next_str, 16) if next_str.startswith('0x') else int(next_str)
                    break
            
            if next_segment_start and size and (rom_offset + size) > next_segment_start:
                error(f"Function size (0x{size:X}) would overlap with next segment at 0x{next_segment_start:X}\n"
                      f"       Function ends at 0x{rom_offset + size:X} but next segment starts at 0x{next_segment_start:X}\n"
                      f"       This may indicate incorrect function size from Ghidra,\n"
                      f"       or the function may share code with another function.")
                return False
            
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
        info("Running splat to generate assembly...")
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

def verify_asm_generated(name: str) -> bool:
    """Verify that ASM files were generated by splat."""
    asm_dir = ASM_BASE / name
    asm_file = asm_dir / f"{name}.s"
    
    if not asm_dir.exists():
        error(f"ASM directory not created: {asm_dir}\n"
              f"       Splat may not have split this segment correctly.\n"
              f"       Check that the segment name matches what was added to splat.pal.yaml")
        return False
    
    if not asm_file.exists():
        error(f"ASM file not found: {asm_file}\n"
              f"       Splat created the directory but no .s file.\n"
              f"       The function may have been decompiled to C already,\n"
              f"       or the segment might be empty.")
        return False
    
    success(f"Assembly generated: {asm_file}")
    return True

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
    parser.add_argument("function", nargs="?", help="Function name to decompile")
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
    parser.add_argument("--export-struct", metavar="NAME",
                        help="Export struct definition from Ghidra to C header format")
    parser.add_argument("--export-lua", metavar="NAME",
                        help="Export struct definition from Ghidra to Lua table format (for game_watcher.lua)")
    parser.add_argument("--export-symbols", action="store_true",
                        help="Export all named functions from Ghidra to symbol_addrs.txt format")
    parser.add_argument("--symbol-filter", metavar="PATTERN",
                        help="Filter symbols by name pattern (e.g., 'Entity' or 'Player')")
    parser.add_argument("--fast", action="store_true",
                        help="Fast mode for --export-symbols (skip individual function size lookups)")
    parser.add_argument("--no-data", action="store_true",
                        help="Exclude data symbols from --export-symbols output")
    parser.add_argument("--include-unnamed-funcs", action="store_true",
                        help="Include FUN_* auto-named functions in --export-symbols")
    parser.add_argument("--include-unnamed-data", action="store_true",
                        help="Include DAT_*, null_*, etc. auto-named data in --export-symbols")
    parser.add_argument("--analyze-gaps", action="store_true",
                        help="Analyze gap content (reads binary to detect pointers, strings, code)")
    
    args = parser.parse_args()
    
    # Handle struct export (doesn't need function name)
    if args.export_struct:
        code = export_struct_from_ghidra(args.export_struct)
        print(code)
        return
    
    # Handle Lua struct export (doesn't need function name)
    if args.export_lua:
        code = export_struct_to_lua(args.export_lua)
        print(code)
        return
    
    # Handle symbol export (doesn't need function name)
    if args.export_symbols:
        code = export_symbols_from_ghidra(
            name_filter=args.symbol_filter,
            include_data=not args.no_data,
            fast_mode=args.fast,
            include_unnamed_funcs=args.include_unnamed_funcs,
            include_unnamed_data=args.include_unnamed_data,
            analyze_gaps=args.analyze_gaps
        )
        print(code)
        return
    
    # All other operations need a function name
    if not args.function:
        parser.error("function name is required (unless using --export-struct, --export-lua, or --export-symbols)")
    
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
    check_symbol_file(name, address, size, args.dry_run)
    
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
            run_splat(args.dry_run)
            
            # Verify ASM was generated
            if not verify_asm_generated(name):
                error("Failed to generate assembly files")
    
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
            print(f"1. Create src/{name}.c with the decompiled code above")
            print(f"2. Fix types and variable names")
            print(f"3. Replace INCLUDE_ASM with actual function")
            print(f"4. Run: make")
            print(f"5. Run: make check")
            print(f"6. If it doesn't match, iterate on the code")
            print(f"\n⚠️  Note: Complex functions may fail in m2c. Use --ghidra as fallback.")
    
    print()

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\nInterrupted by user", file=sys.stderr)
        sys.exit(130)
