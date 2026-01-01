#!/usr/bin/env python3
"""
generate_splat_config.py - PyGhidra script to generate splat segment configuration

Run with PyGhidra:
    python3 -m pyghidra.run_script /path/to/program.bin generate_splat_config.py

Or in Ghidra Script Manager (Jython compatible).

Generates a splat-compatible YAML segments configuration based on:
- Memory blocks/sections from the loaded program
- Detected section types (.text, .data, .rodata, .bss, etc.)
- File offsets and VRAM addresses
"""

from ghidra.program.model.mem import MemoryBlock
from ghidra.program.model.symbol import SymbolType
import re

def get_memory_blocks():
    """Get all memory blocks sorted by start address."""
    memory = currentProgram.getMemory()
    blocks = list(memory.getBlocks())
    blocks.sort(key=lambda b: b.getStart().getOffset())
    return blocks

def classify_section(block):
    """Classify a memory block into splat segment types."""
    name = block.getName().lower()
    is_initialized = block.isInitialized()
    is_executable = block.isExecute()
    is_writable = block.isWrite()
    is_readable = block.isRead()
    
    # Check for explicit section names
    if 'text' in name or name == '.text':
        return 'asm'  # or 'c' if you have source
    elif 'rodata' in name or 'rdata' in name:
        return 'rodata'
    elif 'sdata' in name:
        return 'data'  # GP-relative data, treat as data
    elif 'sbss' in name:
        return 'sbss'
    elif 'bss' in name:
        return 'bss'
    elif 'data' in name:
        return 'data'
    
    # Infer from permissions
    if not is_initialized:
        return 'bss'
    elif is_executable:
        return 'asm'
    elif not is_writable and is_readable:
        return 'rodata'
    else:
        return 'data'

def find_gp_value():
    """Try to find the GP register value from symbols or common patterns."""
    symbol_table = currentProgram.getSymbolTable()
    
    # Look for common GP-related symbols
    gp_symbols = ['_gp', '__gp', '_GP', 'gp']
    for sym_name in gp_symbols:
        symbols = list(symbol_table.getSymbols(sym_name))
        if symbols:
            return symbols[0].getAddress().getOffset()
    
    # Look for _SDA_BASE_ (some toolchains)
    sda_symbols = list(symbol_table.getSymbols('_SDA_BASE_'))
    if sda_symbols:
        return sda_symbols[0].getAddress().getOffset()
    
    return None

def calculate_file_offset(vram, base_vram=0x80010000, file_base=0x800):
    """Calculate file offset from VRAM address."""
    # PSX-EXE typically: file_offset = (vram - base_vram) + header_size
    return (vram - base_vram) + file_base

def find_bss_size(blocks):
    """Calculate combined BSS size from uninitialized sections."""
    bss_size = 0
    bss_start = None
    bss_end = None
    
    for block in blocks:
        name = block.getName().lower()
        if 'bss' in name or not block.isInitialized():
            start = block.getStart().getOffset()
            end = block.getEnd().getOffset() + 1
            
            if bss_start is None or start < bss_start:
                bss_start = start
            if bss_end is None or end > bss_end:
                bss_end = end
    
    if bss_start and bss_end:
        bss_size = bss_end - bss_start
    
    return bss_size, bss_start, bss_end

def get_program_info():
    """Get basic program metadata."""
    info = {}
    info['name'] = currentProgram.getName()
    info['image_base'] = currentProgram.getImageBase().getOffset()
    info['language'] = str(currentProgram.getLanguageID())
    info['compiler'] = str(currentProgram.getCompilerSpec().getCompilerSpecID())
    
    # Get executable format info
    exe_format = currentProgram.getExecutableFormat()
    info['format'] = exe_format if exe_format else "Unknown"
    
    return info

def detect_jumptables(block):
    """Detect if a rodata block contains jump tables (for split hints)."""
    # This is a simplified check - looks for sequences of addresses
    if not block.isInitialized():
        return []
    
    start = block.getStart()
    end = block.getEnd()
    
    # Check references to find potential jump table boundaries
    ref_mgr = currentProgram.getReferenceManager()
    
    splits = []
    # This would require more sophisticated analysis
    # For now, return empty - splat's own detection is usually better
    return splits

def generate_splat_yaml():
    """Generate the complete splat YAML configuration."""
    blocks = get_memory_blocks()
    prog_info = get_program_info()
    gp_value = find_gp_value()
    bss_size, bss_start, bss_end = find_bss_size(blocks)
    
    # Find the first code/data block to determine base addresses
    base_vram = None
    for block in blocks:
        if block.isInitialized():
            base_vram = block.getStart().getOffset()
            break
    
    if base_vram is None:
        base_vram = 0x80010000  # PSX default
    
    # PSX-EXE header is typically 0x800 bytes
    header_size = 0x800
    
    print("# ============================================")
    print("# Splat Configuration Generated by PyGhidra")
    print("# ============================================")
    print("")
    print("# Program: {}".format(prog_info['name']))
    print("# Format:  {}".format(prog_info['format']))
    print("# Language: {}".format(prog_info['language']))
    print("")
    print("name: {}".format(prog_info['name'].replace('.', '_')))
    print("# sha1: <calculate with sha1sum>")
    print("options:")
    print("  basename: game")
    print("  target_path: <path/to/binary>")
    print("  base_path: ..")
    print("  platform: psx")
    print("  compiler: PSYQ")
    print("")
    print("  asm_path: asm")
    print("  src_path: src")
    print("  build_path: build")
    print("")
    
    if gp_value:
        print("  gp_value: 0x{:08X}".format(gp_value))
    else:
        print("  # gp_value: <set from _gp symbol or .sdata start + 0x7FF0>")
    
    print("")
    print("  section_order: [\".rodata\", \".text\", \".data\", \".sdata\", \".sbss\", \".bss\"]")
    print("")
    print("  symbol_addrs_path:")
    print("    - symbol_addrs.txt")
    print("")
    
    print("segments:")
    print("  - name: header")
    print("    type: header")
    print("    start: 0x0")
    print("")
    
    # Group blocks into initialized and uninitialized
    init_blocks = [b for b in blocks if b.isInitialized()]
    uninit_blocks = [b for b in blocks if not b.isInitialized()]
    
    # Find file end from last initialized block
    file_end = 0
    for block in init_blocks:
        block_vram = block.getStart().getOffset()
        block_size = block.getSize()
        block_file_offset = calculate_file_offset(block_vram, base_vram, header_size)
        block_file_end = block_file_offset + block_size
        if block_file_end > file_end:
            file_end = block_file_end
    
    print("  - name: main")
    print("    type: code")
    print("    start: 0x{:X}".format(header_size))
    print("    vram: 0x{:08X}".format(base_vram))
    if bss_size > 0:
        print("    bss_size: 0x{:X}  # 0x{:08X} - 0x{:08X}".format(
            bss_size, bss_start, bss_end))
    print("    subsegments:")
    
    # Generate subsegments for initialized blocks
    for block in init_blocks:
        block_name = block.getName()
        block_vram = block.getStart().getOffset()
        block_size = block.getSize()
        block_end = block.getEnd().getOffset()
        block_type = classify_section(block)
        
        file_offset = calculate_file_offset(block_vram, base_vram, header_size)
        
        # Add comment with section info
        print("      # {}: 0x{:08X} - 0x{:08X} (size: 0x{:X})".format(
            block_name, block_vram, block_end, block_size))
        
        # Handle special section names
        if 'sdata' in block_name.lower():
            print("      - [0x{:X}, data, sdata]".format(file_offset))
        else:
            print("      - [0x{:X}, {}]".format(file_offset, block_type))
    
    # Add BSS comments
    if uninit_blocks:
        print("      # --- Uninitialized sections (handled by bss_size) ---")
        for block in uninit_blocks:
            block_name = block.getName()
            block_vram = block.getStart().getOffset()
            block_end = block.getEnd().getOffset()
            block_size = block.getSize()
            print("      # {}: 0x{:08X} - 0x{:08X} (size: 0x{:X})".format(
                block_name, block_vram, block_end, block_size))
    
    # End marker
    print("")
    print("  - [0x{:X}]  # End of file".format(file_end))
    
    print("")
    print("# ============================================")
    print("# Section Summary")
    print("# ============================================")
    print("#")
    total_init = sum(b.getSize() for b in init_blocks)
    total_uninit = sum(b.getSize() for b in uninit_blocks)
    print("# Initialized sections:   0x{:X} ({} bytes)".format(total_init, total_init))
    print("# Uninitialized sections: 0x{:X} ({} bytes)".format(total_uninit, total_uninit))
    print("# Total memory footprint: 0x{:X} ({} bytes)".format(
        total_init + total_uninit, total_init + total_uninit))
    if gp_value:
        print("# GP register value:      0x{:08X}".format(gp_value))
    print("#")
    
    # List all symbols that might be useful
    print("# ============================================")
    print("# Notable Symbols")
    print("# ============================================")
    print("#")
    
    symbol_table = currentProgram.getSymbolTable()
    notable_patterns = ['main', 'start', '_start', '__start', 'init', '_init',
                       '_gp', 'gp', '_bss', '_sbss', '_data', '_sdata', '_text',
                       '_end', '_edata', '_etext']
    
    found_symbols = []
    for pattern in notable_patterns:
        for sym in symbol_table.getSymbols(pattern):
            found_symbols.append((sym.getName(), sym.getAddress().getOffset()))
    
    # Also look for function entry points
    func_mgr = currentProgram.getFunctionManager()
    entry_funcs = []
    for func in func_mgr.getFunctions(True):
        name = func.getName()
        if name.lower() in ['main', 'start', '_start', '__start', 'maincrtroutine']:
            entry_funcs.append((name, func.getEntryPoint().getOffset()))
    
    for name, addr in sorted(set(found_symbols + entry_funcs), key=lambda x: x[1]):
        print("# {} = 0x{:08X}".format(name, addr))
    
    print("#")

def main():
    print("")
    print("=" * 60)
    print("  Splat Configuration Generator")
    print("  Analyzing: {}".format(currentProgram.getName()))
    print("=" * 60)
    print("")
    
    generate_splat_yaml()
    
    print("")
    print("=" * 60)
    print("  Generation Complete!")
    print("  Copy the YAML output above to your splat config file.")
    print("=" * 60)

# Run the script
if __name__ == "__main__":
    main()
else:
    # Running in Ghidra's script manager
    main()
