#!/usr/bin/env bash
# Compare all C files against all GCC versions to find best matches
# This helps identify which compiler version was used for each file

set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$PROJECT_ROOT"

# Source files to test (excluding those with only INCLUDE_ASM)
SRC_DIR="src"
BUILD_DIR="build/pal"
ORIGINAL="disks/pal/SLES_010.90"
GCC_BUILDS="tools/gcc-builds"

# Compiler flags matching Makefile
CFLAGS="-O2 -G8 -fno-builtin -mno-abicalls -mcpu=3000"

# GCC versions to test
GCC_VERSIONS=(
    "gcc-2.5.7-psx"
    "gcc-2.6.0-psx"
    "gcc-2.6.3-psx"
    "gcc-2.7.2-psx"
    "gcc-2.8.0-psx"
    "gcc-2.8.1-psx"
    "gcc-2.91.66-psx"
    "gcc-2.95.2-psx"
)

# Function info: name, address, size
# Format: "filename:funcname:addr:size"
declare -A FUNC_INFO
FUNC_INFO["initPlayerState"]="initPlayerState.c:initPlayerState:0x800260D0:0x8C"
FUNC_INFO["getLevelName"]="getLevelName.c:getLevelName:0x8002615C:0x14"

# Read symbol_addrs.txt to build function database
echo "==================================================================="
echo "Comparing C files against all GCC versions"
echo "==================================================================="
echo ""

# Get list of C files
C_FILES=$(find "$SRC_DIR" -name "*.c" -type f | sort)

# Create temp directory
TEMP_DIR=$(mktemp -d)
trap "rm -rf $TEMP_DIR" EXIT

# Function to extract function bytes from original binary
get_original_bytes() {
    local file_offset=$1
    local size=$2
    od -A n -t x1 -v -j "$file_offset" -N "$size" "$ORIGINAL" 2>/dev/null | tr -d ' \n'
}

# Function to compile a C file with a specific GCC version
compile_file() {
    local src_file=$1
    local gcc_version=$2
    local output_file=$3
    
    local cc1="$GCC_BUILDS/$gcc_version/cc1"
    local temp_s="$TEMP_DIR/temp.s"
    
    # Preprocess and compile
    cpp -I include -I psyq -D_LANGUAGE_C -DVERSION_pal "$src_file" 2>/dev/null | \
        "$cc1" $CFLAGS -o "$temp_s" 2>/dev/null
    
    if [ $? -ne 0 ]; then
        return 1
    fi
    
    # Assemble with maspsx
    python3 tools/maspsx/maspsx.py --aspsx-version=2.86 --run-assembler \
        --gnu-as-path=mipsel-unknown-linux-gnu-as \
        -o "$output_file" -march=r3000 -mtune=r3000 -no-pad-sections -G8 -Iinclude \
        "$temp_s" 2>/dev/null
    
    return $?
}

# Function to get .text section bytes from object file
get_obj_bytes() {
    local obj_file=$1
    mipsel-unknown-linux-gnu-objcopy -O binary -j .text "$obj_file" "$TEMP_DIR/text.bin" 2>/dev/null
    if [ -f "$TEMP_DIR/text.bin" ]; then
        od -A n -t x1 -v "$TEMP_DIR/text.bin" 2>/dev/null | tr -d ' \n'
        rm -f "$TEMP_DIR/text.bin"
    fi
}

# Function to count matching bytes
count_matching_bytes() {
    local bytes1=$1
    local bytes2=$2
    local len1=${#bytes1}
    local len2=${#bytes2}
    local min_len=$((len1 < len2 ? len1 : len2))
    local matches=0
    
    for ((i=0; i<min_len; i+=2)); do
        if [ "${bytes1:i:2}" = "${bytes2:i:2}" ]; then
            ((matches++))
        fi
    done
    
    echo $matches
}

# Process each C file
echo "Processing C files..."
echo ""

for src_file in $C_FILES; do
    filename=$(basename "$src_file")
    
    # Skip files that are mostly INCLUDE_ASM
    if grep -q "INCLUDE_ASM" "$src_file" && ! grep -q "#if 0" "$src_file"; then
        # Check if there's actual C code or just INCLUDE_ASM
        c_funcs=$(grep -c "^[a-zA-Z].*(" "$src_file" 2>/dev/null || echo 0)
        asm_refs=$(grep -c "INCLUDE_ASM" "$src_file" 2>/dev/null || echo 0)
        if [ "$c_funcs" -le "$asm_refs" ]; then
            echo "[$filename] Skipping - uses INCLUDE_ASM"
            continue
        fi
    fi
    
    echo "==================================================================="
    echo "File: $filename"
    echo "==================================================================="
    
    # Try each compiler version
    declare -A results
    for gcc_ver in "${GCC_VERSIONS[@]}"; do
        obj_file="$TEMP_DIR/${filename%.c}.o"
        
        if compile_file "$src_file" "$gcc_ver" "$obj_file" 2>/dev/null; then
            # Get size of .text section
            text_size=$(mipsel-unknown-linux-gnu-objdump -h "$obj_file" 2>/dev/null | \
                grep "\.text" | awk '{print $3}')
            if [ -n "$text_size" ]; then
                text_size_dec=$((16#$text_size))
                results[$gcc_ver]="$text_size_dec"
            else
                results[$gcc_ver]="0"
            fi
        else
            results[$gcc_ver]="FAIL"
        fi
        rm -f "$obj_file"
    done
    
    # Print results in a table
    printf "%-20s %10s\n" "GCC Version" "Code Size"
    printf "%-20s %10s\n" "--------------------" "----------"
    for gcc_ver in "${GCC_VERSIONS[@]}"; do
        if [ "${results[$gcc_ver]}" = "FAIL" ]; then
            printf "%-20s %10s\n" "$gcc_ver" "FAIL"
        else
            printf "%-20s %10d (0x%X)\n" "$gcc_ver" "${results[$gcc_ver]}" "${results[$gcc_ver]}"
        fi
    done
    
    echo ""
done

echo "==================================================================="
echo "Done!"
echo "==================================================================="
