# GDB connect script for poking around Skullmonkeys (SLES-01090) live in
# PCSX-Redux. Pairs with `make emu-exe` (side-loads build/SLES_010.90.elf as the
# running executable while the disc supplies assets).
#
# Usage:
#   1. terminal A:  make emu-exe          # or: make DEBUG=1 emu-exe  (source lines)
#   2. terminal B:  gdb -x tools/skullmonkeys.gdb
#
# The ELF is loaded ONLY for its symbols/DWARF -- the code is already running in
# the emulator, so we never `run`/`load`, just attach to PCSX's GDB stub. Because
# this is a byte-match decomp, the symbol/line addresses map exactly onto the
# code executing in RAM.

set pagination off
set print pretty on
# PSX is little-endian MIPS R3000; GDB autodetects from the ELF, this is belt-and-braces.
set endian little

# Symbols (and DWARF line info, if built with DEBUG=1) for the running image.
file build/SLES_010.90.elf

# PCSX-Redux GDB server (see GDB_PORT in the Makefile).
target remote localhost:3333

echo \n=== Skullmonkeys debug session ===\n
echo Attached to PCSX-Redux. The ELF's symbols are loaded.\n
echo Try:  break PlayerTickCallback / break main / bt / info registers / x/8xw $sp\n
echo If built with `make DEBUG=1 emu-exe`, `list` and `step` show C source.\n\n
