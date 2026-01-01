#!/usr/bin/env bash
# Find which GCC version best matches the original binary
# Usage: ./scripts/find_matching_gcc.sh <source_file.c> <function_name> <expected_size_hex>
# Example: ./scripts/find_matching_gcc.sh src/LevelDataParser.c func_8007A62C 0x254

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
GCC_BUILDS="$PROJECT_ROOT/tools/gcc-builds"
OUTPUT_DIR="$PROJECT_ROOT/build/gcc-match"
MASPSX="$PROJECT_ROOT/tools/maspsx/maspsx.py"
ASSEMBLER="mipsel-unknown-linux-gnu-as"
OBJCOPY="mipsel-unknown-linux-gnu-objcopy"

SOURCE_FILE="$1"
FUNCTION_NAME="$2"
EXPECTED_SIZE="${3:-}"

if [ -z "$SOURCE_FILE" ] || [ -z "$FUNCTION_NAME" ]; then
    echo "Usage: $0 <source_file.c> <function_name> [expected_size_hex]"
    echo ""
    echo "Compiles a source file with all GCC versions and compares the binary output"
    echo "to find which version best matches the original game."
    echo ""
    echo "Example: $0 src/LevelDataParser.c func_8007A62C 0x254"
    exit 1
fi

if [ ! -f "$SOURCE_FILE" ]; then
    echo "Error: Source file not found: $SOURCE_FILE"
    exit 1
fi

mkdir -p "$OUTPUT_DIR"

# Compiler flags (matching project Makefile)
CFLAGS="-O2 -G8 -mno-abicalls -mcpu=3000 -quiet"
INCLUDES="-I $PROJECT_ROOT/include -I $PROJECT_ROOT/psyq"

# Get base name of source file
BASENAME=$(basename "$SOURCE_FILE" .c)

echo "Finding matching GCC version for: $FUNCTION_NAME in $SOURCE_FILE"
if [ -n "$EXPECTED_SIZE" ]; then
    echo "Expected size: $EXPECTED_SIZE"
fi
echo ""

# Preprocess the file once
PREPROCESSED_FILE="$OUTPUT_DIR/${BASENAME}.i"
cpp -D_LANGUAGE_C -DNON_MATCHING $INCLUDES "$SOURCE_FILE" 2>/dev/null | grep -v '^# ' > "$PREPROCESSED_FILE"

echo "=== Compiling with each GCC version ==="
echo ""

# Store results
declare -A SIZES
declare -A MATCHES

for gcc_dir in "$GCC_BUILDS"/gcc-*-psx; do
    if [ ! -d "$gcc_dir" ]; then
        continue
    fi
    
    VERSION=$(basename "$gcc_dir" | sed 's/gcc-//')
    CC1="$gcc_dir/cc1"
    
    if [ ! -f "$CC1" ]; then
        continue
    fi
    
    echo -n "[$VERSION] "
    
    OUTPUT_S="$OUTPUT_DIR/${BASENAME}_${VERSION}.s"
    OUTPUT_O="$OUTPUT_DIR/${BASENAME}_${VERSION}.o"
    OUTPUT_BIN="$OUTPUT_DIR/${BASENAME}_${VERSION}.bin"
    
    # Compile
    if ! "$CC1" $CFLAGS "$PREPROCESSED_FILE" -o "$OUTPUT_S" 2>/dev/null; then
        echo "compile failed"
        continue
    fi
    
    # Assemble with maspsx
    if ! python3 "$MASPSX" --aspsx-version=2.86 --run-assembler \
        --gnu-as-path="$ASSEMBLER" \
        -o "$OUTPUT_O" \
        -march=r3000 -mtune=r3000 -no-pad-sections -G8 \
        -I"$PROJECT_ROOT/include" \
        "$OUTPUT_S" 2>/dev/null; then
        echo "assemble failed"
        continue
    fi
    
    # Extract just the .text section
    if ! $OBJCOPY -O binary -j .text "$OUTPUT_O" "$OUTPUT_BIN" 2>/dev/null; then
        echo "objcopy failed"
        continue
    fi
    
    # Get size
    SIZE=$(stat -c%s "$OUTPUT_BIN" 2>/dev/null || stat -f%z "$OUTPUT_BIN" 2>/dev/null)
    SIZE_HEX=$(printf "0x%X" "$SIZE")
    SIZES[$VERSION]="$SIZE_HEX"
    
    # Check if size matches expected
    if [ -n "$EXPECTED_SIZE" ]; then
        EXPECTED_DEC=$((EXPECTED_SIZE))
        if [ "$SIZE" -eq "$EXPECTED_DEC" ]; then
            echo "Size: $SIZE_HEX *** EXACT MATCH ***"
            MATCHES[$VERSION]="exact"
        elif [ "$SIZE" -lt "$EXPECTED_DEC" ]; then
            DIFF=$((EXPECTED_DEC - SIZE))
            echo "Size: $SIZE_HEX (-$DIFF bytes)"
            MATCHES[$VERSION]="smaller"
        else
            DIFF=$((SIZE - EXPECTED_DEC))
            echo "Size: $SIZE_HEX (+$DIFF bytes)"
            MATCHES[$VERSION]="larger"
        fi
    else
        echo "Size: $SIZE_HEX"
    fi
done

echo ""
echo "=== Summary ==="
for VERSION in $(echo "${!SIZES[@]}" | tr ' ' '\n' | sort -V); do
    STATUS="${MATCHES[$VERSION]:-}"
    if [ "$STATUS" = "exact" ]; then
        echo "  $VERSION: ${SIZES[$VERSION]} *** MATCH ***"
    else
        echo "  $VERSION: ${SIZES[$VERSION]}"
    fi
done

# If we found exact matches, compare them byte-by-byte with the original
echo ""
echo "=== Exact size matches ==="
for VERSION in "${!MATCHES[@]}"; do
    if [ "${MATCHES[$VERSION]}" = "exact" ]; then
        OUTPUT_BIN="$OUTPUT_DIR/${BASENAME}_${VERSION}.bin"
        echo ""
        echo "[$VERSION] Hex dump of compiled function:"
        od -A x -t x1 "$OUTPUT_BIN" | head -20
    fi
done

# Clean up
rm -f "$PREPROCESSED_FILE"

echo ""
echo "Output files in: $OUTPUT_DIR"
