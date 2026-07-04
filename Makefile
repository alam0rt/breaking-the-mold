# =============================================================================
# PSX Decompilation Project Makefile
# =============================================================================
# This Makefile provides the build infrastructure for a PSX decompilation
# project using splat and the GCC 2.6/2.7 era PSX toolchain.
#
# Based on conventions from sotn-decomp, open-ribbon, and other PSX decomps.
# =============================================================================

# -----------------------------------------------------------------------------
# Configuration - Adjust these for your project
# -----------------------------------------------------------------------------

# Project name (matches binary name, like soul-re)
PROJECT := SLES_010.90

# Splat configuration source and generated YAML file.
# Edit the Jsonnet file; Make renders YAML before invoking splat.
SPLAT_CONFIG_SRC := $(PROJECT).jsonnet
SPLAT_CONFIG := $(PROJECT).yaml
JSONNET := jsonnet

# Expected SHA1 hash (extracted from Jsonnet source)
EXPECTED_SHA1 := $(shell $(JSONNET) -S -e '(import "$(SPLAT_CONFIG_SRC)").sha1' 2>/dev/null || grep -E '^"?sha1"?:' $(SPLAT_CONFIG) 2>/dev/null | sed -E 's/.*"([0-9a-fA-F]{40})".*/\1/' | head -1)

# Build output directory
BUILD_DIR := build

# ASM output directory (from splat)
ASM_DIR := asm

# Source directory
SRC_DIR := src

# -----------------------------------------------------------------------------
# -----------------------------------------------------------------------------
# Toolchain Configuration
# -----------------------------------------------------------------------------
# GCC Version: 2.7.2-psx matches existing decompiled functions.
# Note: 2.5.7-psx produces better matches for some functions (e.g., func_8007A62C)
# but breaks existing matches. May need per-file compiler selection in future.
GCC_VERSION := 2.7.2-psx
GCC_DIR := tools/gcc-$(GCC_VERSION)
CC1 := $(GCC_DIR)/cc1
GCC := $(GCC_DIR)/gcc

# Per-subdirectory compiler/aspsx overrides
# ---------------------------------------------------------------------------
# Borrowed from Vatuu/silent-hill-decomp: their Makefile picks compiler and
# aspsx-version per source path. We pre-stage the alternates here so a stuck
# file/subdir can opt in to a different toolchain by adding a target-specific
# override below the main compile rule, e.g.:
#
#     $(BUILD_DIR)/src/<subdir>/%.o: CC1 := tools/gcc-2.5.7-psx/cc1
#     $(BUILD_DIR)/src/<subdir>/%.o: ASPSX_VERSION := 2.77
#
# Known alternates that match individual functions in past experiments:
#   tools/gcc-2.5.7-psx/cc1     - matches func_8007A62C (breaks others)
#   tools/gcc-2.7.2-cdk/cc1     - Sony "CDK" 970404 build (SH uses for libkpad)
#   tools/gcc-2.8.1-psx/cc1     - matches some 2.8-era code (breaks 2.6/2.7 code)
#   bin/cc1-psx-26              - real Sony GNU C 2.6.3 Sony PSX build
CC1_257  := tools/gcc-2.5.7-psx/cc1
CC1_CDK  := tools/gcc-2.7.2-cdk/cc1
CC1_281  := tools/gcc-2.8.1-psx/cc1
CC1_SONY := bin/cc1-psx-26

# MIPS cross-toolchain (from Nix environment)
# Note: Nix provides mipsel-unknown-linux-gnu-*, not mipsel-linux-gnu-*
# The binutils (as, ld, objcopy) are version-independent for linking
CROSS := mipsel-unknown-linux-gnu-
AS := $(CROSS)as
LD := $(CROSS)ld
OBJCOPY := $(CROSS)objcopy
CPP := cpp
STRIP := $(CROSS)strip

# maspsx - Modern ASPSX replacement
# Transforms GCC assembly output to match PSY-Q ASPSX.EXE behavior
# See: https://github.com/mkst/maspsx
MASPSX := python3 tools/maspsx/maspsx.py

# ast-grep clarity lint rules (see sgconfig.yml / tools/ast-grep/rules)
AST_GREP := ast-grep

# maspsx configuration (adjust based on your game's PSY-Q version)
# PSY-Q 3.3 -> --aspsx-version=2.21
# PSY-Q 3.5 -> --aspsx-version=2.34
# PSY-Q 4.0 -> --aspsx-version=2.56
# PSY-Q 4.1 -> --aspsx-version=2.67
# PSY-Q 4.3 -> --aspsx-version=2.77
# PSY-Q 4.4 -> --aspsx-version=2.79
# PSY-Q 4.5 -> --aspsx-version=2.81
# PSY-Q 4.6 -> --aspsx-version=2.86
# PSY-Q 4.7 -> --aspsx-version=2.86 (same as 4.6)
ASPSX_VERSION := 2.86
# -G8 tells maspsx that symbols <=8 bytes should use GP-relative addressing
# --use-comm-section: emit non-static uninitialized globals as GLOBAL .comm
#   symbols -- this is the DEFAULT ccpsx (real PSY-Q driver) behaviour per the
#   maspsx README. maspsx's own default (lowering them to local .sbss defs) is
#   the deviation, which silently breaks any TU that defines such a global
#   (e.g. menu's D_800A6045). Global here so every file matches the real
#   toolchain; harmless for files with no common symbols. No opt-out flag
#   exists, but none is wanted since this matches ccpsx everywhere.
# MASPSX_EXTRA_FLAGS lets per-target overrides add e.g. --expand-div without
# rewriting the whole MASPSX_FLAGS variable. Uses '=' (recursive) so per-target
# changes to ASPSX_VERSION / MASPSX_EXTRA_FLAGS take effect at recipe time.
MASPSX_EXTRA_FLAGS :=
MASPSX_FLAGS = --aspsx-version=$(ASPSX_VERSION) --run-assembler --gnu-as-path=$(AS) -G8 --use-comm-section $(MASPSX_EXTRA_FLAGS)

# Python (from Nix environment)
PYTHON := python3

# splat binary splitting tool
SPLAT := $(PYTHON) -m splat split

# Post-splat rodata migration: re-applies compiler-emitted rodata (jump tables)
# for functions migrated to C, since splat regenerates asm/ + .ld from the base
# ROM on every extract. Idempotent. See tools/rodata_migrations.json.
RODATA_MIGRATE := $(PYTHON) tools/migrate_rodata.py

# Post-splat blob splitter: spimdisasm glues indirectly-called functions (via
# function-pointer tables) into single .s "blobs" when a sub-8-byte fragment at
# a segment boundary stalls its boundary detector. This splits each blob at its
# glabel/alabel function boundaries into one .s per function so they can be
# individually decompiled. Byte-safe (moves identical instr bytes). See
# tools/split_blobs.py.
SPLIT_BLOBS := $(PYTHON) tools/split_blobs.py

# m2c - MIPS to C decompiler
M2C := $(PYTHON) tools/m2c/m2c.py

# -----------------------------------------------------------------------------
# Compiler Flags
# -----------------------------------------------------------------------------

# C preprocessor flags
CPPFLAGS := -I include -I psyq -D_LANGUAGE_C

# Header-dependency tracking: cpp writes a .d makefile fragment alongside each
# .o (as a side effect of preprocessing) listing the user headers it pulled in.
# -MP emits a phony target per header so deleting/renaming a header doesn't
# wedge the build. These fragments are -include'd below so editing a header
# correctly rebuilds every .o that uses it (the rule prereq is only the .c).
DEPFLAGS = -MMD -MP -MF $(@:.o=.d) -MT $@

# PSX compiler flags (GCC 2.6 compatible)
# -O2: Optimization level 2
# -G8: Small data threshold (variables <=8 bytes go in .sdata for GP-relative access)
# -fno-builtin: Don't use builtin functions
# -mno-abicalls: No PIC/position independent code
# -mcpu=3000: MIPS R3000 (PSX CPU)
CFLAGS := -O2 -G8 -fno-builtin -mno-abicalls -mcpu=3000

# DEBUG=1 adds DWARF line info (-g) for source-level stepping in GDB against the
# side-loaded ELF (see `make emu-exe`). VERIFIED byte-match-safe: the PSX cc1's
# DWARF survives maspsx+as but does NOT change .text, and `objcopy -O binary`
# strips all debug sections, so build/$(PROJECT).bin is byte-identical and
# `make check` still passes. Only the .elf grows (gains .debug_* sections).
ifdef DEBUG
CFLAGS += -g
endif

# Assembler flags
ASFLAGS := -march=r3000 -mtune=r3000 -no-pad-sections -G8 -Iinclude

# Linker flags
LDFLAGS := -nostdlib

# -----------------------------------------------------------------------------
# File Discovery
# -----------------------------------------------------------------------------

# Find all C source files
C_SRCS := $(shell find $(SRC_DIR) -name '*.c' 2>/dev/null)

# Find all ASM source files (from splat extraction)
# Exclude nonmatchings/ - those are included by C files via INCLUDE_ASM
S_SRCS := $(shell find $(ASM_DIR) -name '*.s' -not -path '*/nonmatchings/*' 2>/dev/null)

# Find all binary assets (from splat extraction)
BIN_SRCS := $(shell find assets -name '*.bin' 2>/dev/null)

# Object files - paths must match what splat generates in the linker script
# splat outputs: build/asm/file.o (build_path + source path with .s -> .o)
C_OBJS := $(C_SRCS:%.c=$(BUILD_DIR)/%.o)
S_OBJS := $(S_SRCS:%.s=$(BUILD_DIR)/%.o)
BIN_OBJS := $(BIN_SRCS:%.bin=$(BUILD_DIR)/%.o)

# All object files
OBJS := $(C_OBJS) $(S_OBJS) $(BIN_OBJS)

# Linker script (generated by splat, in repo root like soul-re)
LD_SCRIPT := $(PROJECT).ld

# -----------------------------------------------------------------------------
# Targets
# -----------------------------------------------------------------------------

.PHONY: all clean extract config expected diff context check tools help lint lint-decomp lint-clarity check-lua lint-fix decompme progress setup-hooks ghidra-mcp ghidra-mcp-stop annotate-asm annotate-asm-force analyze analyze-baseline

# Default target - re-extracts if config is newer than linker script or asm/ is missing
all: $(SPLAT_CONFIG)
	@if [ ! -d "$(ASM_DIR)" ] || [ "$(SPLAT_CONFIG)" -nt "$(LD_SCRIPT)" ]; then \
		echo "Config changed or ASM missing, re-extracting..."; \
		rm -rf $(ASM_DIR) $(BUILD_DIR); \
		$(SPLAT) $(SPLAT_CONFIG) && $(RODATA_MIGRATE) && $(SPLIT_BLOBS) && touch $(LD_SCRIPT); \
	fi
	@$(MAKE) --no-print-directory build
	@echo "Build complete!"

# Actual build target (called after ASM exists)
.PHONY: build
build: $(BUILD_DIR)/$(PROJECT).bin

# Show help
help:
	@echo "PSX Decompilation Project (Modern Workflow)"
	@echo "============================================"
	@echo ""
	@echo "Toolchain: GCC $(GCC_VERSION) + maspsx (ASPSX $(ASPSX_VERSION))"
	@echo ""
	@echo "Targets:"
	@echo "  all              - Build the project (default, byte-matching)"
	@echo "  config           - Render $(SPLAT_CONFIG) from $(SPLAT_CONFIG_SRC)"
	@echo "  extract          - Extract/disassemble binary using splat (auto-annotates ASM)"
	@echo "  annotate-asm     - Annotate ASM files in place (idempotent)"
	@echo "  annotate-asm-force - Re-annotate all ASM files (no skip)"
	@echo "  expected         - Copy original binary to expected/"
	@echo "  check            - Verify build matches original (quick)"
	@echo "  verify           - Alias for 'check' (for compatibility)"
	@echo "  clean            - Remove build artifacts"
	@echo ""
	@echo "Code Quality:"
	@echo "  lint             - Run all linters"
	@echo "  lint-clarity     - Report ast-grep C clarity cleanup candidates"
	@echo ""
	@echo "Emulator/Debugging:"
	@echo "  emu              - Launch PCSX-Redux with GDB server (boots the disc)"
	@echo "  emu-exe          - Launch PCSX-Redux running OUR compiled ELF (assets from ISO)"
	@echo "                     (make DEBUG=1 emu-exe for source-level stepping)"
	@echo "  watch            - Run game with behavior tracing (Lua)"
	@echo "  snapshot         - Capture RAM snapshot (web API)"
	@echo "  mcp-server       - Start MCP server for Copilot"
	@echo ""
	@echo "Ghidra Integration:"
	@echo "  ghidra-mcp       - Start GhidraMCP headless server (port 8089)"
	@echo "  ghidra-mcp-stop  - Stop GhidraMCP headless server"
	@echo "  ghidra-export    - Export symbols from Ghidra (PyGhidra)"
	@echo "  ghidra-export-all- Export and save to symbol_addrs_new.txt"
	@echo ""
	@echo "Decompilation:"
	@echo "  diff FUNC=name   - Compare function with asm-differ"
	@echo "  decompile FUNC=f - Decompile function with m2c"
	@echo "  context          - Generate ctx.c for m2c"
	@echo "  permuter FUNC=f  - Run decomp-permuter"
	@echo "  decompme FUNC=f  - Create a decomp.me scratch from nonmatchings/<FUNC>/"
	@echo ""
	@echo "Tools:"
	@echo "  tools            - Download GCC toolchain"
	@echo "  detect-compiler  - Analyze binary's compiler"
	@echo ""
	@echo "Workflow:"
	@echo "  1. ./tools/download-toolchain.sh"
	@echo "  2. make extract"
	@echo "  3. make"
	@echo "  4. make check"
	@echo ""

# -----------------------------------------------------------------------------
# Extraction (splat)
# -----------------------------------------------------------------------------

config: $(SPLAT_CONFIG)

$(SPLAT_CONFIG): $(SPLAT_CONFIG_SRC) Makefile
	@echo "Rendering splat config from $<..."
	@tmp="$@.tmp"; \
	rm -f "$$tmp"; \
	{ \
		echo "# GENERATED FILE - DO NOT EDIT."; \
		echo "# Source: $<"; \
		echo "# Regenerate with: make config"; \
		echo ""; \
		$(JSONNET) -S -e 'std.manifestYamlDoc(import "$<")'; \
	} > "$$tmp" && mv "$$tmp" "$@"

extract: $(SPLAT_CONFIG)
	@echo "Cleaning stale asm/ and build/ before extraction..."
	rm -rf $(ASM_DIR) $(BUILD_DIR)
	@echo "Extracting binary using splat..."
	$(SPLAT) $(SPLAT_CONFIG)
	@$(RODATA_MIGRATE)
	@$(SPLIT_BLOBS)
	@touch $(LD_SCRIPT)
	@$(MAKE) --no-print-directory annotate-asm
	@echo "Extraction complete. ASM files in $(ASM_DIR)/"

# Generate linker script and ASM files if they don't exist
$(LD_SCRIPT): $(SPLAT_CONFIG) $(BASEROM)
	@echo "Running splat to generate linker script and ASM..."
	$(SPLAT) $(SPLAT_CONFIG)
	@$(RODATA_MIGRATE)
	@$(SPLIT_BLOBS)
	@touch $(LD_SCRIPT)

# Annotate splat-extracted .s files in place with symbol/struct/asset/callee
# context (idempotent via --skip-annotated; annotations are `#` comments so
# the assembler ignores them and the build stays byte-identical).
ANNOTATE := $(PYTHON) tools/asm_annotate.py
ANNOTATE_DIRS := $(ASM_DIR)/nonmatchings $(ASM_DIR)/libs
.PHONY: annotate-asm annotate-asm-force
annotate-asm:
	@if [ -d $(ASM_DIR) ]; then \
		echo "Annotating ASM files (skipping already-annotated)..."; \
		find $(ANNOTATE_DIRS) -name '*.s' -print0 2>/dev/null \
			| xargs -0 -r $(ANNOTATE) --overwrite --no-color --skip-annotated; \
	else \
		echo "No $(ASM_DIR)/ to annotate (run 'make extract' first)."; \
	fi
annotate-asm-force:
	@if [ -d $(ASM_DIR) ]; then \
		echo "Re-annotating ALL ASM files (no skip)..."; \
		find $(ANNOTATE_DIRS) -name '*.s' -print0 2>/dev/null \
			| xargs -0 -r $(ANNOTATE) --overwrite --no-color; \
	else \
		echo "No $(ASM_DIR)/ to annotate (run 'make extract' first)."; \
	fi

# -----------------------------------------------------------------------------
# Compilation Rules
# -----------------------------------------------------------------------------

# Create build directories
$(BUILD_DIR)/:
	mkdir -p $@

# Compile C to object file using old-gcc + maspsx pipeline
# This is the MODERN approach that doesn't require wine/dosemu2:
#   1. Preprocess with system cpp
#   2. Compile with old gcc cc1 (same codegen as PSY-Q)
#   3. Pass through maspsx (replicates ASPSX.EXE behavior)
#   4. Assemble with GNU as (via maspsx --run-assembler)
#
# The maspsx wrapper handles:
#   - $gp-relative addressing
#   - NOP insertion for pipeline hazards
#   - div/rem expansion
#   - .comm/.sdata handling
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)/
	@mkdir -p $(dir $@)
	@echo "CC $<"
	$(CPP) $(CPPFLAGS) $(DEPFLAGS) $< | $(CC1) $(CFLAGS) -o $(BUILD_DIR)/$*.s
	$(MASPSX) $(MASPSX_FLAGS) -o $@ $(ASFLAGS) $(BUILD_DIR)/$*.s

# Assemble ASM files (from splat extraction or handwritten)
# Output to build/asm/*.o to match splat linker script
$(BUILD_DIR)/%.o: %.s | $(BUILD_DIR)/
	@mkdir -p $(dir $@)
	@echo "AS $<"
	@$(AS) $(ASFLAGS) $< -o $@

# Convert binary assets to object files
# Uses objcopy to wrap raw binary data into an ELF object
$(BUILD_DIR)/%.o: %.bin | $(BUILD_DIR)/
	@mkdir -p $(dir $@)
	@echo "BIN $<"
	@$(OBJCOPY) -I binary -O elf32-tradlittlemips -B mips $< $@

# Pull in the per-object header dependencies generated by $(DEPFLAGS). Wrapped
# so a clean tree (no build/) doesn't error; the leading '-' ignores missing
# files on the first build.
DEPS := $(shell find $(BUILD_DIR) -name '*.d' 2>/dev/null)
-include $(DEPS)

# ---------------------------------------------------------------------------
# Per-subdir / per-file toolchain overrides
# ---------------------------------------------------------------------------
# Add target-specific variable assignments here when a subsystem needs a
# different cc1, ASPSX version, or maspsx knob. Pattern mirrors
# Vatuu/silent-hill-decomp. Examples (uncomment + adapt when needed):
#
#   # Use Sony CDK 970404 build for libsd/libetc-like glue
#   $(BUILD_DIR)/src/libs/%.o: CC1 := $(CC1_CDK)
#   $(BUILD_DIR)/src/libs/%.o: ASPSX_VERSION := 2.77
#
#   # PSY-Q libsd-style divisions need expanded div sequences
#   $(BUILD_DIR)/src/libs/libsd/%.o: MASPSX_EXTRA_FLAGS := --expand-div
#
#   # A single file that only matches with 2.5.7
#   $(BUILD_DIR)/src/system/foo.o: CC1 := $(CC1_257)
#
# No overrides are active by default.
$(BUILD_DIR)/src/anim.o: MASPSX_EXTRA_FLAGS := --expand-div
$(BUILD_DIR)/src/player.o: MASPSX_EXTRA_FLAGS := --expand-div

# NOTE: --use-comm-section is now applied GLOBALLY (see MASPSX_FLAGS above) since
# it is the default ccpsx behaviour, so menu.o no longer needs a per-file override.
# menu.c relies on it for its tentative-def small globals (e.g. D_800A6045) to get
# single-instruction %gp_rel stores. See gp-rel-extern-blocker memory.

# -----------------------------------------------------------------------------
# Linking
# -----------------------------------------------------------------------------

# Undefined symbols (generated by splat + manual overrides)
UNDEFINED_SYMS := undefined_syms_auto.txt
UNDEFINED_SYMS_MANUAL := undefined_syms.txt
UNDEFINED_FUNCS := undefined_funcs_auto.txt

# Link all objects into ELF
$(BUILD_DIR)/$(PROJECT).elf: $(OBJS) $(LD_SCRIPT) $(UNDEFINED_SYMS) $(UNDEFINED_SYMS_MANUAL) $(UNDEFINED_FUNCS)
	@echo "LD $@"
	@$(LD) $(LDFLAGS) -T $(LD_SCRIPT) -T $(UNDEFINED_SYMS) -T $(UNDEFINED_SYMS_MANUAL) -T $(UNDEFINED_FUNCS) -Map $(BUILD_DIR)/$(PROJECT).map -o $@ $(OBJS)

# Convert ELF to raw binary (PSX format)
$(BUILD_DIR)/$(PROJECT).bin: $(BUILD_DIR)/$(PROJECT).elf
	@echo "OBJCOPY $@"
	@$(OBJCOPY) -O binary $< $@

# -----------------------------------------------------------------------------
# Verification
# -----------------------------------------------------------------------------

# Check if build matches expected SHA1 hash from config
check: all
	@echo "Verifying build against expected SHA1..."
	@ACTUAL=$$(sha1sum $(BUILD_DIR)/$(PROJECT).bin | cut -d' ' -f1); \
	echo "Expected: $(EXPECTED_SHA1)"; \
	echo "Actual:   $$ACTUAL"; \
	if [ "$$ACTUAL" = "$(EXPECTED_SHA1)" ]; then \
		echo "✓ BUILD MATCHES"; \
	else \
		echo "✗ BUILD DOES NOT MATCH"; \
		exit 1; \
	fi

# Verify: Compatibility alias for 'check'
# Usage: make verify
verify: check

# -----------------------------------------------------------------------------
# Diff Tools
# -----------------------------------------------------------------------------

# Side-by-side binary diff (requires original binary file)
# Usage: make bindiff ORIG=path/to/original.bin
# bindiff: $(BUILD_DIR)/$(PROJECT).bin
# 	$(PYTHON) scripts/bindiff.py $(BUILD_DIR)/$(PROJECT).bin $(ORIG)

# Run asm-differ on a function
# Usage: make diff FUNC=FunctionName
diff:
ifndef FUNC
	@echo "Usage: make diff FUNC=FunctionName"
	@exit 1
endif
	$(PYTHON) tools/asm-differ/diff.py -mw3 $(FUNC)

# Generate context file for m2c decompiler
# This preprocesses common.h to create ctx.c with all type definitions
# Usage: make context
CTX_FILE := ctx.c
context: $(CTX_FILE)

$(CTX_FILE): include/common.h
	@echo "Generating $(CTX_FILE)..."
	@cpp -E -P -I include -I psyq -D_LANGUAGE_C include/common.h > $(CTX_FILE)
	@echo "✓ Generated $(CTX_FILE) ($$(wc -l < $(CTX_FILE)) lines)"

# Decompile a function using m2c
# Usage: make decompile FUNC=FunctionName
# The function must exist in nonmatchings/*/FUNC.s
# Automatically generates ctx.c if needed
decompile: $(CTX_FILE)
ifndef FUNC
	@echo "Usage: make decompile FUNC=FunctionName"
	@exit 1
endif
	@ASM_FILE=$$(find $(ASM_DIR)/nonmatchings -name "$(FUNC).s" 2>/dev/null | head -1); \
	if [ -z "$$ASM_FILE" ]; then \
		echo "Error: Could not find $(FUNC).s in $(ASM_DIR)/nonmatchings/"; \
		exit 1; \
	fi; \
	$(PYTHON) tools/m2c/m2c.py --context $(CTX_FILE) -t mipsel-gcc-c "$$ASM_FILE"

# Import function into decomp-permuter
# Usage: make permuter-import FILE=src/main.c FUNC=FunctionName
permuter-import:
ifndef FILE
	@echo "Usage: make permuter-import FILE=src/main.c FUNC=FunctionName"
	@exit 1
endif
	$(PYTHON) tools/decomp-permuter/import.py $(FILE)

# Run decomp-permuter on imported function
# Usage: make permuter FUNC=FunctionName
permuter:
ifndef FUNC
	@echo "Usage: make permuter FUNC=FunctionName"
	@exit 1
endif
	$(PYTHON) tools/decomp-permuter/permuter.py nonmatchings/$(FUNC) --best-only -j4

# Create a decomp.me scratch from nonmatchings/<FUNC>/
# Usage: make decompme FUNC=FunctionName
# Env vars: DECOMPME_COMPILER (default psyq4.0), DECOMPME_FLAGS, DECOMPME_API
decompme:
ifndef FUNC
	@echo "Usage: make decompme FUNC=FunctionName"
	@exit 1
endif
	$(PYTHON) tools/decompme.py $(FUNC) $(DECOMPME_ARGS)

# -----------------------------------------------------------------------------
# Progress
# -----------------------------------------------------------------------------

progress:
	@PURE_ASM=$$(find $(ASM_DIR) -name '*.s' -not -path '*/nonmatchings/*' -exec grep -h '^\s*[ga]label ' {} + 2>/dev/null | awk '{print $$2}' | sort -u | wc -l); \
	NM_FUNCS=$$(find $(ASM_DIR)/nonmatchings -name '*.s' -exec grep -h '^\s*[ga]label ' {} + 2>/dev/null | awk '{print $$2}' | sort -u | wc -l); \
	DECOMPILED=$$(grep -r '^[a-zA-Z_].*(.*).*{' $(SRC_DIR) --include='*.c' 2>/dev/null | grep -v 'INCLUDE_ASM\|extern\|typedef\|struct ' | wc -l); \
	TOTAL=$$((PURE_ASM + NM_FUNCS + DECOMPILED)); \
	echo "Decompilation Progress"; \
	echo "====================="; \
	echo "Fully decompiled:  $$DECOMPILED / $$TOTAL ($$(python3 -c "print(f'{100*$$DECOMPILED/$$TOTAL:.1f}%')" 2>/dev/null))"; \
	echo "INCLUDE_ASM stubs: $$NM_FUNCS / $$TOTAL ($$(python3 -c "print(f'{100*$$NM_FUNCS/$$TOTAL:.1f}%')" 2>/dev/null))"; \
	echo "Pure asm:          $$PURE_ASM / $$TOTAL ($$(python3 -c "print(f'{100*$$PURE_ASM/$$TOTAL:.1f}%')" 2>/dev/null))"

# -----------------------------------------------------------------------------
# Cleanup
# -----------------------------------------------------------------------------

clean:
	rm -rf $(BUILD_DIR)
	rm -rf asm
	rm -rf assets

# Clean only build artifacts, keep extracted files
clean-build:
	rm -rf $(BUILD_DIR)

# Install git hooks from tools/hooks/
setup-hooks:
	@cp tools/hooks/* .git/hooks/ 2>/dev/null && chmod +x .git/hooks/*
	@echo "✓ Git hooks installed"

# =============================================================================
# Code Quality / Linting
# =============================================================================

# Check Lua syntax (fast check without full analysis)
check-lua:
	@echo "Checking Lua syntax..."
	@for f in scripts/*.lua; do \
		luac -p "$$f" || exit 1; \
	done
	@echo "✓ All Lua scripts have valid syntax"

# Lint all code
lint: lint-decomp check-asset-ids
	@echo "Linting complete!"

# Flag raw asset-id literals in src/ that should use an asset_ids.h constant.
# Exits non-zero if any remain, so it gates `make lint`.
check-asset-ids:
	@$(PYTHON) tools/check_asset_ids.py

# Decomp-specific checks (struct redefs, broken INCLUDE_ASM paths, duplicates)
lint-decomp:
	@$(PYTHON) tools/lint_decomp.py

# Analysis build (advisory) - modern-compiler diagnostics + machine-checked
# struct-layout invariants. NEVER feeds the matching build or the ROM bytes.
# See tools/analysis/README.md. Layout drift is a hard failure; warnings are a
# delta vs tools/analysis/warnings.baseline.txt.
analyze:
	@bash tools/analysis/run_analyze.sh check

# Re-snapshot the advisory warning baseline (run after an intentional change
# that legitimately alters the warning set).
analyze-baseline:
	@bash tools/analysis/run_analyze.sh baseline

# C clarity checks (non-blocking while the decomp still has known cleanup debt)
lint-clarity:
	@echo "Running ast-grep clarity checks..."
	@$(AST_GREP) scan --config sgconfig.yml --report-style medium src include || true

.PHONY: lint lint-decomp lint-clarity lint-fix

# =============================================================================
# Launch PCSX-Redux in debug mode (requires nixGL for OpenGL on non-NixOS)
# Connect Ghidra debugger to localhost:3333
# TODO: Use the flake's nixGLIntel wrapper instead of 'nix run' once we figure out
#       how to make the overlay work with --impure for host GPU driver access
# Override with: make emu ISO=path/to/game.iso
ISO ?= disks/pal/sles-01090.iso01.iso
ELF := $(BUILD_DIR)/$(PROJECT).elf
GDB_PORT := 3333

# PCSX-Redux GPU wrapper. Override with `make emu GPU=intel` if needed.
# Auto-detect: NVIDIA if /dev/nvidia0 exists, else Intel.
# Valid values: nvidia, intel, default, none (none = run pcsx-redux directly).
# NOTE: the nixGL Nvidia wrapper forces SDL_VIDEODRIVER=x11 (XWayland + GLX via
# the nvidia driver). The "MESA-LOADER ... wrong ELF class ELFCLASS32" line it
# prints is a harmless GBM probe failure -- GLX still comes from nvidia, so the
# window opens. GPU=none fails on a nix-store pcsx-redux because no GL driver is
# on the loader path.
GPU ?= $(shell test -e /dev/nvidia0 && echo nvidia || echo intel)
# NVIDIA driver is unfree; nixGL needs NIXPKGS_ALLOW_UNFREE=1 to build it.
# Optimus/PRIME laptops also need __NV_PRIME_RENDER_OFFLOAD=1 and SDL_VIDEODRIVER=x11
# so the window is created via X11 (not Wayland EGL) and offloaded to the dGPU.
NIXGL_intel   := nix run --impure github:nix-community/nixGL\#nixGLIntel --
NIXGL_nvidia  := NIXPKGS_ALLOW_UNFREE=1 SDL_VIDEODRIVER=x11 __NV_PRIME_RENDER_OFFLOAD=1 __GLX_VENDOR_LIBRARY_NAME=nvidia nix run --impure github:nix-community/nixGL\#nixGLNvidia --
NIXGL_default := nix run --impure github:nix-community/nixGL --
NIXGL_none    :=
NIXGL_WRAPPER := $(NIXGL_$(GPU))

# PCSX-Redux executable (wrapped for host GPU access)
PCSX := $(NIXGL_WRAPPER) pcsx-redux

# Launch PCSX-Redux emulator with GDB server (runs in foreground)
# NOTE: Web server must be enabled manually in GUI:
#   Configuration > Emulation > Enable Web Server
.PHONY: emu emu-exe
emu:
	$(PCSX) -interpreter -debugger -gdb -iso $(ISO) -run -fastboot

# Side-load OUR compiled binary instead of the disc's executable, while the
# original ISO supplies BLB assets (the game loads them by name via CdSearchFile
# -- see src/gamecd.c). PCSX-Redux's -exe accepts an ELF and loads it at the
# address in its header (.text @ 0x80010000, same place the BIOS would put the
# disc exe), so the disc's own executable is never run. This decouples code
# iteration from the ISO: rebuild the ELF, relaunch -- no ISO repack needed.
#
#   make emu-exe              # symbol-level debugging (named breakpoints/bt)
#   make DEBUG=1 emu-exe      # rebuild with -g first for source-level stepping
#
# Then connect GDB:  gdb -x tools/skullmonkeys.gdb
emu-exe: $(ELF)
	$(PCSX) -interpreter -debugger -gdb -iso $(ISO) -exe $(ELF) -run -fastboot

# Run MCP server for Copilot integration
# PCSX-Redux must have web server enabled on port 8080
# Enable: Configuration > Emulation > Enable Web Server
mcp-server:
	python3 scripts/pcsx_mcp_server.py

# Game Watcher - Record gameplay traces with comprehensive debugging
# Output: game_watcher/logs/trace_TIMESTAMP_LEVEL_fFRAME.jsonl (auto-saved on exit)
# Usage: make record [LEVEL=1] [STAGE=0]
#        make record LEVEL=5 STAGE=1    # Start at MOSS Stage 1
#        make record LEVEL=SCIE STAGE=0 # Use level ID
# Commands in Lua console:
#   status(), entities(), stats()
#   dump_log(), clear_log()
#   mark('label'), markers()
#   load_level(idx, stage), set_boot_override(idx, stage)
RECORD_LEVEL ?= $(LEVEL)
RECORD_STAGE ?= $(STAGE)

record: check-lua
	@echo "Starting PCSX-Redux with game watcher..."
	@mkdir -p game_watcher/logs
ifdef LEVEL
	@echo "Boot override: Level $(LEVEL), Stage $(or $(STAGE),0)"
	@cp scripts/game_watcher.lua /tmp/watcher_override.lua
	@echo "" >> /tmp/watcher_override.lua
	@echo "-- Override applied via make record" >> /tmp/watcher_override.lua
	@echo "CONFIG.boot_level_override = {level=$(LEVEL), stage=$(or $(STAGE),0)}" >> /tmp/watcher_override.lua
	@echo "reload_watchers()" >> /tmp/watcher_override.lua
	$(PCSX) -interpreter -debugger -lua_stdout -iso $(ISO) -dofile /tmp/watcher_override.lua -run
else
	@echo "No level override (normal boot). Use: make record LEVEL=1 STAGE=0"
	$(PCSX) -interpreter -debugger -lua_stdout -iso $(ISO) -dofile scripts/game_watcher.lua -run
endif

# Legacy alias
watch: record
	@echo "Note: 'make watch' is deprecated. Use 'make record' instead."

# GameState Dumper - Dump raw GameState bytes for analysis
# Usage: make dump-gamestate [LEVEL=1] [STAGE=0]
dump-gamestate: check-lua
	@echo "Starting PCSX-Redux with GameState dumper..."
	@mkdir -p game_watcher/logs
ifdef LEVEL
	@echo "Boot override: Level $(LEVEL), Stage $(or $(STAGE),0)"
	@cp scripts/gamestate_dumper.lua /tmp/gamestate_override.lua
	@echo "" >> /tmp/gamestate_override.lua
	@echo "-- Override applied via make dump-gamestate" >> /tmp/gamestate_override.lua
	@echo "CONFIG.boot_level_override = {level=$(LEVEL), stage=$(or $(STAGE),0)}" >> /tmp/gamestate_override.lua
	$(PCSX) -interpreter -debugger -lua_stdout -iso $(ISO) -dofile /tmp/gamestate_override.lua -run
else
	@echo "No level override (normal boot). Use: make dump-gamestate LEVEL=1 STAGE=0"
	$(PCSX) -interpreter -debugger -lua_stdout -iso $(ISO) -dofile scripts/gamestate_dumper.lua -run
endif

# Quick RAM snapshot (requires web server enabled in PCSX-Redux)
# Usage: make snapshot
snapshot:
	@echo "Capturing RAM snapshot..."
	python3 scripts/ram_snapshot.py -v -o /tmp/skullmonkeys_snapshot.json
	@echo "Saved to /tmp/skullmonkeys_snapshot.json"

# Raw RAM dump via the GDB stub (PCSX-Redux must be running with -gdb).
# Bulk `dump binary memory` is the only reliable read over the stub (small reads
# flake; inferior calls / register writes wedge it). Defaults to the
# GameState-covering region (fast, ~1 min) which still holds g_pGameState, the
# GameState, the BLB header buffer, and the entity heap. For everything use the
# full range: make memdump MEMSTART=0x80000000 MEMEND=0x80200000 (slower).
# Usage:
#   make memdump                         # -> dumps/snap_<timestamp>.bin
#   make memdump MEMDUMP=dumps/foo.bin   # explicit path
.PHONY: memdump
MEMDUMP  ?= dumps/snap_$(shell date +%Y%m%d_%H%M%S).bin
MEMSTART ?= 0x80090000
MEMEND   ?= 0x801b0000
memdump:
	@mkdir -p dumps
	@echo "Dumping RAM $(MEMSTART)..$(MEMEND) from :$(GDB_PORT) ..."
	gdb -nx -batch -ex 'set pagination off' -ex 'set confirm off' \
	    -ex 'target remote localhost:$(GDB_PORT)' \
	    -ex 'dump binary memory $(MEMDUMP) $(MEMSTART) $(MEMEND)'
	@test -s "$(MEMDUMP)" && echo "wrote $(MEMDUMP) ($$(stat -c%s $(MEMDUMP)) bytes)" \
	    || { echo "dump failed -- is PCSX-Redux running with -gdb on :$(GDB_PORT)?"; exit 1; }
	@echo "analyze: tools/memreport.py $(MEMDUMP) --base $(MEMSTART) --summary"

# Run a Lua script in PCSX-Redux.
# Usage: make lua SCRIPT=scripts/hello.lua
#        make lua SCRIPT=scripts/breaks.lua BREAK=1  # use -interpreter for breakpoints
#
# Default uses -dynarec (fast). Breakpoints via PCSX.addBreakpoint() require
# the interpreter -- pass BREAK=1 to switch.
PCSX_CPU := $(if $(BREAK),-interpreter -debugger,-dynarec)

lua:
ifndef SCRIPT
	@echo "Usage: make lua SCRIPT=scripts/hello.lua [BREAK=1]"
	@exit 1
endif
	$(PCSX) $(PCSX_CPU) -lua_stdout -iso $(ISO) -dofile $(SCRIPT) -run

# Struct watcher with auto-launch (dynarec).
# Usage: make watcher [SCRIPT=scripts/struct_watcher.lua] [LEVEL=5] [STAGE=0]
# LEVEL/STAGE: polls BLB header via vsync and patches level/stage indices
# once the header is loaded (no breakpoints, dynarec-safe).
WATCHER_SCRIPT ?= scripts/struct_watcher.lua
watcher:
	@mkdir -p game_watcher/logs
ifdef LEVEL
	@echo "Boot override: Level $(LEVEL), Stage $(or $(STAGE),0) (dynarec, vsync poller)"
	@cp $(WATCHER_SCRIPT) /tmp/struct_watcher_override.lua
	@echo "" >> /tmp/struct_watcher_override.lua
	@echo "-- Boot override appended by make watcher (dynarec, vsync polled)" >> /tmp/struct_watcher_override.lua
	@echo "do" >> /tmp/struct_watcher_override.lua
	@echo "  local LVL, STG = $(LEVEL), $(or $(STAGE),0)" >> /tmp/struct_watcher_override.lua
	@echo "  local m = PCSX.getMemPtr()" >> /tmp/struct_watcher_override.lua
	@echo "  local function r8(a) return m[bit.band(a,0x1FFFFF)] end" >> /tmp/struct_watcher_override.lua
	@echo "  local function w8(a,v) m[bit.band(a,0x1FFFFF)] = bit.band(v,0xFF) end" >> /tmp/struct_watcher_override.lua
	@echo "  local applied, written = false, 0" >> /tmp/struct_watcher_override.lua
	@echo "  local lst" >> /tmp/struct_watcher_override.lua
	@echo "  lst = PCSX.Events.createEventListener('GPU::Vsync', function()" >> /tmp/struct_watcher_override.lua
	@echo "    if applied then return end" >> /tmp/struct_watcher_override.lua
	@echo "    if r8(0x800AF311) == 0 then return end  -- BLB header not loaded yet" >> /tmp/struct_watcher_override.lua
	@echo "    w8(0x800AF316, 3)         -- game_mode = level" >> /tmp/struct_watcher_override.lua
	@echo "    w8(0x800AF372, LVL)        -- level index" >> /tmp/struct_watcher_override.lua
	@echo "    w8(0x800AF373, STG)        -- stage index" >> /tmp/struct_watcher_override.lua
	@echo "    written = written + 1" >> /tmp/struct_watcher_override.lua
	@echo "    if written >= 60 then       -- ~1s of forcing, then stop" >> /tmp/struct_watcher_override.lua
	@echo "      applied = true" >> /tmp/struct_watcher_override.lua
	@echo "      lst:remove()" >> /tmp/struct_watcher_override.lua
	@echo "      print(string.format('[BOOT] override applied: level=%d stage=%d (forced %d frames)', LVL, STG, written))" >> /tmp/struct_watcher_override.lua
	@echo "    end" >> /tmp/struct_watcher_override.lua
	@echo "  end)" >> /tmp/struct_watcher_override.lua
	@echo "  print(string.format('[BOOT] watcher will override to level=%d stage=%d once BLB header loads', LVL, STG))" >> /tmp/struct_watcher_override.lua
	@echo "end" >> /tmp/struct_watcher_override.lua
ifeq ($(LEVEL),4)
	@echo "" >> /tmp/struct_watcher_override.lua
	@echo "-- FINN vehicle overlay watch (FinnPlayerEntity, fields beyond 0x80)" >> /tmp/struct_watcher_override.lua
	@echo "do" >> /tmp/struct_watcher_override.lua
	@echo "  local function _finn_player_ptr()" >> /tmp/struct_watcher_override.lua
	@echo "    local gs_off = bit.band(0x8009DC40, 0x1FFFFF)" >> /tmp/struct_watcher_override.lua
	@echo "    local pe = mem_word(gs_off + 0x30)" >> /tmp/struct_watcher_override.lua
	@echo "    if not is_valid_ptr(pe) then return nil end" >> /tmp/struct_watcher_override.lua
	@echo "    return pe" >> /tmp/struct_watcher_override.lua
	@echo "  end" >> /tmp/struct_watcher_override.lua
	@echo "  add_watch('FINN', _finn_player_ptr, 'FinnPlayerEntity', 1)" >> /tmp/struct_watcher_override.lua
	@echo "  set_field_filter('FINN', {" >> /tmp/struct_watcher_override.lua
	@echo "    'pInput'," >> /tmp/struct_watcher_override.lua
	@echo "    'wakeEntity_or_velocityX'," >> /tmp/struct_watcher_override.lua
	@echo "    'velocityY_or_stateCallback'," >> /tmp/struct_watcher_override.lua
	@echo "    'rotationAngle'," >> /tmp/struct_watcher_override.lua
	@echo "    'rotationSpriteBucket'," >> /tmp/struct_watcher_override.lua
	@echo "    'rotationVelocity'," >> /tmp/struct_watcher_override.lua
	@echo "    'soundHandle_or_inputFlags'," >> /tmp/struct_watcher_override.lua
	@echo "  })" >> /tmp/struct_watcher_override.lua
	@echo "  print('[FINN] vehicle overlay watch installed (rotation/heading fields)')" >> /tmp/struct_watcher_override.lua
	@echo "end" >> /tmp/struct_watcher_override.lua
endif
	$(PCSX) -dynarec -lua_stdout -iso $(ISO) -dofile /tmp/struct_watcher_override.lua -run
else
	$(PCSX) -dynarec -lua_stdout -iso $(ISO) -dofile $(WATCHER_SCRIPT) -run
endif

# Run inline Lua code
# Usage: make lua-exec CODE='print("hello")'
lua-exec:
ifndef CODE
	@echo "Usage: make lua-exec CODE='print(\"hello\")'"
	@exit 1
endif
	$(PCSX) -lua_stdout -iso $(ISO) -exec '$(CODE)' -run

# Print debug instructions for Ghidra native debugger
debug:
	@echo "=== PSX Debug Session (Ghidra Native Debugger) ==="
	@echo ""
	@echo "1. Run 'make emu' to start PCSX-Redux with GDB server on port $(GDB_PORT)"
	@echo ""
	@echo "2. In Ghidra:"
	@echo "   a. Right-click project -> 'Open With' -> 'Debugger'"
	@echo "   b. In 'Debugger Targets' window, click the connector button (top-right)"
	@echo "   c. Select 'gdb' from dropdown"
	@echo "   d. Set launch command to:"
	@echo "      ghidra-gdb -i mi2"
	@echo "   e. Click 'Connect'"
	@echo ""
	@echo "3. In Ghidra's GDB Interpreter window:"
	@echo "   (gdb) source $(CURDIR)/tools/ghidra_debugger_scripts"
	@echo "   (gdb) target remote localhost:$(GDB_PORT)"
	@echo ""
	@echo "4. In 'Modules' tab: right-click top line -> 'Map Module to <your project>'"
	@echo ""
	@echo "You're now debugging! Use Ghidra's debugger controls (F5=continue, F10=step, etc.)"

# Export symbols from Ghidra using PyGhidra (headless, more accurate)
# Requires: GHIDRA_INSTALL_DIR set, PyGhidra installed
# Usage: make ghidra-export [FORMAT=text|yaml|json|symbol_addrs]
GHIDRA_PROJECT_DIR ?= $(if $(wildcard /mnt/share/sam/ghidra/skullmonkeys.gpr),/mnt/share/sam/ghidra,$(HOME)/ghidra_projects)
GHIDRA_PROJECT_NAME ?= skullmonkeys
GHIDRA_PROGRAM ?= SLES_010.90
EXPORT_FORMAT ?= symbol_addrs
GHIDRA_MCP_URL ?= http://127.0.0.1:8089
GHIDRA_MCP_PORT ?= 8089
GHIDRA_EXPORT_BACKEND ?= mcp

# -----------------------------------------------------------------------------
# GhidraMCP Headless Server
# Starts the GhidraMCP HTTP server in headless mode (no GUI required).
# This exposes the Ghidra project over HTTP for use by MCP tools (Copilot, etc).
# Requires: GHIDRA_INSTALL_DIR set (automatic in nix develop shell)
# Usage: make ghidra-mcp
# -----------------------------------------------------------------------------
.PHONY: ghidra-mcp ghidra-mcp-stop
ghidra-mcp:
	@if [ -z "$$GHIDRA_INSTALL_DIR" ]; then \
		echo "Error: GHIDRA_INSTALL_DIR not set. Run inside 'nix develop' or set manually."; \
		exit 1; \
	fi
	@if ss -tlnp 2>/dev/null | grep -q ":$(GHIDRA_MCP_PORT) "; then \
		echo "Error: Port $(GHIDRA_MCP_PORT) already in use. Run 'make ghidra-mcp-stop' first."; \
		exit 1; \
	fi
	@echo "Starting GhidraMCP headless server on port $(GHIDRA_MCP_PORT)..."
	@echo "  Project: $(GHIDRA_PROJECT_DIR)/$(GHIDRA_PROJECT_NAME).gpr"
	@echo "  Program: /$(GHIDRA_PROGRAM)"
	@echo "  URL:     $(GHIDRA_MCP_URL)"
	@echo ""
	$$GHIDRA_INSTALL_DIR/support/launch.sh fg jdk GhidraMCP-Headless 2G \
		"-Djava.awt.headless=true -XX:ParallelGCThreads=2 -XX:CICompilerCount=2" \
		com.xebyte.headless.GhidraMCPHeadlessServer \
		--project $(GHIDRA_PROJECT_DIR)/$(GHIDRA_PROJECT_NAME).gpr \
		--program /$(GHIDRA_PROGRAM) \
		--port $(GHIDRA_MCP_PORT)

ghidra-mcp-stop:
	@if ss -tlnp 2>/dev/null | grep -q ":$(GHIDRA_MCP_PORT) "; then \
		pid=$$(ss -tlnp | grep ":$(GHIDRA_MCP_PORT) " | grep -oP 'pid=\K[0-9]+'); \
		if [ -n "$$pid" ]; then \
			echo "Stopping GhidraMCP server (PID $$pid)..."; \
			kill "$$pid"; \
		fi; \
	else \
		echo "No GhidraMCP server running on port $(GHIDRA_MCP_PORT)."; \
	fi

.PHONY: ghidra-export ghidra-export-all
ghidra-export:
	@if [ "$(GHIDRA_EXPORT_BACKEND)" = "pyghidra" ] && [ -z "$$GHIDRA_INSTALL_DIR" ]; then \
		echo "Error: GHIDRA_INSTALL_DIR not set"; \
		echo "Set it in your shell or flake.nix will auto-detect common paths"; \
		exit 1; \
	fi
	python3 tools/scripts/pyghidra_export_symbols.py \
		--project $(GHIDRA_PROJECT_DIR) \
		--name $(GHIDRA_PROJECT_NAME) \
		--program $(GHIDRA_PROGRAM) \
		--format $(EXPORT_FORMAT) \
		--functions-only \
		$(if $(filter mcp,$(GHIDRA_EXPORT_BACKEND)),--use-mcp --mcp-url $(GHIDRA_MCP_URL),) \
		-v

# Export function symbols for manual review/merge into symbol_addrs.txt.
# Struct/type exports should live in a separate type-header workflow.
ghidra-export-all:
	@if [ "$(GHIDRA_EXPORT_BACKEND)" = "pyghidra" ] && [ -z "$$GHIDRA_INSTALL_DIR" ]; then \
		echo "Error: GHIDRA_INSTALL_DIR not set"; \
		exit 1; \
	fi
	@echo "Exporting symbols from Ghidra..."
	python3 tools/scripts/pyghidra_export_symbols.py \
		--project $(GHIDRA_PROJECT_DIR) \
		--name $(GHIDRA_PROJECT_NAME) \
		--program $(GHIDRA_PROGRAM) \
		--format symbol_addrs \
		--functions-only \
		-o symbol_addrs_new.txt \
		$(if $(filter mcp,$(GHIDRA_EXPORT_BACKEND)),--use-mcp --mcp-url $(GHIDRA_MCP_URL),) \
		-v
	@echo ""
	@echo "New symbols saved to symbol_addrs_new.txt"
	@echo "Review and merge into symbol_addrs.txt manually"

# -----------------------------------------------------------------------------
# Ghidra C export (best/latest decompiled representation for analysis/decomp)
# -----------------------------------------------------------------------------
# Exports the live Ghidra program to export/:
#   export/$(GHIDRA_PROGRAM).c  - whole-program C (datatype defs + decompiled bodies)
#   export/datatypes.txt        - every struct/union/enum: offset/size/type/name/comment
#   export/functions.txt        - every function signature + plate comment
# Requirements: run inside `nix develop` (sets GHIDRA_INSTALL_DIR, provides
# pyghidra) and keep the Ghidra GUI CLOSED (project lock). Uses the GHIDRA_*
# project vars defined above.
EXPORT_DIR := export

.PHONY: export
export:
	@echo "Exporting Ghidra program $(GHIDRA_PROGRAM) -> $(EXPORT_DIR)/ (this re-decompiles; takes a few minutes)..."
	@$(PYTHON) tools/scripts/ghidra_full_export.py \
		--project $(GHIDRA_PROJECT_DIR) \
		--name $(GHIDRA_PROJECT_NAME) \
		--program $(GHIDRA_PROGRAM) \
		--out $(EXPORT_DIR)
	@echo "Done -> $(EXPORT_DIR)/$(GHIDRA_PROGRAM).c (+ datatypes.txt, functions.txt)"

# -----------------------------------------------------------------------------
# Asset Extraction
# -----------------------------------------------------------------------------

.PHONY: layers layers-all sprites

# Extract all tilemap layers from all levels (default action for layers)
layers: layers-all

# Extract all tilemap layers from all levels
layers-all:
	@echo "Extracting all tilemap layers..."
	python3 scripts/extract_layers.py --output output/layers
	@echo "Done! Layers saved to output/layers/"

# Extract layers from a specific level (usage: make layers-level LEVEL=PHRO)
layers-level:
ifndef LEVEL
	@echo "Usage: make layers-level LEVEL=PHRO"
	@echo "       make layers-level LEVEL=1"
	@echo ""
	@echo "Use 'make layers-list' to see available levels"
	@exit 1
endif
	python3 scripts/extract_layers.py --level $(LEVEL) --output output/layers

# List all available levels
layers-list:
	python3 scripts/extract_layers.py --list

# -----------------------------------------------------------------------------
# BLB asset extraction (graphics/audio/sprites -> extracted/)
# -----------------------------------------------------------------------------
# Two stages: (1) imhex parses GAME.BLB against docs/blb.hexpat into a big JSON
# structure; (2) tools.extract_blb walks that JSON to dump PNGs/GIFs/etc.
# Both outputs are clean-room game data -- NEVER committed (see .gitignore).
# Sprites are keyed by hash: extracted/<LEVEL>/**/sprite_<decimal-hash>_*.png
# (tools/memreport.py links live entities to these).
#   make blb-json      # (re)generate the parsed JSON
#   make extract-blb   # parse if needed, then extract all assets
.PHONY: blb-json extract-blb
BLB      ?= disks/blb/GAME.BLB
BLB_JSON ?= /tmp/blb_parsed.json

$(BLB_JSON): $(BLB) docs/blb.hexpat
	imhex --pl format --pattern docs/blb.hexpat --input $(BLB) > $(BLB_JSON)

blb-json: $(BLB_JSON)

extract-blb: $(BLB_JSON)
	python3 -m tools.extract_blb -b $(BLB) -j $(BLB_JSON) -o extracted/

