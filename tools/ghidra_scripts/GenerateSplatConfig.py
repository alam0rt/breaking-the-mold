# Generate splat segment configuration from Ghidra
# @category Analysis
# @menupath Tools.Generate Splat Config
# @description Generates splat YAML segment configuration from memory map

"""
Jython script for Ghidra (standard installation without PyGhidra).
Run via Script Manager or Tools menu.
Output goes to Console window.
"""

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
        return 'asm'
    elif 'rodata' in name or 'rdata' in name:
        return 'rodata'
    elif 'sdata' in name:
        return 'data'
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
    """Try to find the GP register value from symbols."""
    symbol_table = currentProgram.getSymbolTable()
    
    gp_symbols = ['_gp', '__gp', '_GP', 'gp', '_SDA_BASE_']
    for sym_name in gp_symbols:
        symbols = list(symbol_table.getSymbols(sym_name))
        if symbols:
            return symbols[0].getAddress().getOffset()
    return None

def calculate_file_offset(vram, base_vram, file_base):
    """Calculate file offset from VRAM address."""
    return (vram - base_vram) + file_base

def find_bss_info(blocks):
    """Calculate BSS size from uninitialized sections."""
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
        return (bss_end - bss_start, bss_start, bss_end)
    return (0, None, None)

def format_hex(val):
    """Format value as hex string."""
    return "0x%08X" % val

def format_hex_short(val):
    """Format value as short hex string."""
    return "0x%X" % val

def main():
    blocks = get_memory_blocks()
    gp_value = find_gp_value()
    bss_size, bss_start, bss_end = find_bss_info(blocks)
    
    # Find base VRAM from first initialized block
    base_vram = 0x80010000  # PSX default
    for block in blocks:
        if block.isInitialized():
            base_vram = block.getStart().getOffset()
            break
    
    header_size = 0x800  # PSX-EXE header
    
    # Separate initialized and uninitialized
    init_blocks = [b for b in blocks if b.isInitialized()]
    uninit_blocks = [b for b in blocks if not b.isInitialized()]
    
    # Find file end
    file_end = 0
    for block in init_blocks:
        block_vram = block.getStart().getOffset()
        block_size = block.getSize()
        block_file_end = calculate_file_offset(block_vram, base_vram, header_size) + block_size
        if block_file_end > file_end:
            file_end = block_file_end
    
    # Output YAML
    println("=" * 60)
    println("Splat Configuration - Copy below this line")
    println("=" * 60)
    println("")
    println("name: %s" % currentProgram.getName().replace('.', '_'))
    println("# sha1: <run sha1sum on binary>")
    println("options:")
    println("  basename: game")
    println("  target_path: <path/to/binary>")
    println("  base_path: ..")
    println("  platform: psx")
    println("  compiler: PSYQ")
    println("")
    println("  asm_path: asm")
    println("  src_path: src")
    println("  build_path: build")
    println("")
    
    if gp_value:
        println("  gp_value: %s" % format_hex(gp_value))
    else:
        println("  # gp_value: <set from _gp symbol>")
    
    println("")
    println("  section_order: [\".rodata\", \".text\", \".data\", \".sdata\", \".sbss\", \".bss\"]")
    println("")
    println("  symbol_addrs_path:")
    println("    - symbol_addrs.txt")
    println("")
    println("segments:")
    println("  - name: header")
    println("    type: header")
    println("    start: 0x0")
    println("")
    println("  - name: main")
    println("    type: code")
    println("    start: %s" % format_hex_short(header_size))
    println("    vram: %s" % format_hex(base_vram))
    
    if bss_size > 0:
        println("    bss_size: %s  # %s - %s" % (
            format_hex_short(bss_size), format_hex(bss_start), format_hex(bss_end)))
    
    println("    subsegments:")
    
    # Subsegments for initialized blocks
    for block in init_blocks:
        block_name = block.getName()
        block_vram = block.getStart().getOffset()
        block_size = block.getSize()
        block_end = block.getEnd().getOffset()
        block_type = classify_section(block)
        file_offset = calculate_file_offset(block_vram, base_vram, header_size)
        
        println("      # %s: %s - %s (size: %s)" % (
            block_name, format_hex(block_vram), format_hex(block_end), format_hex_short(block_size)))
        
        if 'sdata' in block_name.lower():
            println("      - [%s, data, sdata]" % format_hex_short(file_offset))
        else:
            println("      - [%s, %s]" % (format_hex_short(file_offset), block_type))
    
    # BSS comments
    if uninit_blocks:
        println("      # --- Uninitialized (handled by bss_size) ---")
        for block in uninit_blocks:
            println("      # %s: %s - %s (size: %s)" % (
                block.getName(),
                format_hex(block.getStart().getOffset()),
                format_hex(block.getEnd().getOffset()),
                format_hex_short(block.getSize())))
    
    println("")
    println("  - [%s]  # End of file" % format_hex_short(file_end))
    println("")
    println("=" * 60)
    println("Summary")
    println("=" * 60)
    
    total_init = sum(b.getSize() for b in init_blocks)
    total_uninit = sum(b.getSize() for b in uninit_blocks)
    
    println("Initialized:   %s (%d bytes)" % (format_hex_short(total_init), total_init))
    println("Uninitialized: %s (%d bytes)" % (format_hex_short(total_uninit), total_uninit))
    println("Total memory:  %s (%d bytes)" % (format_hex_short(total_init + total_uninit), total_init + total_uninit))
    if gp_value:
        println("GP value:      %s" % format_hex(gp_value))
    
    # Notable symbols
    println("")
    println("Notable Symbols:")
    symbol_table = currentProgram.getSymbolTable()
    patterns = ['main', 'start', '_start', '__start', '_gp', '_bss', '_end']
    
    for pattern in patterns:
        for sym in symbol_table.getSymbols(pattern):
            println("  %s = %s" % (sym.getName(), format_hex(sym.getAddress().getOffset())))
    
    # Entry functions
    func_mgr = currentProgram.getFunctionManager()
    for func in func_mgr.getFunctions(True):
        name = func.getName().lower()
        if name in ['main', 'start', '_start']:
            println("  %s() = %s" % (func.getName(), format_hex(func.getEntryPoint().getOffset())))

# Run
main()
