---
title: "PC Port Plan (TOMB5-modeled): native build of the Skullmonkeys decomp"
category: plans
tags: [plans, port, pc, sdl2, opengl, hal, spec]
---

# PC Port Plan — native build of the Skullmonkeys decomp

**Created:** 2026-07-05
**Status:** Active — Phase 0 COMPLETE; Phase 1 next
**Owner:** port
**Model:** [TOMB5](https://github.com/TOMB5/TOMB5) (decomp-to-PC via a PSY-Q → PC HAL layer)
**Related:** `docs/plans/c-migration-plan.md` (ASM→C roadmap — Phase 2 consumes it),
`docs/plans/bare-include-asm-rule.md`, `docs/systems/rendering-order.md`

> This is a **living checkpoint document**. Each phase has explicit
> checkpoints with acceptance criteria; update the **Progress Log** at the
> bottom as they land. Every port commit MUST keep `make check` (the MIPS
> byte-match) green — the port is additive and invisible to that build.

---

## 1. Context & goal

The repo is a byte-match decompilation of **Skullmonkeys** (PAL, SLES-01090). The
new deliverable is a **native PC port**: compile the decompiled C for PC, replacing
the PlayStation hardware libraries (PSY-Q: libgpu / libgte / libspu / libcd / BIOS)
with PC backends, while keeping the game-logic C unchanged.

**The gating fact.** A native link needs *every* function as compilable C. Today:

| | Count | Notes |
|---|---|---|
| Matched C functions | ~985 | reused as-is by the port |
| Still `INCLUDE_ASM` (raw MIPS) | ~1065 | cannot link natively — must become C or be stubbed |
| Shelved via `maspsx-keep` | 39 | subset of the above |

Even `main`, the frame loop (`src/main.c`), and the entire render core
(`src/gfx.c`: `InitGraphicsSystem`, `SwapBuffersAndClearOT`, `ClearOrderingTables`,
`WaitForVBlankIfNeeded`, `FlushDebugFontAndEndFrame`) are asm-only.

## 2. Confirmed decisions (user, 2026-07-05)

1. **Fork functional-C.** A parallel PC build reuses all matched C as-is and fills
   remaining asm functions with functionally-correct (non-matching) C. The
   byte-match build stays green in parallel; every future match flows into the port.
2. **32-bit ABI** (`-m32`, i686). Pointers stay 4 bytes so game structs, vtables, and
   asset-loaded structs keep their PSX layout with zero struct refactoring. Enforced
   by a `_Static_assert` in `port/include/port_config.h`.
3. **SDL2 + OpenGL** backend. Linux-first, then Windows/macOS.

## 3. Key architectural insight — the two-bucket split

The ~1065 still-asm functions are handled **two different ways**:

1. **Hardware / HAL functions** (`gfx.c` render core, `sound.c` SPU glue, `gamecd.c`
   CD, PAD/BIOS shims, the PSY-Q lib bodies) — do **NOT** faithfully decompile.
   **Replace** them with hand-written PC-native code in `port/spec/` (TOMB5's
   `SPEC_PSXPC` layer). This throws away the hardest-to-match layer entirely.
2. **Game-logic functions** (entities, bosses, menus, physics, level flow) —
   convert to **functional C** with `tools/m2c`, guarded so they compile only in the
   PC build. Bulk-convertible in near-identical families (see `c-migration-plan.md`).

## 4. Portability-blocker inventory (survey 2026-07-05)

Confirmed by a full read of matched `src/*.c`, `src/libs/*.c`, `include/`:

- **No raw MIPS instruction bodies in matched C** (good). GTE instruction macros live
  only in `include/gte_macros.inc`, consumed by `.s` files, not the C.
- **~59 `register T x asm("$N")` pins** (playst.c ×36, enemies.c ×13, vram.c,
  ending.c, menu.c) + **empty `__asm__ volatile("")` scheduling fences** — MIPS-only,
  semantic no-ops. Must be neutralised on x86.
- **`FSM_REG` / `FSM_KEEP_LIVE` / `FSM_RELAY`** (`include/Game/fsm_dispatch.h`) already
  have a behaviour-only mode gated on `FSM_PORTABLE` — the port defines it.
- **753 `asm("D_...")` / `asm("g_...")` linker-alias globals** bind C symbols to
  absolute PSX addresses (`.data`/`.bss`/`.sdata`). Native link needs real backing
  storage for each.
- **Baked absolute RAM addresses in C**: `blb.c:438` `(void*)0x80780000` (HUD base);
  `pickups.c:571` / `entinit.c:996` `0x80E85EA0` (sprite-hash arg — value, harmless).
- **Pointer-in-struct-at-fixed-offset** everywhere (`Entity` @0x18/0x04/0x20/…,
  8-byte `EntityCallbackSlot`/`FSMStateSlot`, `SpriteContext` stores 3 pointers as
  `u32`). All 32/64-*unsafe* → the 32-bit build is load-bearing, not a preference.
- **PSY-Q surface actually called from C** (call counts): GPU `GetTPage`(18)
  `AddPrim`(4) `StoreImage`(3) `DrawSync`(6) `LoadTPage`/`LoadClut`/`SetDispMask`/
  `SetDef{Draw,Disp}Env`; timing `VSync`(8) `VSyncCallback` `Set/GetVideoMode`;
  GTE `InitGeom`(1); SPU `SpuVoiceRegisters`(7) `SpuSetVoicePitch`(4)
  `SpuSet/GetVoiceVolume/Pitch` `SpuSetKey`(3) `SpuSetCommonCDVolume`(3);
  libsnd `SsUtReverbOn`(2); CD `CdControl`(10) `CdReadSync`(5) `CdRead`(3)
  `CdIntToPos`/`CdControlB`(3) + `CdInit/Sync/ControlF/SearchFile/GetSector/PosToInt`
  and the callbacks; PAD `PadInit`/`PadRead`(2) `Start/Init/StopPAD` `ChangeClearPAD`;
  Fnt `FntLoad/Open`(4) `FntPrint`(2) `FntFlush` `SetDumpFnt`(4);
  BIOS `DeliverEvent`(5) `DMACallback`(4) `InterruptCallback` `ResetCallback`(2)
  `Enter/ExitCriticalSection`; libc `rand`(49) `srand` `free`(17) `memcpy`/`memmove`
  `strcmp`(4) `strcpy` `printf`. The core OT present path (`DrawOTag`, `ClearOTag`,
  `PutDrawEnv/DispEnv`, `ResetGraph`) is inside the shelved `gfx.c` asm → **replaced**,
  not called.
- **DMA/IRQ backends still asm** in `src/libs/libcd.c` (`dma_execute`, `startIntrDMA`,
  `StCdInterrupt`) — replaced by `cd_files.c`.

## 5. The coexistence mechanism (keeps `make check` green)

The MIPS build never defines `TARGET_PC` and never sees `port/`, so it is byte-identical.
The PC build differs only through a preprocessor pivot injected by CMake
(`-include port_config.h`, `-DTARGET_PC`):

- **`INCLUDE_ASM(dir, name)` → empty** on PC (`port_config.h` defines it before
  `common.h`/`include_asm.h`, whose `#ifndef` guards then skip the asm-emitting form).
  The symbol is instead satisfied by a **weak stub** (below), or by a real functional-C
  body once written.
- **`_autostubs.c`** (generated): for every asm-function name,
  `__attribute__((weak)) void NAME(void){ port_stub("NAME"); }`. Weak + no prototypes →
  no signature conflicts; a real strong body (matched C, or Phase-2 functional C)
  overrides it automatically. This makes all ~1065 functions link at once.
- **`_autoglobals.c`** (generated): for every `asm("D_…")` alias,
  `__attribute__((weak)) unsigned char storage[SIZE] asm("D_…");` (size from
  `symbol_addrs.txt`; default page when unknown). Backs the 753 absolute-address globals.
- **Register pins / fences**: `FSM_PORTABLE` handles the macro-wrapped family; the ~59
  bare `register … asm("$N")` sites are swept to `PSX_PIN()`/`PSX_FENCE()` macros
  (empty on PC, byte-identical expansion on MIPS) by `tools/portize.py`.
- When a function later **byte-matches**, the matched C is a strong def and the weak
  stub is simply shadowed — the port strictly improves as matching continues, no
  divergence, nothing to hand-sync.

## 6. Repo layout (new — all additive)

```
port/
  CMakeLists.txt          # native 32-bit build; globs src/*.c + port/spec/*.c + generated
  CMakePresets.json       # linux-debug / linux-release / windows / macos
  README.md               # mechanism + how to build
  include/
    port_config.h         # TARGET_PC, 32-bit assert, INCLUDE_ASM/pin/fence neutralisation   [DONE]
    psyq_pc.h             # PC reconstruction of PSY-Q types + prototypes (32-bit layouts)    [DONE]
    LIB{API,GTE,GPU,CD,SPU,ETC,PAD}.H  # thin shims -> psyq_pc.h (satisfy common.h probes)    [partial]
  spec/                   # SPEC_PSXPC layer — hardware REPLACED on PC
    gpu_gl.c              # OT walk + primitive -> OpenGL textured quads; VRAM as GL texture
    gte.c                # software GTE (RotTransPers, matrix ops) — light; game is 2D
    spu_sdl.c            # SPU-ADPCM voice decode/mix -> SDL_AudioStream
    cd_files.c           # CdReadFile/SearchFile/Control -> host filesystem over extracted disc
    pad_sdl.c            # PadInit/PadRead -> SDL gamepad + keyboard (PSX button-mask packing)
    bios.c              # events/interrupts/critical-section/heap/rand/mem*/printf
    port_runtime.c       # port_stub(), panic, logging
    host_main.c          # SDL window + real main(); calls the game boot + drives frames
  decomp/                 # functional-C bodies for still-asm GAME functions (grows in Phase 2)
    _autostubs.c          # GENERATED weak stubs for every asm func (tools/gen_port_stubs.py)
    _autoglobals.c        # GENERATED weak backing storage for asm("D_…") globals
    <segment>/<Func>.c    # hand/m2c bodies, replace the weak stub as they land
tools/
  gen_port_stubs.py       # emits _autostubs.c + _autoglobals.c from asm/ + symbol_addrs.txt
  portize.py              # sweeps bare register-pins/fences to PSX_PIN()/PSX_FENCE()
```

Shared `src/*.c` and `include/` are reused; the only edits to them are the mechanical
pin/fence sweep (byte-neutral on MIPS). `asm/`, `SLES_010.90.yaml/.ld`, and the Makefile
path are untouched.

## 7. Environment prerequisites (flake.nix)

The current `nix develop` shell has **gcc 15 x86-64 only** — no SDL2, no libGL, no 32-bit
multilib. `gcc -m32 -c` (compile-to-object) works today; **linking** needs 32-bit libc/
libgcc + SDL2 + GL. Add to `flake.nix` `buildInputs`:

- 32-bit toolchain: `pkgsi686Linux.gcc` (or `gcc` with `enableMultilib`) + `pkgsi686Linux.glibc`.
- `pkgsi686Linux.SDL2`, `pkgsi686Linux.libGL` (mesa), and `cmake`/`pkg-config`.
- Keep 64-bit MIPS/decomp tools unchanged.

Until this lands, Phase 0 is validated with `gcc -m32 -c` (all TUs must compile to
objects); the link step gates on the flake change.

---

## 8. Phased plan with checkpoints

### Phase 0 — Build skeleton that links (no rendering yet)
Goal: `./skullmonkeys` starts, opens an SDL window, and aborts on the first
unimplemented HAL/game call. Proves the ABI/layout/toolchain path end-to-end.

- **CP-0.1** `port_config.h` + `psyq_pc.h` + `LIB*.H` shims written.  — **DONE**
- **CP-0.2** All 7 `LIB*.H` shims present; a clean matched TU (e.g. `src/main.c`
  accessors) compiles with `gcc -m32 -c -DTARGET_PC -include port_config.h`. — **DONE**
- **CP-0.3** `tools/gen_port_stubs.py` emits `_autostubs.c` + `_autoglobals.c`; they
  compile. — **DONE** (1056 weak func stubs, 1828 weak globals)
- **CP-0.4** `tools/portize.py` sweeps the ~59 bare pins/fences to `PSX_PIN/PSX_FENCE`;
  `make check` still green (byte-identical MIPS expansion verified). — **DONE**
  (49 `register…asm("$N")` pins → `PSX_REG`; 6 `__asm__ volatile("":::"$N")` clobber
  fences → `PSX_CLOBBER`; macros in `include/common.h` gated on `TARGET_PC`)
- **CP-0.5** `port/spec/port_runtime.c` (`port_stub`) + minimal `host_main.c` (SDL
  window). All HAL prototypes have at least aborting stub bodies. — **DONE**
  (`hal_stubs.c` = weak aborting body per PSY-Q call via `#pragma weak`)
- **CP-0.6** `port/CMakeLists.txt` + presets; **every** `src/*.c` + `port/spec/*.c` +
  generated TUs compile to 32-bit objects (no link yet). — **DONE** (48/48 game
  TUs + spec + generated compile `-m32 -c` clean)
- **CP-0.7** flake.nix updated (Phase-7 deps); `cmake --preset linux-debug &&
  cmake --build` **links** an executable that opens a window and aborts on first stub.
  — **DONE.** **Phase 0 exit gate MET** — `./skullmonkeys` opens an SDL2 window +
  OpenGL 2.1 context (320×240 @2×) and aborts on the first stub (`port_game_main`).

### Phase 1 — SPEC layer: the HAL replacements (`port/spec/`)
Goal: booting no longer aborts in the HAL; game reaches its main loop driving real
SDL/GL/audio/input, even if most game logic is still stubbed.

- **CP-1.1 GPU** (`gpu_gl.c`): GL context; VRAM = one 1024×512 16-bit GL texture;
  `LoadImage/StoreImage/MoveImage` = sub-rect uploads; `DrawOTag` walks the OT and
  translates each primitive (`SPRT`, `POLY_FT4`, `TILE`, `LINE_F4`, …) to a textured
  quad; CLUT/TPage select sampling; `DrawSync`/`VSync` frame gate → SDL present.
  Reference: `docs/systems/rendering-order.md` + emulator traces (the real semantics
  are in the *replaced* asm, so RE from docs/traces, not from decompiled C).
- **CP-1.2 GTE** (`gte.c`): software `InitGeom`/`RotTransPers`/matrix ops (light — 2D game).
- **CP-1.3 CD** (`cd_files.c`): map the disc/BLB file tree (`disks/pal/`, `disks/blb/`)
  to a host directory; `CdSearchFile`/`CdRead*` → `fopen`/`fread`. Cross-check
  `src/gamecd.c`, `src/blbacc.c`, `src/lvlload.c`.
- **CP-1.4 SPU** (`spu_sdl.c`): decode SPU-ADPCM voices, mix to a stereo
  `SDL_AudioStream`; `SpuSetKey`/pitch/volume. Cross-check `src/libs/libspu*.c`,
  `libvoice.c`, `src/sound.c`.
- **CP-1.5 PAD** (`pad_sdl.c`): SDL gamepad + keyboard → PSX button-mask format
  (bit layout documented by the cheat table in `src/main.c`).
- **CP-1.6 BIOS/runtime** (`bios.c`): `Enter/ExitCriticalSection`, `InterruptCallback`,
  `VSyncCallback`, `ResetCallback`, heap/`malloc`, `rand/srand`, `mem*`, `printf`;
  frame pacing at 50 Hz PAL in `host_main.c`.
- **CP-1.7** boot sequence (`main_8008E6E0` → … → `InitGameState`) runs without a HAL
  abort. — **Phase 1 exit gate.**

### Phase 2 — Convert the critical-path game functions to functional C
Goal: title screen renders, then level 1 loads and is playable. Convert along the
**boot → frame → render-dispatch spine**, not alphabetically. Use `tools/m2c` +
family batches (`c-migration-plan.md`).

- **CP-2.1** `main` (boot + frame loop) + `gfx.c` OT-flush/buffer-swap glue → C.
- **CP-2.2** level load + asset path: `lvlload.c`, `blb.c`/`blbacc.c`/`blbmem.c`
  remaining stubs, `vram.c`, `sprset.c`, `sprite.c`.
- **CP-2.3** entity tick + render dispatch: `entity.c`, `anim.c`, `entinit.c` stubs.
- **CP-2.4** **Milestone: title screen renders.**
- **CP-2.5** player/HUD/menu enough to play: `player.c`, `hud.c`, `menu.c`, `playst.c`.
- **CP-2.6** **Milestone: level 1 loads and is playable.** — **Phase 2 exit gate.**
- Everything else (bosses, most enemies, effects) stays an `abort()` stub until reached.

### Phase 3 — Playable bring-up + polish
- Iterate title → level 1 → controls → audio against PCSX-Redux as the reference oracle
  (`scripts/game_watcher.lua` JSONL traces = correct state transitions).
- Fill remaining game functions on-demand as aborts surface them.
- Post-MVP niceties (do NOT block MVP): widescreen, hi-res, rebindable input,
  Windows/macOS packaging, save-file mapping.

---

## 9. Risk register

| Risk | Mitigation |
|---|---|
| Struct/asset layout drift at 32-bit | `_Static_assert(sizeof==0x80,…)` size tripwires (Phase 0) vs known PSX offsets; `docs/blb.hexpat` is asset-format truth |
| HAL fidelity (render core is *replaced*, exact semantics only in asm) | RE from `docs/systems/rendering-order.md` + emulator traces; validate primitive-by-primitive vs PCSX-Redux |
| PSY-Q header layout errors (prim packing) | `psyq_pc.h` uses standard PSY-Q 32-bit layouts; verify each prim struct size vs game usage before trusting `gpu_gl.c` |
| `asm("D_…")` global sizing wrong | size from `symbol_addrs.txt`; over-allocate a page when unknown; asserts catch overlap |
| 32-bit multilib friction in Nix | `pkgsi686Linux.*`; fall back to a Docker/Ubuntu-i386 build container if Nix multilib is painful |
| Byte-match regression from pin/fence sweep | sweep expands byte-identically on MIPS; gate every commit on `make check` (never `--no-verify`) |
| Endianness | none — PSX and x86 are both little-endian; asset byte layout is compatible |

## 10. Verification

- **Byte-match unregressed:** `make check` reproduces the ROM SHA1 on every port commit
  (the `#ifdef`/macro pivots are invisible to the MIPS build).
- **Port compiles:** full-tree `gcc -m32 -c` clean (pre-flake); then
  `cmake --preset linux-debug && cmake --build` links.
- **Behavioral parity:** boot the port and the same build in PCSX-Redux side by side;
  compare title, level-1 render, input, audio. `game_watcher.lua` traces are the
  reference for correct state machines.
- **Milestone gates:** (0) links & window → (1) HAL no longer aborts on boot →
  (2) title renders → (2) level 1 playable.

---

## 11. Progress log

- **2026-07-05** — Plan approved (fork functional-C / 32-bit / SDL2+OpenGL). Blocker
  survey completed. Scaffolding started:
  - `port/include/port_config.h` — **DONE** (CP-0.1)
  - `port/include/psyq_pc.h` — **DONE** (CP-0.1)
  - `port/include/LIBGPU.H`, `LIBGTE.H` — done; `LIBAPI/CD/SPU/ETC/PAD.H` — TODO
  - Environment gap identified: no SDL2/GL/32-bit multilib in flake (CP-0.7 dep).
- **2026-07-05 (cont.)** — **Phase 0 COMPLETE.** All checkpoints CP-0.2..CP-0.7
  landed; `./skullmonkeys` opens an SDL2 window + GL2.1 context and aborts on the
  first unimplemented call. Key implementation notes for Phase 1:
  - **All 7 `LIB*.H` shims** present (thin `#include "psyq_pc.h"`).
  - **`psyq_pc.h` exposes TYPES only, no HAL function prototypes** — the decomp
    carries mutually-inconsistent per-TU `extern`s (return/arg-width guesses that
    differ between files), and a header prototype collides under modern GCC's hard
    "conflicting types" error. Each TU's own decl stands; the SPEC layer supplies
    the definitions. Safe because -m32 makes every HAL return 4 bytes (int/long/
    ptr interchangeable in cdecl). `CdlLOC`/`CdlFILE` likewise dropped (only
    `gamecd.c` uses them, with its own local typedef).
  - **Register artifacts:** `tools/portize.py` swept 49 `register…asm("$N")` pins →
    `PSX_REG("$N")` and 6 `__asm__ volatile("":::"$N")` clobber fences →
    `PSX_CLOBBER("$N")`. Macros live in `include/common.h`, gated on `TARGET_PC`
    (empty on PC, byte-identical `asm(...)` on MIPS). `AddPrim` in
    `include/functions.h` was the only game/HAL prototype needing reconciliation.
  - **`gen_port_stubs.py`:** backs **every** `symbol_addrs.txt` RAM symbol
    (addr ≥ 0x80000000) ∪ `asm("D_…")` src aliases as WEAK storage (1828), and
    every asm-only function as a WEAK `void f(void){port_stub}` (1056). libc names
    (`memcpy/printf/rand/str*/…`) and the PSX `main` are excluded (bind to host
    libc / `host_main`). Weak everywhere → any strong matched-C def wins.
  - **`hal_stubs.c`** uses `#pragma weak` on the entire PSY-Q surface, so the 14
    HAL functions already matched in C (`libcd.c`/`libspu*`/`libvoice.c`:
    `ResetCallback`, `PadRead`, `DMACallback`, `VSyncCallback`, Spu*, Cd*, …)
    override the abort stubs with **no hand-maintained exclusion list**.
  - **CMake flags that matter:** `-m32 -fcommon` (the engine's multi-TU gp-rel
    tentative `T x asm("D_..")` globals need `-fcommon`; modern GCC defaults to
    `-fno-common` → duplicate strong defs) + the decomp-idiom `-Wno-*` demotions.
  - **flake.nix:** added `gcc_multi` (full `-m32` compile+link), `pkgsi686Linux.{SDL2,
    libGL,glibc.dev}`, and a **`port-cc`** wrapper (→ `gcc_multi`) that
    `CMakePresets.json` uses as `CMAKE_C_COMPILER` (the devShell stdenv gcc-wrapper
    only does the `-m32` *compile*, not the link). SDL2 is discovered via
    pkg-config using the flake-exported `PORT_PKG_CONFIG_PATH` (32-bit `.pc` dir).
    NOTE: a **fresh `nix develop`** is required to put `port-cc` on PATH.
  - **`host_main.c`** gates SDL strictly on the CMake-set `PORT_HAVE_SDL2` (not
    `__has_include`) so compile and link stay in sync; windowless fallback links
    and still exercises the HAL-abort path.
- _next (Phase 1):_ implement the SPEC/HAL backends in `port/spec/` — start with
  `gpu_gl.c` (OT walk → GL quads) and `bios.c` (critical-section/rand/mem*),
  guided by `docs/systems/rendering-order.md` + emulator traces; then wire
  `port_game_main` (Phase 2) to the converted PSX `main`.
