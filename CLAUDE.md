# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Decompilation of **Skullmonkeys** (PAL, SLES-01090) for PlayStation 1. The goal is a byte-matching reproduction of the original binary from C source. Expected SHA1 of `build/SLES_010.90.bin` is in `SLES_010.90.yaml`.

**This is a clean-room reverse engineering project.** The code is not derived from any existing codebase — no original source, symbols, or debug info exist. Every symbol name (functions, globals, struct fields) is a guess inferred from behavior, and documentation (docs/, Ghidra plate comments, this file) can be incorrect even when it claims to be verified. Always double-check claims against the actual code/disassembly (and runtime traces where possible) before making any claims or decisions based on a name or doc.

## Environment

The dev environment is provided by Nix (`flake.nix`). Enter it with `nix develop` (or `direnv allow`). It provides the MIPS cross-toolchain (`mipsel-unknown-linux-gnu-*`), Python venv, PCSX-Redux (wrapped via nixGL), Ghidra with the PSX loader, `luacheck`, plus helpers `ram2off` / `off2ram`. The shell hook auto-creates `.venv` and pip-installs `tools/requirements-python.txt` and `pyghidra`.

The PSX-era compiler (`bin/cc1-psx-26`) is not in Nix — run `./tools/download-toolchain.sh` once. PSY-Q headers belong in `psyq/` and are not committed.

## Common Commands

```bash
# Build (auto-runs splat if asm/ is missing) and verify SHA1 match
make            # or: make all
make check      # build + SHA1 verification

# Re-extract from base ROM with splat
make extract    # uses SLES_010.90.yaml -> writes asm/, *.ld, undefined_*.txt

# Per-function decomp workflow
make context                  # regenerate ctx.c (preprocessed common.h) for m2c
make decompile FUNC=FuncName  # m2c on asm/nonmatchings/**/FuncName.s
make diff FUNC=FuncName       # asm-differ side-by-side (-mw3)
make permuter-import FILE=src/X.c FUNC=FuncName
make permuter FUNC=FuncName

# Lint
make lint            # luacheck on scripts/*.lua
make check-lua       # luacheck + luac -p syntax check

# Emulator / runtime tracing
make emu                              # PCSX-Redux + GDB on :3333 (enable Web Server in GUI)
make record LEVEL=1 STAGE=0           # boot directly into a level with game_watcher.lua attached
make dump-gamestate LEVEL=1 STAGE=0   # dump raw GameState
make snapshot                         # RAM dump via web API -> /tmp/skullmonkeys_snapshot.json
make lua SCRIPT=scripts/foo.lua       # run an arbitrary Lua script in PCSX-Redux

# MCP / Ghidra
make mcp-server                       # PCSX-Redux MCP bridge (needs Web Server on :8080)
make ghidra-export                    # PyGhidra symbol export (needs GHIDRA_INSTALL_DIR)
make ghidra-export-all                # writes symbol_addrs_new.txt for manual merge

make clean        # nukes build/, asm/, assets/
make clean-build  # keeps extracted asm/, only removes build/
```

## Build Pipeline

1. **splat** (`SLES_010.90.yaml`) splits the base ROM at `bin/SLES_010.90` into `asm/`, generates `SLES_010.90.ld`, and writes `undefined_funcs_auto.txt` / `undefined_syms_auto.txt`. Configured options of note: `gp_value: 0x800A5954`, `migrate_rodata_to_functions: false`, `find_file_boundaries: true`, `ld_legacy_generation: true` (preserves YAML segment order).
2. **Compile**: `cpp` → `tools/gcc-2.7.2-psx/cc1` (`-O2 -G8 -fno-builtin -mno-abicalls -mcpu=3000`) → `tools/maspsx/maspsx.py --aspsx-version=2.86 --run-assembler` → `mipsel-unknown-linux-gnu-as`. `maspsx` replicates ASPSX.EXE quirks (gp-relative addressing, hazard NOPs, div/rem expansion).
3. **Link**: `mipsel-unknown-linux-gnu-ld -nostdlib -T SLES_010.90.ld -T undefined_syms_auto.txt -T undefined_funcs_auto.txt`. Object paths must match those in the splat-generated linker script (`build/asm/...o`, `build/src/...o`).
4. **Pack**: `objcopy -O binary` → `build/SLES_010.90.bin`. `make check` SHA1's it against the value in the splat YAML.

Switching compilers (e.g. 2.5.7-psx) may improve some functions but break others — there is no per-file selection yet.

## Repo Layout (essentials)

- `src/` — decompiled C, flat layout (Glover convention): `gfx.c`, `entity.c`, `anim.c`, `player.c`, `clayball.c`, `vehicle.c`, `lvlload.c`, `entinit.c`, `main.c`, etc. PSY-Q libraries live under `src/libs/` (`libcd.c`, `libvoice.c`). Non-matching functions stub out via `INCLUDE_ASM(...)` referencing `asm/nonmatchings/<segment>/<func>.s`.
- `include/` — `common.h` (base types, m2c context source), `Game/`, `globals.h`, `macro.inc`, `labels.inc`, `gte_macros.inc`. **Don't modify unless instructed**; `common.h` is the source for `ctx.c`.
- `asm/` — splat output; **never edit by hand**. `asm/nonmatchings/` holds per-function asm pulled in via `INCLUDE_ASM`.
- `SLES_010.90.yaml` / `SLES_010.90.ld` — splat config and generated linker script.
- `symbol_addrs.txt` — known symbols (manually curated; merge new exports from `symbol_addrs_new.txt`).
- `tools/` — vendored: `gcc-2.7.2-psx`, `maspsx`, `m2c`, `asm-differ`, `decomp-permuter`, `mipsmatch`, `splat_ext`, `scripts/pyghidra_export_symbols.py`.
- `scripts/` — runtime tooling: `game_watcher.lua`, `gamestate_dumper.lua`, `pcsx_mcp_server.py`, `ram_snapshot.py`, asset extractors, `blb.hexpat`, `addr2offset.sh`.
- `disks/pal/` — base ISO (`sles-01090.iso01.iso`); `disks/blb/` — extracted BLB asset files.
- `docs/` — extensive RE notes. Start at `docs/README.md`, `docs/decompilation-guide.md`, `docs/blb/README.md`, `docs/SYSTEMS_INDEX.md`. Per-system docs live under `docs/systems/` (player, collision, animation, bosses, enemies, audio, etc.); reference tables under `docs/reference/`; in-flight research under `docs/analysis/`.

## Address / Format Conventions

- RAM base `0x80010000`; ROM offset = VRAM − `0x80010000` + `0x800`. Use `ram2off`/`off2ram` (in shell) or `./scripts/addr2offset.sh`.
- `sdata` starts at `0x800A5954` (matches splat `gp_value`). `-G8` keeps ≤8-byte symbols GP-relative.
- BLB asset format: **`docs/blb.hexpat` (ImHex) is the source of truth.** Document confirmed findings in `docs/blb/`; unconfirmed observations go in `docs/analysis/unconfirmed-findings.md` until verified. Confirmed findings should also be applied to Ghidra via the MCP tools when available.

## Runtime Verification (PCSX-Redux)

PCSX-Redux exposes a Web Server (must be enabled manually: Configuration → Emulation → Enable Web Server) and a Lua API. `scripts/pcsx_mcp_server.py` bridges the web API as MCP tools (`read_ram`, `read_u8/16/32`, `read_string`, `write_ram`, `read_vram`, `pause`/`resume`/`reset`, `read_disc_file`, `patch_disc_file`, `upload_symbols`).

`scripts/game_watcher.lua` hooks key engine functions (e.g. `PlayerTickCallback @0x80059E10`, `EntitySetState @0x8001EAAC`, `SetEntitySpriteId @0x8001D080`, `DispatchEventToCollidingEntity @0x800226F8` (formerly `CheckEntityCollision`), `LoadLevelFromBLB @0x8007E474`) and streams JSONL traces to `game_watcher/logs/trace_*.jsonl`. Configure sampling/throttling/boot-override at the top of the script. Use traces to verify decomp output, derive physics constants, and document state machines (see `docs/systems/player/`).

## Decompilation Conventions

- Don't hand-edit anything in `asm/`.
- New symbol names from Ghidra: export to `symbol_addrs_new.txt` via `make ghidra-export-all`, then manually merge into `symbol_addrs.txt`.
- Splat rodata-warnings ("Rodata segment X may belong to text segment Y"): identify the owning subsystem (PSY-Q libs → `LIBS` segment; game code → its named segment) and either rename or extend the rodata segment in `SLES_010.90.yaml` so its name matches the corresponding text segment. Rebuild to confirm.
- Compiler flags are fixed; do not tweak them per-file without coordinating — the byte-match depends on them.
