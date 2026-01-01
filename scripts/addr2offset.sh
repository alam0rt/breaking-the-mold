#!/usr/bin/env zsh
# Convert a VRAM address to file offset for SLES_010.90
# Usage: ./scripts/addr2offset.sh 800260d0

if [ -z "$1" ]; then
    echo "Usage: $0 <vram_address>"
    echo "Example: $0 800260d0"
    exit 1
fi

# Remove 0x prefix if present
ADDR="${1#0x}"

# Convert to decimal
VRAM=$((16#$ADDR))

# Base VRAM and file offset
BASE_VRAM=$((0x80010000))
BASE_OFFSET=$((0x800))

# Calculate file offset
OFFSET=$((VRAM - BASE_VRAM + BASE_OFFSET))

# Output
printf "VRAM:   0x%08X\n" $VRAM
printf "Offset: 0x%X\n" $OFFSET
