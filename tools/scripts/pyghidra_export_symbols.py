#!/usr/bin/env python3
"""
Export all symbols from a Ghidra project using PyGhidra (headless).

This script uses PyGhidra to directly access the Ghidra API, providing
much more accurate symbol information than the REST API, including:
- Accurate function sizes (body ranges)
- Proper parameter/return types
- Local variable information
- Decompiled signatures

Usage:
    # Export from existing Ghidra project
    pyghidra-export --project ~/ghidra_projects --name skullmonkeys --program SLES_010.90
    
    # Export specific address range
    pyghidra-export --project ~/ghidra_projects --name skullmonkeys --program SLES_010.90 \
        --range 0x80010000-0x80020000
    
    # Output formats
    pyghidra-export ... --format symbol_addrs > symbol_addrs.txt
    pyghidra-export ... --format yaml > symbols.yaml
    pyghidra-export ... --format json > symbols.json

Environment:
    GHIDRA_INSTALL_DIR - Path to Ghidra installation (or uses lastrun file)

Requirements:
    pip install pyghidra
"""

import argparse
import json
import os
import re
import sys
from pathlib import Path
from typing import Optional, Tuple, List, Dict, Any

# Memory layout constants for PSX
RAM_BASE = 0x80010000
ROM_BASE = 0x800


def vram_to_rom(vram: int) -> int:
    """Convert VRAM address to ROM offset."""
    if vram >= RAM_BASE:
        return (vram - RAM_BASE) + ROM_BASE
    return -1


def parse_address_range(range_str: str) -> Tuple[int, int]:
    """Parse address range like '0x80010000-0x80020000'."""
    if '-' in range_str:
        start, end = range_str.split('-')
        return int(start, 0), int(end, 0)
    else:
        return int(range_str, 0), 0xFFFFFFFF


def sanitize_name(name: str) -> str:
    """Sanitize symbol name for C identifier compatibility."""
    if not name:
        return "unnamed"
    # Replace common problematic patterns
    name = name.replace('??', 'void')
    name = name.replace('::', '_')
    name = name.replace('<', '_')
    name = name.replace('>', '_')
    name = name.replace(',', '_')
    name = name.replace(' ', '_')
    name = name.replace('*', 'ptr')
    name = name.replace('&', 'ref')
    # Keep only valid C identifier chars
    name = re.sub(r'[^a-zA-Z0-9_]', '_', name)
    # Remove consecutive underscores
    name = re.sub(r'_+', '_', name)
    # Remove leading/trailing underscores
    name = name.strip('_')
    # Ensure doesn't start with digit
    if name and name[0].isdigit():
        name = '_' + name
    return name or "unnamed"


def ghidra_type_to_splat(ghidra_type: str) -> Optional[str]:
    """Convert Ghidra type to splat-compatible type annotation."""
    if not ghidra_type:
        return None
    
    t = ghidra_type.lower()
    
    # Pointers
    if '*' in ghidra_type or 'pointer' in t:
        return 'u32'  # Pointers are 32-bit on PSX
    
    # Strings
    if 'string' in t or 'char[' in t:
        return 'asciz'
    
    # Sized types
    if t in ('byte', 'char', 'uchar', 'undefined1', 'uint8', 'int8'):
        return 'u8'
    if t in ('word', 'short', 'ushort', 'undefined2', 'uint16', 'int16'):
        return 'u16'
    if t in ('dword', 'int', 'uint', 'long', 'ulong', 'undefined4', 'uint32', 'int32'):
        return 'u32'
    if t in ('qword', 'undefined8', 'uint64', 'int64', 'longlong'):
        return 'u64'
    
    # Skip undefined/unknown
    if t.startswith('undefined'):
        return None
    
    return None


def export_symbols(
    project_path: str,
    project_name: str,
    program_name: str,
    addr_range: Tuple[int, int] = (0, 0xFFFFFFFF),
    include_functions: bool = True,
    include_data: bool = True,
    include_auto_names: bool = False,
    verbose: bool = False
) -> List[Dict[str, Any]]:
    """
    Export symbols from a Ghidra project using PyGhidra.
    
    Returns a list of symbol dictionaries sorted by address.
    """
    import pyghidra
    from pyghidra import HeadlessPyGhidraLauncher, ExtensionDetails
    
    if verbose:
        print("Starting PyGhidra...", file=sys.stderr)
    
    # Create launcher and install PSX extension if available
    launcher = HeadlessPyGhidraLauncher(verbose=verbose)
    
    # Check for PSX loader extension in common locations
    psx_ext_paths = [
        Path.home() / ".config/ghidra" / f"ghidra_{launcher.app_info.version}_NIX/Extensions/ghidra-psx-ldr",
        Path.home() / ".config/ghidra" / f"ghidra_{launcher.app_info.version}_PUBLIC/Extensions/ghidra-psx-ldr",
        Path("/nix/store") / "z088mpgaxbk78rvr4c011pbk2k50ag2p-ghidra-psx-ldr-2025.09.06/lib/ghidra/Ghidra/Extensions/ghidra-psx-ldr",
    ]
    
    for psx_path in psx_ext_paths:
        if psx_path.exists():
            if verbose:
                print(f"Installing PSX extension from: {psx_path}", file=sys.stderr)
            launcher.install_plugin(
                psx_path,
                ExtensionDetails(
                    name="ghidra-psx-ldr",
                    description="Sony PlayStation PSX executables loader for Ghidra",
                    author="lab313ru"
                )
            )
            break
    
    launcher.start()
    
    if verbose:
        print(f"Opening project: {project_path}/{project_name}", file=sys.stderr)
    
    symbols = []
    addr_min, addr_max = addr_range
    
    # PyGhidra's open_program works with existing Ghidra projects
    # nested_project_location=False for projects created by Ghidra GUI
    with pyghidra.open_program(
        binary_path=None,
        project_location=project_path,
        project_name=project_name,
        program_name=program_name,
        analyze=False,  # Don't re-analyze, use existing analysis
        nested_project_location=False  # GUI-created projects aren't nested
    ) as flat_api:
        program = flat_api.getCurrentProgram()
        
        if verbose:
            print(f"Opened program: {program.getName()}", file=sys.stderr)
        
        listing = program.getListing()
        function_manager = program.getFunctionManager()
        symbol_table = program.getSymbolTable()
        
        # Get decompiler interface for better type info
        from ghidra.app.decompiler import DecompInterface
        decomp = DecompInterface()
        decomp.openProgram(program)
        
        if include_functions:
            if verbose:
                print("Exporting functions...", file=sys.stderr)
            
            func_iter = function_manager.getFunctions(True)  # forward iteration
            func_count = 0
            
            while func_iter.hasNext():
                func = func_iter.next()
                addr = func.getEntryPoint().getOffset()
                
                # Filter by address range
                if not (addr_min <= addr <= addr_max):
                    continue
                
                name = func.getName()
                
                # Skip auto-generated names unless requested
                if not include_auto_names:
                    if name.startswith('FUN_') or name.startswith('func_'):
                        continue
                
                # Get function size from body
                body = func.getBody()
                size = body.getNumAddresses() if body else 0
                
                # Get signature
                signature = func.getSignature().getPrototypeString()
                
                # Get return type
                return_type = str(func.getReturnType())
                
                # Get calling convention
                calling_conv = func.getCallingConventionName()
                
                # Check if external/thunk
                is_external = func.isExternal()
                is_thunk = func.isThunk()
                
                # Get parameters
                params = []
                for param in func.getParameters():
                    params.append({
                        'name': param.getName(),
                        'type': str(param.getDataType()),
                        'storage': str(param.getVariableStorage())
                    })
                
                symbols.append({
                    'type': 'function',
                    'address': addr,
                    'name': name,
                    'size': size,
                    'signature': signature,
                    'return_type': return_type,
                    'calling_convention': calling_conv,
                    'parameters': params,
                    'is_external': is_external,
                    'is_thunk': is_thunk,
                    'rom_offset': vram_to_rom(addr)
                })
                func_count += 1
            
            if verbose:
                print(f"  Found {func_count} functions", file=sys.stderr)
        
        if include_data:
            if verbose:
                print("Exporting data symbols...", file=sys.stderr)
            
            # Only export data that has user-defined labels (not auto-generated)
            # This is much faster than iterating all defined data
            symbol_iter = symbol_table.getAllSymbols(True)  # forward iteration
            data_count = 0
            
            while symbol_iter.hasNext():
                symbol = symbol_iter.next()
                
                # Skip function symbols (already handled above)
                if symbol.getSymbolType().toString() == "Function":
                    continue
                
                addr = symbol.getAddress().getOffset()
                
                # Filter by address range
                if not (addr_min <= addr <= addr_max):
                    continue
                
                name = symbol.getName()
                
                # Skip auto-generated names unless requested
                if not include_auto_names:
                    if name.startswith('DAT_') or name.startswith('PTR_') or name.startswith('s_'):
                        continue
                
                # Get the data at this address if it exists
                data = listing.getDataAt(symbol.getAddress())
                if data:
                    data_type = str(data.getDataType())
                    size = data.getLength()
                else:
                    data_type = "undefined"
                    size = 0
                
                # Try to get value for simple types
                value = None
                try:
                    if data and data.hasStringValue():
                        value = data.getValue()
                    elif data and size <= 4:
                        val_obj = data.getValue()
                        if val_obj is not None:
                            value = str(val_obj)
                except:
                    pass
                
                symbols.append({
                    'type': 'data',
                    'address': addr,
                    'name': name,
                    'size': size,
                    'data_type': data_type,
                    'value': value,
                    'rom_offset': vram_to_rom(addr)
                })
                data_count += 1
            
            if verbose:
                print(f"  Found {data_count} data symbols", file=sys.stderr)
        
        decomp.dispose()
    
    # Sort by address
    symbols.sort(key=lambda x: x['address'])
    
    if verbose:
        print(f"\nTotal: {len(symbols)} symbols", file=sys.stderr)
    
    return symbols


def format_symbol_addrs(symbols: List[Dict], include_size: bool = True) -> str:
    """Format symbols as symbol_addrs.txt for splat."""
    lines = [
        "// Auto-generated from Ghidra via PyGhidra",
        f"// Total: {len(symbols)} symbols",
        ""
    ]
    
    for sym in symbols:
        addr = sym['address']
        name = sanitize_name(sym['name'])
        
        # Skip remaining auto-generated names
        if name.startswith('FUN_') or name.startswith('func_') or name.startswith('DAT_'):
            continue
        
        if sym['type'] == 'function':
            size = sym.get('size', 0)
            if include_size and size > 0:
                lines.append(f"{name} = 0x{addr:08X}; // size:0x{size:X}")
            else:
                lines.append(f"{name} = 0x{addr:08X};")
        else:
            splat_type = ghidra_type_to_splat(sym.get('data_type', ''))
            if splat_type:
                lines.append(f"{name} = 0x{addr:08X}; // type:{splat_type}")
            else:
                lines.append(f"{name} = 0x{addr:08X};")
    
    return '\n'.join(lines)


def format_yaml(symbols: List[Dict]) -> str:
    """Format symbols as YAML for splat config."""
    lines = [
        "# Auto-generated from Ghidra via PyGhidra",
        "# Copy relevant sections to your splat YAML config",
        "subsegments:"
    ]
    
    last_rom = None
    for sym in symbols:
        rom = sym['rom_offset']
        if rom < 0:
            continue
        
        # Skip duplicates at same address
        if rom == last_rom:
            lines.append(f"  # Duplicate: {sym['name']} @ 0x{sym['address']:08X}")
            continue
        
        name = sanitize_name(sym['name'])
        
        if sym['type'] == 'function':
            lines.append(f"  - [0x{rom:X}, asm, {name}]")
        else:
            lines.append(f"  - [0x{rom:X}, rodata]  # {name}")
        
        last_rom = rom
    
    return '\n'.join(lines)


def format_text(symbols: List[Dict]) -> str:
    """Format symbols as human-readable text."""
    lines = [
        f"# Ghidra symbols export (PyGhidra)",
        f"# Total: {len(symbols)} symbols",
        f"# {'='*78}",
        f"# {'Address':<12} {'ROM':<8} {'Size':<8} {'Type':<10} Name",
        f"# {'-'*78}"
    ]
    
    for sym in symbols:
        addr = sym['address']
        rom = sym['rom_offset']
        size = sym.get('size', 0)
        stype = sym['type']
        name = sym['name']
        
        rom_str = f"0x{rom:05X}" if rom >= 0 else "N/A"
        size_str = f"0x{size:X}" if size > 0 else "-"
        
        if stype == 'function':
            sig = sym.get('signature', '')
            lines.append(f"  0x{addr:08X}  {rom_str}  {size_str:<8} {stype:<10} {name}")
            if sig:
                lines.append(f"  {'':12} {'':8} {'':8} {'':10} -> {sig}")
        else:
            dtype = sym.get('data_type', '')
            lines.append(f"  0x{addr:08X}  {rom_str}  {size_str:<8} {stype:<10} {name} [{dtype}]")
    
    return '\n'.join(lines)


def format_json(symbols: List[Dict]) -> str:
    """Format symbols as JSON."""
    return json.dumps(symbols, indent=2)


def main():
    parser = argparse.ArgumentParser(
        description='Export symbols from Ghidra project using PyGhidra (headless)',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s --project ~/ghidra_projects --name skullmonkeys --program SLES_010.90
  %(prog)s --project ~/ghidra_projects --name skullmonkeys --program SLES_010.90 --format symbol_addrs
  %(prog)s --project ~/ghidra_projects --name skullmonkeys --program SLES_010.90 --range 0x80010000-0x80020000
        """
    )
    
    parser.add_argument('--project', '-p', required=True,
                        help='Path to Ghidra project directory (parent of .gpr file)')
    parser.add_argument('--name', '-n', required=True,
                        help='Ghidra project name')
    parser.add_argument('--program', '-P', required=True,
                        help='Program name within the project')
    parser.add_argument('--format', '-f', choices=['text', 'yaml', 'json', 'symbol_addrs'],
                        default='text', help='Output format (default: text)')
    parser.add_argument('--range', '-r', type=str, default=None,
                        help='Address range to export (e.g., 0x80010000-0x80020000)')
    parser.add_argument('--functions-only', action='store_true',
                        help='Export only functions')
    parser.add_argument('--data-only', action='store_true',
                        help='Export only data symbols')
    parser.add_argument('--include-auto', action='store_true',
                        help='Include auto-generated names (FUN_*, DAT_*)')
    parser.add_argument('--verbose', '-v', action='store_true',
                        help='Verbose output')
    parser.add_argument('--output', '-o', type=str, default=None,
                        help='Output file (default: stdout)')
    
    args = parser.parse_args()
    
    # Parse address range
    addr_range = (0, 0xFFFFFFFF)
    if args.range:
        addr_range = parse_address_range(args.range)
    
    # Determine what to export
    include_functions = not args.data_only
    include_data = not args.functions_only
    
    try:
        symbols = export_symbols(
            project_path=args.project,
            project_name=args.name,
            program_name=args.program,
            addr_range=addr_range,
            include_functions=include_functions,
            include_data=include_data,
            include_auto_names=args.include_auto,
            verbose=args.verbose
        )
    except ImportError as e:
        print(f"ERROR: PyGhidra not installed. Run: pip install pyghidra", file=sys.stderr)
        print(f"       Also ensure GHIDRA_INSTALL_DIR is set or Ghidra was recently run.", file=sys.stderr)
        sys.exit(1)
    except FileNotFoundError as e:
        print(f"ERROR: {e}", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"ERROR: {e}", file=sys.stderr)
        if args.verbose:
            import traceback
            traceback.print_exc()
        sys.exit(1)
    
    # Format output
    if args.format == 'text':
        output = format_text(symbols)
    elif args.format == 'yaml':
        output = format_yaml(symbols)
    elif args.format == 'json':
        output = format_json(symbols)
    elif args.format == 'symbol_addrs':
        output = format_symbol_addrs(symbols)
    
    # Write output
    if args.output:
        with open(args.output, 'w') as f:
            f.write(output)
        if args.verbose:
            print(f"Wrote output to: {args.output}", file=sys.stderr)
    else:
        print(output)


if __name__ == "__main__":
    main()
