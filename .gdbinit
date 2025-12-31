# PSX GDB Configuration for PCSX-Redux debugging
# This file is auto-loaded when gdb is started from the project root

# MIPS R3000 architecture (PlayStation 1 CPU)
set architecture mips:3000
set endian little

# Disable pagination for scripted use
set pagination off

# Load Ghidra debugger compatibility script (fakes /proc/pid/maps for emulated target)
source tools/ghidra_debugger_scripts

# Connect to PCSX-Redux GDB server
target remote localhost:3333

# Load symbols from built ELF (uncomment if desired)
# symbol-file build/pal/game.elf

# PSX Memory Map Reference:
# RAM:        0x80000000 - 0x801FFFFF (2MB, mirrored at 0x00000000 and 0xA0000000)
# Scratchpad: 0x1F800000 - 0x1F8003FF (1KB)
# Hardware:   0x1F801000 - 0x1F802FFF
# BIOS:       0xBFC00000 - 0xBFC7FFFF (512KB)

# Convenience commands
define psxpc
  x/10i $pc
end
document psxpc
Disassemble 10 instructions at current PC
end

define psxregs
  info registers
end
document psxregs
Display all MIPS registers
end

define psxstack
  x/20xw $sp
end
document psxstack
Display 20 words from stack pointer
end
