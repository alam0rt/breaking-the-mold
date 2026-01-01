#!/usr/bin/env bash
# Compare compiled output across different GCC versions
# Usage: ./scripts/compare_gcc_versions.sh <source_file.c> [function_name]
# Example: ./scripts/compare_gcc_versions.sh src/LevelDataParser.c func_8007A62C

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
GCC_BUILDS="$PROJECT_ROOT/tools/gcc-builds"
OUTPUT_DIR="$PROJECT_ROOT/build/gcc-compare"

SOURCE_FILE="$1"
FUNCTION_NAME="${2:-}"

if [ -z "$SOURCE_FILE" ]; then
    echo "Usage: $0 <source_file.c> [function_name]"
    echo ""
    echo "Compiles a source file with multiple GCC versions and compares the output."
    echo "If function_name is provided, extracts just that function for comparison."
    exit 1
fi

if [ ! -f "$SOURCE_FILE" ]; then
    echo "Error: Source file not found: $SOURCE_FILE"
    exit 1
fi

# Check for available GCC builds
if [ ! -d "$GCC_BUILDS" ]; then
    echo "No GCC builds found. Run ./scripts/build_old_gcc.sh first."
    exit 1
fi

mkdir -p "$OUTPUT_DIR"

# Compiler flags (matching project Makefile)
CFLAGS="-O2 -G8 -mno-abicalls -mcpu=3000 -quiet"
INCLUDES="-I $PROJECT_ROOT/include -I $PROJECT_ROOT/psyq"

# Get base name of source file
BASENAME=$(basename "$SOURCE_FILE" .c)

echo "Comparing GCC versions for: $SOURCE_FILE"
echo "Compiler flags: $CFLAGS"
echo ""

# Preprocess the file once
PREPROCESSED_FILE="$OUTPUT_DIR/${BASENAME}.i"
cpp -D_LANGUAGE_C -DNON_MATCHING $INCLUDES "$SOURCE_FILE" 2>/dev/null | grep -v '^# ' > "$PREPROCESSED_FILE"

# Store results for comparison
declare -A RESULTS
declare -A SIZES

for gcc_dir in "$GCC_BUILDS"/gcc-*; do
    if [ ! -d "$gcc_dir" ]; then
        continue
    fi
    
    VERSION=$(basename "$gcc_dir" | sed 's/gcc-//')
    CC1="$gcc_dir/cc1"
    
    if [ ! -f "$CC1" ]; then
        echo "[$VERSION] cc1 not found, skipping..."
        continue
    fi
    
    echo -n "[$VERSION] Compiling..."
    
    OUTPUT_S="$OUTPUT_DIR/${BASENAME}_${VERSION}.s"
    
    # Compile with this GCC version
    if "$CC1" $CFLAGS "$PREPROCESSED_FILE" -o "$OUTPUT_S" 2>/dev/null; then
        # Count lines (rough measure of code size)
        LINE_COUNT=$(wc -l < "$OUTPUT_S")
        
        # Count actual instructions
        INST_COUNT=$(grep -cE '^\t(addiu|addu|subu|sll|srl|sra|and|or|xor|nor|lui|lw|sw|lb|sb|lhu|sh|lbu|beq|bne|bgtz|bgez|bltz|blez|j|jal|jr|jalr|mult|div|mflo|mfhi|nop|move|la|li|slt|sltu|slti|sltiu|andi|ori|xori)' "$OUTPUT_S" 2>/dev/null || echo "0")
        
        echo " OK ($LINE_COUNT lines, ~$INST_COUNT instructions)"
        RESULTS[$VERSION]="$LINE_COUNT lines"
        SIZES[$VERSION]="$INST_COUNT"
    else
        echo " FAILED"
        RESULTS[$VERSION]="compile failed"
        SIZES[$VERSION]="0"
    fi
done

echo ""
echo "=== Summary ==="
for VERSION in $(echo "${!RESULTS[@]}" | tr ' ' '\n' | sort -V); do
    printf "  %-20s %s (~%s instructions)\n" "$VERSION:" "${RESULTS[$VERSION]}" "${SIZES[$VERSION]}"
done

echo ""
echo "Output files in: $OUTPUT_DIR"

# Compare the assembly files
echo ""
echo "=== Diff Summary (vs first version) ==="
FIRST_FILE=""
for asm_file in "$OUTPUT_DIR/${BASENAME}"_*.s; do
    if [ -z "$FIRST_FILE" ]; then
        FIRST_FILE="$asm_file"
        echo "Base: $(basename "$FIRST_FILE")"
        continue
    fi
    
    DIFF_LINES=$(diff "$FIRST_FILE" "$asm_file" 2>/dev/null | wc -l || echo "0")
    echo "  vs $(basename "$asm_file"): $DIFF_LINES lines differ"
done

# Clean up preprocessed file
rm -f "$PREPROCESSED_FILE"
