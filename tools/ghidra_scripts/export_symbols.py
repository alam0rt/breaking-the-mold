#!/usr/bin/env python3
"""
export_symbols.py - Ghidra script to export all symbols with sizes

Run with PyGhidra:
    python3 -m pyghidra.run_script disks/pal/SLES_010.90 tools/ghidra_scripts/export_symbols.py

Or in Ghidra Script Manager (copy to ghidra_scripts folder, then run).

Outputs CSV format:
    address,rom_offset,type,name,size,end_address

This is MUCH faster than HTTP API since it directly accesses the program database.
"""

from ghidra.program.model.symbol import SymbolType
from ghidra.program.model.listing import Function
from ghidra.program.model.data import DataType

# PSX memory layout constants
RAM_BASE = 0x80010000
ROM_BASE = 0x800

def vram_to_rom(vram):
    """Convert VRAM address to ROM offset."""
    if vram >= RAM_BASE:
        return (vram - RAM_BASE) + ROM_BASE
    return -1

def get_all_symbols():
    """
    Get all defined symbols (functions + data) sorted by address.
    Returns list of dicts with: address, name, type, size, end_address
    """
    symbols = []
    
    # Get all functions
    func_mgr = currentProgram.getFunctionManager()
    for func in func_mgr.getFunctions(True):  # True = forward iteration
        addr = func.getEntryPoint().getOffset()
        name = func.getName()
        body = func.getBody()
        size = body.getNumAddresses() if body else 0
        end_addr = addr + size if size > 0 else addr
        
        symbols.append({
            'address': addr,
            'name': name,
            'type': 'function',
            'size': size,
            'end_address': end_addr,
            'data_type': '',
        })
    
    # Get all defined data
    listing = currentProgram.getListing()
    data_iter = listing.getDefinedData(True)  # True = forward iteration
    
    for data in data_iter:
        addr = data.getAddress().getOffset()
        
        # Get label/name
        symbol = data.getPrimarySymbol()
        if symbol:
            name = symbol.getName()
        else:
            name = "DAT_{:08X}".format(addr)
        
        # Get data type and size
        dt = data.getDataType()
        data_type_name = dt.getName() if dt else "undefined"
        size = data.getLength()
        end_addr = addr + size
        
        symbols.append({
            'address': addr,
            'name': name,
            'type': 'data',
            'size': size,
            'end_address': end_addr,
            'data_type': data_type_name,
        })
    
    # Sort by address
    symbols.sort(key=lambda x: x['address'])
    
    return symbols

def find_gaps(symbols, min_addr=0x80010000, max_addr=0x800A0000):
    """
    Find gaps between symbols (undefined regions).
    Returns list of (start_addr, end_addr, size) tuples.
    """
    gaps = []
    
    # Filter to address range
    filtered = [s for s in symbols if min_addr <= s['address'] < max_addr]
    if not filtered:
        return gaps
    
    # Check gap at start
    first_addr = filtered[0]['address']
    if first_addr > min_addr:
        gaps.append((min_addr, first_addr, first_addr - min_addr))
    
    # Check gaps between symbols
    for i in range(len(filtered) - 1):
        curr_end = filtered[i]['end_address']
        next_start = filtered[i + 1]['address']
        
        if next_start > curr_end:
            gap_size = next_start - curr_end
            if gap_size > 0:
                gaps.append((curr_end, next_start, gap_size))
    
    return gaps

def analyze_region(start_addr, size):
    """
    Analyze a memory region to determine its likely type.
    Returns: 'code', 'pointers', 'strings', 'zeros', 'data'
    """
    memory = currentProgram.getMemory()
    addr = currentProgram.getAddressFactory().getAddress("0x{:08X}".format(start_addr))
    
    try:
        data = bytearray(size if size <= 256 else 256)
        memory.getBytes(addr, data)
    except:
        return 'unknown'
    
    # Count patterns
    zero_count = sum(1 for b in data if b == 0)
    
    # Count potential pointers (0x80xxxxxx)
    pointer_count = 0
    for i in range(0, len(data) - 3, 4):
        word = data[i] | (data[i+1] << 8) | (data[i+2] << 16) | (data[i+3] << 24)
        if 0x80010000 <= word <= 0x80200000:
            pointer_count += 1
    
    # Check for ASCII strings
    ascii_count = sum(1 for b in data if 0x20 <= b <= 0x7E)
    
    total_words = len(data) // 4
    
    if zero_count > len(data) * 0.9:
        return 'zeros/bss'
    elif pointer_count > total_words * 0.3:
        return 'pointers/jumptable'
    elif ascii_count > len(data) * 0.5:
        return 'strings'
    else:
        return 'data'

def main():
    symbols = get_all_symbols()
    
    # Print header
    print("# Ghidra Symbol Export")
    print("# Program: {}".format(currentProgram.getName()))
    print("# Total symbols: {}".format(len(symbols)))
    print("#")
    print("# address,rom_offset,type,name,size,end_address,data_type")
    
    # Filter to PSX RAM range
    psx_symbols = [s for s in symbols if 0x80000000 <= s['address'] < 0x80200000]
    
    print("# PSX RAM symbols: {}".format(len(psx_symbols)))
    print("#")
    
    # Print CSV
    for sym in psx_symbols:
        addr = sym['address']
        rom = vram_to_rom(addr)
        rom_str = "0x{:05X}".format(rom) if rom >= 0 else "N/A"
        
        print("0x{:08X},{},{},{},{},0x{:08X},{}".format(
            addr,
            rom_str,
            sym['type'],
            sym['name'],
            sym['size'],
            sym['end_address'],
            sym['data_type']
        ))
    
    # Find and report gaps
    print("#")
    print("# === GAPS (undefined regions) ===")
    gaps = find_gaps(psx_symbols)
    
    for start, end, size in gaps:
        if size >= 16:  # Only report gaps >= 16 bytes
            rom_start = vram_to_rom(start)
            region_type = analyze_region(start, size)
            print("# GAP: 0x{:08X}-0x{:08X} (ROM 0x{:05X}, size 0x{:X}) - likely {}".format(
                start, end, rom_start, size, region_type))

# Ghidra script entry point
if __name__ == "__main__":
    main()
else:
    # Running inside Ghidra
    main()
