# =============================================================================
# Bash Aliases for PSX Decompilation
# =============================================================================
# Add these to your ~/.bash_aliases or source this file in shellHook
#
# Usage:
#   source tools/.bash_aliases
# =============================================================================

# asm-differ - Compare decompiled function against original
# Usage: ad FunctionName
# Options: -m show missing asm, -w watch mode, -o show obj files, -3 three-way diff
alias ad='python3 tools/asm-differ/diff.py -mwo3 '

# asm-differ with larger output (for big functions)
alias ad2='python3 tools/asm-differ/diff.py -mwo3 --max-size 10000 '

# m2c - Decompile MIPS assembly to C
# Usage: m2c asm/us/nonmatchings/main/func_80012345.s
alias m2c='python3 tools/m2c/m2c.py --target mipsel-gcc-c --context ctx.c '

# decomp-permuter import - Import function for permutation
# Usage: di src/main.c
alias di='python3 tools/decomp-permuter/import.py '

# decomp-permuter - Run permuter on imported function
# Usage: dp nonmatchings/func_80012345/
alias dp='python3 tools/decomp-permuter/permuter.py --best-only -j4 '

# mipsmatch - Find matching patterns
# Usage: mm func_80012345
alias mm='python3 tools/mipsmatch/mipsmatch.py '

# Quick rebuild and diff
# Usage: rd FunctionName
rd() {
    make -j && python3 tools/asm-differ/diff.py -mwo3 "$1"
}

# First diff - Find first difference between build and original
alias fd='python3 tools/first-diff.py '

# Generate context file for m2c
alias ctx='make context'

# Watch mode diff (auto-rebuild on file change)
# Usage: wd FunctionName
wd() {
    python3 tools/asm-differ/diff.py -mwo3 --watch "$1"
}

# Grep for function in asm
# Usage: gf pattern
gf() {
    grep -r "$1" asm/ --include="*.s"
}

# Grep for symbol in source
# Usage: gs pattern
gs() {
    grep -r "$1" src/ include/ --include="*.c" --include="*.h"
}
