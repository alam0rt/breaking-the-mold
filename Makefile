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
# MASPSX_EXTRA_FLAGS lets per-target overrides add e.g. --expand-div without
# rewriting the whole MASPSX_FLAGS variable. Uses '=' (recursive) so per-target
# changes to ASPSX_VERSION / MASPSX_EXTRA_FLAGS take effect at recipe time.
MASPSX_EXTRA_FLAGS :=
MASPSX_FLAGS = --aspsx-version=$(ASPSX_VERSION) --run-assembler --gnu-as-path=$(AS) -G8 $(MASPSX_EXTRA_FLAGS)

# Python (from Nix environment)
PYTHON := python3

# splat binary splitting tool
SPLAT := $(PYTHON) -m splat split

# m2c - MIPS to C decompiler
M2C := $(PYTHON) tools/m2c/m2c.py

# -----------------------------------------------------------------------------
# Compiler Flags
# -----------------------------------------------------------------------------

# C preprocessor flags
CPPFLAGS := -I include -I psyq -D_LANGUAGE_C

# PSX compiler flags (GCC 2.6 compatible)
# -O2: Optimization level 2
# -G8: Small data threshold (variables <=8 bytes go in .sdata for GP-relative access)
# -fno-builtin: Don't use builtin functions
# -mno-abicalls: No PIC/position independent code
# -mcpu=3000: MIPS R3000 (PSX CPU)
CFLAGS := -O2 -G8 -fno-builtin -mno-abicalls -mcpu=3000

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

.PHONY: all clean extract config expected diff context check tools help lint lint-lua check-lua lint-fix decompme

# Default target - uses recursive make if ASM files don't exist
all: $(SPLAT_CONFIG)
	@if [ ! -d "$(ASM_DIR)" ]; then \
		echo "ASM directory missing, running splat..."; \
		$(SPLAT) $(SPLAT_CONFIG) && touch $(LD_SCRIPT); \
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
	@echo "  extract          - Extract/disassemble binary using splat"
	@echo "  expected         - Copy original binary to expected/"
	@echo "  check            - Verify build matches original (quick)"
	@echo "  verify           - Alias for 'check' (for compatibility)"
	@echo "  clean            - Remove build artifacts"
	@echo ""
	@echo "Code Quality:"
	@echo "  lint             - Run all linters (Lua)"
	@echo "  lint-lua         - Lint Lua scripts with luacheck"
	@echo ""
	@echo "Emulator/Debugging:"
	@echo "  emu              - Launch PCSX-Redux with GDB server"
	@echo "  watch            - Run game with behavior tracing (Lua)"
	@echo "  snapshot         - Capture RAM snapshot (web API)"
	@echo "  mcp-server       - Start MCP server for Copilot"
	@echo ""
	@echo "Ghidra Integration:"
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
	@echo "Extracting binary using splat..."
	$(SPLAT) $(SPLAT_CONFIG)
	@touch $(LD_SCRIPT)
	@echo "Extraction complete. ASM files in $(ASM_DIR)/"

# Generate linker script and ASM files if they don't exist
$(LD_SCRIPT): $(SPLAT_CONFIG) $(BASEROM)
	@echo "Running splat to generate linker script and ASM..."
	$(SPLAT) $(SPLAT_CONFIG)
	@touch $(LD_SCRIPT)

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
	$(CPP) $(CPPFLAGS) $< | $(CC1) $(CFLAGS) -o $(BUILD_DIR)/$*.s
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
$(BUILD_DIR)/src/Game/ENGINE/animation_setters.o: MASPSX_EXTRA_FLAGS := --expand-div

# -----------------------------------------------------------------------------
# Linking
# -----------------------------------------------------------------------------

# Undefined symbols (generated by splat)
UNDEFINED_SYMS := undefined_syms_auto.txt
UNDEFINED_FUNCS := undefined_funcs_auto.txt

# Link all objects into ELF
$(BUILD_DIR)/$(PROJECT).elf: $(OBJS) $(LD_SCRIPT) $(UNDEFINED_SYMS) $(UNDEFINED_FUNCS)
	@echo "LD $@"
	@$(LD) $(LDFLAGS) -T $(LD_SCRIPT) -T $(UNDEFINED_SYMS) -T $(UNDEFINED_FUNCS) -Map $(BUILD_DIR)/$(PROJECT).map -o $@ $(OBJS)

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
# Cleanup
# -----------------------------------------------------------------------------

clean:
	rm -rf $(BUILD_DIR)
	rm -rf asm
	rm -rf assets

# Clean only build artifacts, keep extracted files
clean-build:
	rm -rf $(BUILD_DIR)

# =============================================================================
# Code Quality / Linting
# =============================================================================

# Lint Lua scripts
lint-lua:
	@echo "Running luacheck on Lua scripts..."
	@luacheck scripts/*.lua --globals PCSX bit mem --no-max-line-length --no-unused-args --codes || true

# Check Lua syntax (fast check without full analysis)
check-lua: lint-lua
	@echo "Checking Lua syntax..."
	@for f in scripts/*.lua; do \
		luac -p "$$f" || exit 1; \
	done
	@echo "✓ All Lua scripts have valid syntax"

# Lint all code
lint: lint-lua
	@echo "Linting complete!"

# Fix common Lua issues automatically
lint-fix:
	@echo "Auto-fixing Lua scripts not yet implemented (luacheck doesn't auto-fix)"
	@echo "Please manually fix issues reported by: make lint-lua"

.PHONY: lint lint-lua lint-fix

# =============================================================================
# Launch PCSX-Redux in debug mode (requires nixGL for OpenGL on non-NixOS)
# Connect Ghidra debugger to localhost:3333
# TODO: Use the flake's nixGLIntel wrapper instead of 'nix run' once we figure out
#       how to make the overlay work with --impure for host GPU driver access
# Override with: make emu ISO=path/to/game.iso
ISO ?= disks/pal/sles-01090.iso01.iso
ELF := $(BUILD_DIR)/$(PROJECT).elf
GDB_PORT := 3333

# PCSX-Redux executable (via nixGL for GPU access)
# Note: \# escapes the hash which Make treats as comment
PCSX := nix run --impure github:nix-community/nixGL\#nixGLIntel -- pcsx-redux

# Launch PCSX-Redux emulator with GDB server (runs in foreground)
# NOTE: Web server must be enabled manually in GUI:
#   Configuration > Emulation > Enable Web Server
emu:
	$(PCSX) -interpreter -debugger -gdb -iso $(ISO) -run -fastboot

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

# Run a Lua script in PCSX-Redux
# Usage: make lua SCRIPT=scripts/hello.lua
# Note: -interpreter -debugger required for breakpoints to work
lua:
ifndef SCRIPT
	@echo "Usage: make lua SCRIPT=scripts/hello.lua"
	@exit 1
endif
	$(PCSX) -interpreter -debugger -lua_stdout -iso $(ISO) -dofile $(SCRIPT) -run

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
GHIDRA_EXPORT_BACKEND ?= mcp

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

