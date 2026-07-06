---
title: "PC Port Plan (TOMB5-modeled): native build of the Skullmonkeys decomp"
category: plans
tags: [plans, port, pc, sdl2, opengl, hal, spec]
---

# PC Port Plan — native build of the Skullmonkeys decomp

**Created:** 2026-07-05
**Status:** Active — Phase 0 + Phase 1 HAL COMPLETE; Phase 2 IN PROGRESS (CP-2.1/2.2 done; **CP-2.4 MILESTONE HIT: the title screen renders correctly** — full animated logo, menu, cursor; next: emulator pixel-diff + CP-2.5 menu→level-1)
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
  — **DONE (plumbing).** VRAM texture + transfers, exact `GetTPage`/`GetClut`,
  `AddPrim`/`ClearOTag`/`DrawOTag` (PC-native **32-bit tag** scheme — host pointers
  are 4 bytes under -m32, so the full pointer lives in the primitive `tag` word,
  sidestepping the PSX 24-bit-pointer limit), decode dispatch (SPRT/POLY_FT4/GT4/
  TILE/POLY_F4/DR_TPAGE → GL quads), `Set*` initialisers, env/`DrawSync`, minimal
  Fnt. Synthetic 3-primitive OT self-test passes. CLUT/4-bit-texture pixel-exact
  sampling is refined in Phase 2 against emulator traces (needs live game frames).
- **CP-1.2 GTE** (`gte.c`): software `InitGeom`/`RotTransPers`/matrix ops (light — 2D game).
  — **DONE.**
- **CP-1.3 CD** (`cd_files.c`): the game streams **all** data from **GAME.BLB** via
  absolute-sector reads (`CdIntToPos`→`CdControl(Setloc)`→`CdRead`→`CdReadSync`). The
  backend opens `disks/blb/GAME.BLB` **directly** (NOT the ISO) and treats it as a
  flat 2048-byte sector device (sector 0 = start of GAME.BLB; PC boot sets
  `BLB_BASE_SECTOR = 0`). Full `Cd*` surface + BCD MSF↔LBA. The game's BLB parser
  (`blbacc/blbmem/lvlload`) runs unchanged on the fed bytes. Verified: reads sector 0,
  magic `c9002000`. CD-audio commands accepted/no-op'd. — **DONE.**
- **CP-1.4 SPU** (`spu_sdl.c`): the game-facing voice-control surface
  (`SpuSetKey`/`SpuSetVoicePitch`/`SpuGetVoicePitch`/`SpuSetVoiceVolume`/
  `SpuSetCommonCDVolume`/`SpuQuit`/`SsUtReverbOn`) is implemented as per-voice state;
  ADPCM decode + mix to `SDL_AudioStream` lands in Phase 2 when the game keys voices.
  — **DONE (surface).**
- **CP-1.5 PAD** (`pad_sdl.c`): SDL keyboard **and** game controller → PSX 16-bit
  button mask; `PadRead` returns it (post-invert convention matching
  `src/libs/libcd.c PadRead`). — **DONE.**
- **CP-1.6 BIOS/runtime** (`bios.c`): `Enter/ExitCriticalSection`, `DeliverEvent`,
  `InterruptCallback`/`VSyncCallback`/`ResetCallback`/`DMACallback` registry, `VSync`
  counter; frame pacing at 50 Hz PAL in `port_boot.c`. — **DONE.**
- **CP-1.7** boot sequence (`main_8008E6E0` → … → `InitGameState`) runs without a HAL
  abort. — **Phase 1 exit gate. HAL READY**: a bring-up harness (`port_boot.c`) drives
  the full HAL in a live 50 Hz loop (clear → present → input → vsync-callback → pace)
  with GPU/CD/pad self-tests passing; the *game-driven* boot lands with Phase 2 (it
  needs the converted PSX `main`). Architecture decision: `src/libs/*.c` (the matched
  PSY-Q lib C) is **excluded** from the port build — it dereferences PSX kernel/HW
  globals that don't exist on PC — and the whole PSY-Q surface is provided by
  `port/spec/` instead (42 game-called HAL symbols verified strong, none stubbed).

### Phase 2 — Convert the critical-path game functions to functional C
Goal: title screen renders, then level 1 loads and is playable. Convert along the
**boot → frame → render-dispatch spine**, not alphabetically. Use `tools/m2c` +
family batches (`c-migration-plan.md`).

- **CP-2.1** `main` (boot + frame loop) + `gfx.c` OT-flush/buffer-swap glue → C.
  — **DONE (boot spine).** The PSX `main` boot sequence + frame body are
  reconstructed as functional C in `port/decomp/boot/game_boot.c`
  (`port_game_boot_init` / `port_game_boot_frame`, from the working
  `docs/analysis/decomp-drafts/main.c`), driven by `port_boot.c`'s
  `port_game_main` (heap + HAL bring-up → boot init → frame loop with SDL
  present/input/50 Hz pacing). Supporting pieces landed: `port/spec/port_heap.c`
  (mallocs the 16 MB BLB heap, sets `g_pBlbHeapBase`); trivial gfx helpers
  `WaitForVBlankIfNeeded` + `FlushDebugFontAndEndFrame`
  (`port/decomp/gfx/gfx_port.c`) and `initPlayerState`
  (`port/decomp/blb/initPlayerState.c`); and the CD-model `LoadGameAssetLocations`
  **replacement** (`port/decomp/gamecd/`, sets `BLB_BASE_SECTOR = 0` for the
  direct-GAME.BLB backend rather than doing ISO file searches). CMake `decomp/`
  glob is now recursive; `gen_port_stubs.py` also scans `port/decomp/` so
  converted bodies' `asm("D_…")` globals auto-get weak backing. **The boot now
  runs live** through `SsUtReverbOn → ResetCallback → LoadGameAssetLocations` and
  aborts (cleanly, by name) at the first still-asm callee — establishing the
  iterative work queue. Two boot modes: default (game boot) and
  `PORT_HAL_HARNESS=1` (HAL-only smoke loop).
  - **NEXT (render core):** `InitGraphicsSystem` is the first abort. It sets up a
    ~20-field **double-buffered GPU-context struct** in the BLB heap (screen w/h
    @ base+0x0/0x2 = 0x140/0x100; `SetDefDrawEnv` @ base+0x4 & base+0x5044;
    `SetDefDispEnv` @ base+0x60 & base+0x50A0; OT + prim-pool counts @
    base+0xA084→; installs the VSync-ISR swap via `VSyncCallback`) and depends on
    **`InitVRAMSlotTable`** and **`SwapBuffersAndClearOT`** (the buffer-swap that
    calls `DrawOTag` — the render-present integration point). This is the render
    architecture core: it needs the GPU-context struct layout verified against
    Ghidra/`prim.c`/emulator traces, and a deliberate decision on how the game's
    OT + double-buffer envs map onto the `gpu_gl.c` present path (candidate: PC
    `SwapBuffersAndClearOT` = `DrawOTag(game_ot)` + `port_gpu_present()` +
    `ClearOTag`, ignoring the PSX ISR-driven double-buffer). Do this deliberately,
    not as an end-of-context transcription.
- **CP-2.2** level load + asset path: `lvlload.c`, `blb.c`/`blbacc.c`/`blbmem.c`
  remaining stubs, `vram.c`, `sprset.c`, `sprite.c`.
  — **IN PROGRESS.** The render core landed (`port/decomp/gfx/render_core.c`:
  `InitGraphicsSystem`, `SwapBuffersAndClearOT`, `ClearOrderingTables`,
  `GetFrameReadyFlag`, `TriggerBufferSwapIfReady`; `port/decomp/vram/
  InitVRAMSlotTable.c`). The **double-buffered GPU context** is set up in the BLB
  heap (buffers at base+0x4 / base+0x5044, ot ptr @ +0x70, prim pools @ +0x74);
  `SwapBuffersAndClearOT` = the present (`DrawOTag(gpu->ot)` → GL, toggle buffer,
  `ClearOTag`, reset pool counts), fired by the C `TriggerBufferSwapIfReady` VSync
  ISR that `port_boot.c` runs each host frame. **KEY FIX:** `GetFrameReadyFlag` is
  a dual-entry function — its *entry* returns `&D_800AE3E0` (a scratch buffer
  pointer), only `+0xC` returns the `g_FrameReady` byte (the gfx.c plate comment
  described only the latter). `InitGraphicsSystem` uses the entry to set the
  BLB-header read buffer at heap+0xA650. Then the level-load spine converted:
  `InitGameState` (`port/decomp/gstate/`: sets initial game-mode dispatch +
  builds the sub-level asset list + calls the loaders), `LoadBLBHeader`
  (`port/decomp/blb/`: reads the first 2 GAME.BLB header sectors via the CD
  backend — **reads real GAME.BLB data** — then `InitLevelDataContext` +
  `SetSpriteTables`, both matched C), `InitSPUDefaults` (no-op; SPU config
  subsumed by the spu_sdl.c state model). The boot now runs the **entire**
  power-on → HAL → graphics-init → level-load-entry path and reaches
  **`InitializeAndLoadLevel`** (`lvlload.c`, 460 lines, ~40 callees) — the full
  level asset/tile/sprite/entity-spawn subsystem, the next major work unit
  (deserves a focused session + emulator runtime verification as tiles/sprites
  upload to VRAM).
  - **BLB heap allocator DONE + unit-tested** (`port/decomp/vram/heap_alloc.c`:
    `InitHeapFreeList`, `ClearHeapBlocks`, `AllocateFromHeap`, `FreeFromHeap`).
    This is the block-descriptor allocator the level loader carves every asset
    buffer from (`InitHeapConfig`→`InitHeapFreeList`→`AllocateFromHeap`→
    `ClearHeapBlocks` all appear on `InitializeAndLoadLevel`'s critical path).
    Free-list = 100 descriptors at heap+0xA320 (`{s32 ptr; u16 size16; u16 next}`);
    alloc reserves a 4-byte size header at ptr[0], returns ptr+4 (m2c mistyped the
    pointer as +16 — verified `+0x4` bytes against the asm). Verified with
    `tools/test_heap.c` (-m32): non-overlapping allocations, no aliasing, free
    restores the free count, coalesce works — **ALL PASS**. Heap corruption is
    silent, so this was tested standalone before being relied upon.
  - **`InitializeAndLoadLevel` structure fully mapped** (see repo memory
    `pc-port.md` for the offset-level breakdown). It's a movie/loading-screen
    playback state machine (switch on `AdvancePlaybackSequence`→0-5 via
    `jtbl_80012140`, so m2c can't auto-decompile it) whose tail does the real
    asset→heap→VRAM load. ~18 still-asm callees remain (LevelDataParser,
    LoadAssetContainer, LoadTileDataToVRAM, InitLayersAndTileState,
    LoadEntitiesFromAsset501, the movie/loading-screen functions, …); ~12 more are
    already matched C. Next-session approach: hand-write the driver from the .s
    (jump table needed), no-op the movie/loading-screen path initially to reach
    the tile/sprite load faster, and verify VRAM uploads against PCSX-Redux
    (visible tiles = CP-2.4 "title renders").
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
- **2026-07-05 (cont.)** — **Phase 1 HAL backends COMPLETE.** All six SPEC backends
  provide real (non-abort) behavior; 42 game-called HAL symbols verified strong.
  Key decisions & findings:
  - **Excluded `src/libs/*.c`** from the port build (the matched PSY-Q lib C
    dereferences PSX kernel/HW globals — `BIOS_CALLBACK_TABLES`, `SPU_REGISTER_BASE`
    — that are weak-backed zero on PC → would null-crash). The whole PSY-Q surface
    is provided by `port/spec/` instead, per the plan's "replace the lib bodies".
    Used the linker as the exhaustive work-list (exclude → link → fill → repeat).
  - **CD = direct GAME.BLB, not ISO** (user directive). `cd_files.c` opens
    `disks/blb/GAME.BLB` and serves it as a flat 2048-byte sector device; the game's
    `CdIntToPos→CdControl(Setloc)→CdRead→CdReadSync` sequence reads it unchanged.
    PC boot must set `BLB_BASE_SECTOR = 0` (it's weak-backed 0 by default; the
    Phase-2 `LoadGameAssetLocations` replacement keeps it 0). Other regions' BLBs
    work too (soft magic check).
  - **GPU OT 24-bit-pointer problem SOLVED by -m32**: host pointers are 4 bytes, so
    the full pointer is stored in the primitive's 4-byte `tag` word (the `code` byte
    is a separate word). `AddPrim`/`ClearOTag`/`DrawOTag` agree on this; no low-16MB
    arena needed. `GetTPage`/`GetClut` use exact PSX bit-packing. Synthetic
    3-primitive OT self-test (`PORT_GPU_SELFTEST=1`) walks + renders cleanly.
  - **`hal_stubs.c` `#pragma weak` design paid off**: each real backend definition
    (gpu_gl/gte/spu_sdl/cd_files/pad_sdl/bios) shadows the abort stub automatically,
    no exclusion lists to maintain.
  - **`port_boot.c`** is the HAL bring-up harness driving a live 50 Hz loop
    (`PORT_MAX_FRAMES=N` to auto-exit headless; `PORT_GPU_SELFTEST=1` to draw a
    synthetic OT). It's also the skeleton the Phase-2 converted `main` grows from.
  - Deferred to Phase 2 (they need live game data to validate): pixel-exact GPU
    CLUT/4-bit texture sampling, and SPU-ADPCM voice decode/mix. Both co-develop
    with the render/audio bring-up against PCSX-Redux traces.
- _next (Phase 2):_ convert the boot→frame→render spine to functional C
  (`main`, `gfx.c` OT-flush/swap glue), wire `port_game_main` → the converted boot,
  then iterate the GPU/SPU pixel/audio fidelity against the emulator oracle.
- **2026-07-05 (cont.)** — **Phase 2 CP-2.1 COMPLETE (boot spine).** The PSX `main`
  boot + frame body are functional C (`port/decomp/boot/game_boot.c`), driven by
  `port_game_main`; the port boots live through the HAL + heap + `SsUtReverbOn` +
  `ResetCallback` + `LoadGameAssetLocations` and aborts by name at the first
  still-asm callee (`InitGraphicsSystem`). Notes:
  - **Iterative abort-driven conversion loop is live**: run → read the PANIC name →
    convert that function under `port/decomp/<seg>/<Func>.c` → rebuild → repeat.
    Each converted body is a strong def that shadows its weak stub automatically.
  - **`port/decomp/` is now recursive** (CMake `GLOB_RECURSE`) and
    `gen_port_stubs.py` scans it, so converted bodies' `asm("D_…")` globals get
    weak backing without touching symbol_addrs.txt.
  - **CD-model replacements**: functions that do ISO/CD lookups (e.g.
    `LoadGameAssetLocations`) are *replaced* (set `BLB_BASE_SECTOR = 0`), not
    faithfully decompiled — the direct-GAME.BLB backend has no ISO.
  - **Heap**: `port_heap.c` mallocs 16 MB and sets `g_pBlbHeapBase` (PSX retail is
    2 MB; oversized while struct sizes are still being confirmed). GameState weak
    backing is exactly 0x1A0 (correct).
  - **Render-core gate**: `InitGraphicsSystem` + `InitVRAMSlotTable` +
    `SwapBuffersAndClearOT` are the next chunk and constitute the render
    architecture — deferred to a focused session (needs GPU-context struct layout
    verified vs Ghidra/prim.c/traces + a deliberate OT→present mapping). See the
    CP-2.1 "NEXT (render core)" note above.
- _next (Phase 2 cont.):_ design + convert the render core (InitGraphicsSystem /
  SwapBuffersAndClearOT / InitVRAMSlotTable), then InitGameState + the level/asset/
  sprite/entity render chain, driving toward CP-2.4 "title screen renders".
- **2026-07-05 (cont.)** — **Phase 2 CP-2.2 render core + level-load spine.** The
  boot now runs the *entire* power-on → HAL → graphics-init → level-load-entry path
  and reaches `InitializeAndLoadLevel` (the level asset loader). Converted:
  `render_core.c` (InitGraphicsSystem / SwapBuffersAndClearOT / ClearOrderingTables
  / GetFrameReadyFlag / the C VSync ISR TriggerBufferSwapIfReady),
  `vram/InitVRAMSlotTable.c` (minimal), `gstate/InitGameState.c` (from m2c),
  `blb/LoadBLBHeader.c` (reads real GAME.BLB header via the CD backend),
  `sound/InitSPUDefaults.c` (no-op). Notes:
  - **Double-buffered GPU context** mapped onto gpu_gl.c: buffers at heap+0x4 /
    heap+0x5044, `ot` ptr @ +0x70 → a static per-buffer list head; the game's
    AddPrim links into it, SwapBuffersAndClearOT's DrawOTag walks it (the present).
    Wired the VSync ISR to fire once per host frame between clear and present.
  - **GetFrameReadyFlag dual-entry gotcha**: the function *entry* returns a scratch
    BUFFER pointer (`&D_800AE3E0`), NOT the g_FrameReady byte the gfx.c plate
    comment describes (that's the +0xC entry). InitGraphicsSystem uses the buffer
    ptr for the BLB-header read destination. Clean-room docs can be wrong — verified
    against the .s.
  - **Global-backing rule for converted bodies**: reference absolute-address game
    globals via `extern T x[] asm("D_xxxx");` (asm alias) so gen_port_stubs.py's
    scanner backs them weakly; a plain `extern T D_xxxx;` link-fails.
  - **Deferred bug** (documented in repo memory): BLB_ReadSectorsWrapper passes only
    2 of CdBLB_ReadSectors' 3 args (the buffer is implicit on MIPS); on PC cdecl the
    buffer arg is garbage — fix when the asset loader first uses the callback.
- _next (Phase 2 cont., focused session):_ `InitializeAndLoadLevel` (lvlload.c, 460
  lines, ~40 callees) — the full level asset/tile/sprite/entity-spawn subsystem +
  the VRAM atlas allocator, converted with **emulator runtime verification** as
  tiles/sprites upload to the VRAM texture (visible progress), driving to CP-2.4
  "title screen renders".
- **2026-07-05 (cont.)** — **BLB heap allocator DONE + unit-tested.** Converted
  `InitHeapFreeList`, `ClearHeapBlocks`, `AllocateFromHeap`, `FreeFromHeap`
  (`port/decomp/vram/heap_alloc.c`) — the block-descriptor allocator every level
  asset buffer is carved from, directly on `InitializeAndLoadLevel`'s path. Notes:
  - **Verified standalone** (`tools/test_heap.c`, -m32): non-overlapping
    allocations, no pattern aliasing, free restores the free count, coalesce lets a
    full re-allocation succeed — ALL PASS. Heap corruption is silent, so it was
    tested before being relied on rather than "run the boot and hope".
  - **m2c pointer-typing trap**: m2c rendered the payload offset as `u32* + 4`
    (= +16 bytes); the asm is `addiu +0x4` (= +4 bytes). The size header is a single
    4-byte word at ptr[0], payload = ptr+4. Verified against the .s — this is the
    exact "silent corruption" risk the plan warns about, caught by reading the asm.
  - **`InitializeAndLoadLevel` fully structure-mapped** (offset-level breakdown in
    repo memory): a movie/loading-screen playback state machine (switch on
    `AdvancePlaybackSequence`→0-5 via `jtbl_80012140`, so m2c can't auto-decompile
    it) whose tail runs the asset→heap→VRAM load. ~18 still-asm callees remain,
    ~12 are matched C. Boot still cleanly reaches its abort.
- _next:_ hand-write `InitializeAndLoadLevel` from the .s (jump table needed);
  no-op the movie/loading-screen path initially to reach the tile/sprite load, and
  verify VRAM uploads against PCSX-Redux (CP-2.4 title renders).
- **2026-07-05 (cont.)** — **`InitializeAndLoadLevel` DONE; boot reaches
  `LevelDataParser`.** The 460-line level loader is converted
  (`port/decomp/lvlload/InitializeAndLoadLevel.c`) from a full m2c draft — the key
  was feeding m2c the jump table: `make decompile` only passes the function .s, so
  run m2c manually with **both** `InitializeAndLoadLevel.s` and
  `asm/data/lvlload.rodata.s` (which holds `jtbl_80012140`). Also converted:
  `RemoveFromZOrderList` + `ClearEntityDefList` (`port/decomp/blb/entity_lists.c`),
  `InitializePlayerState` (byte-identical to the earlier `initPlayerState`), and
  no-op movie/loading functions (`port/decomp/lvlload/movie_stub.c`:
  `DisplayLoadingScreen`/`PlayMovieFromCD`/`PlayMovieFromBLBSectors` — the real ones
  are 181/289/280-line STR/MDEC subsystems, deferred to Phase 3). Notes:
  - **Movie no-op strategy validated at runtime**: `DisplayLoadingScreen`→0
    ("loading done") takes the level-select "ready" branch; `PlayMovie*`→0 advances
    the sequence. The playback loop **terminates cleanly** at seq type 3/6 (no
    infinite loop) and reaches the asset-load tail — confirmed by the boot advancing
    past it to `LevelDataParser`.
  - **PC memory-model fix**: the tail's `InitHeapConfig(heap, levelBuf+off,
    0x801FC000 - (levelBuf+off))` assumes PSX's flat 2 MB RAM; `InitHeapConfig`
    clamps the garbage-on-PC size to `0xFFFF0` (~1 MB), so the level-data staging
    buffer `D_800AE3E0` was enlarged to **4 MB** in `render_core.c` to hold that
    sub-heap. Fragile (watch for overruns once the asset loaders fill it) but
    correct for now.
- _next (focused session):_ `LevelDataParser` (168 lines) + `LoadAssetContainer`
  (262 lines, recursive) — the BLB asset-container decoder. Needs the BLB level-data
  format (`docs/blb.hexpat` = source of truth) + the `LevelDataContext` layout, with
  field offsets verified against the format spec + emulator runtime (silent-
  corruption risk). These unlock the tile/sprite asset load → `LoadTileDataToVRAM`
  (VRAM atlas) → the tile/color/entity/layer init chain → CP-2.4 "title renders".
- **2026-07-05 (cont.)** — **BLB asset decoder DONE; boot reads real GAME.BLB data
  and reaches `LoadTileDataToVRAM`.** Converted `LevelDataParser`
  (`port/decomp/level/`) + `LoadAssetContainer` (`port/decomp/blbacc/`, recursive) —
  the BLB asset-container decoder — plus a `UploadAudioToSPU` no-op
  (`port/decomp/sound/sound_stub.c`). The boot now runs the **entire asset-load
  path**: it reads actual level asset data out of GAME.BLB via the CD backend,
  parses every asset container, and reaches the tile→VRAM upload. Notes:
  - **The `LevelDataContext` struct is fully named** in
    `include/Game/level_data_context.h` (0x80 bytes) with the complete asset-ID→field
    map in its header comment — used directly rather than guessing offsets, and
    cross-checked against the `.s` switch.
  - **Callback bug fixed**: the decomp's `BLB_ReadSectorsWrapper` is a MIPS-only
    2-arg trampoline relying on `$a2=dst` register passthrough (breaks on PC cdecl).
    `LoadBLBHeader` now installs a proper 3-arg `port_blb_read_sectors(sector,count,
    dst)` as `LevelDataContext.sector_read_callback` (+0x64).
  - **Record format asm-verified**: container `base[0]` = record count (u32 in
    `LoadAssetContainer`, **u16** in `LevelDataParser` — a real difference), then
    records at stride 0xC `{type @+4, size @+8, dataOffset @+0xC}`, `dataPtr = base +
    dataOffset`. Header directory: `dir = blb + seq_index`; `type = dir[0xF36]`,
    `subIdx = dir[0xF92]`; type-3 entry at `blb + subIdx*0x70`. All transcribed from
    the `.s`, not m2c's struct guesses.
  - Reached `LoadTileDataToVRAM` **without crashing** through the real BLB data —
    strong evidence the parsed asset pointers are valid.
- _next (focused session):_ `LoadTileDataToVRAM` (242 lines) + the VRAM atlas
  allocator (`AllocateVRAMSlot`/`UploadTextureOrClut`/`InitVRAMSlotArray`) — the tile-
  graphics decode + VRAM texture upload = **visible output**. `LoadImage` (HAL) already
  uploads to the `gpu_gl.c` VRAM texture, so this is the CP-2.4 "title renders" push
  and the point where **PCSX-Redux runtime comparison** matters (do tiles land at the
  right VRAM coords?). Convert it + the slot allocator as a unit, then run and diff
  the first frame vs the emulator.

- **2026-07-05 (session 2) — CP-2.2/2.4: the ENTIRE boot→level-load→render-setup→
  player-spawn pipeline now runs.** From the `LoadTileDataToVRAM` abort the boot was
  driven all the way through `InitGameState`'s completion; it now reaches the
  title-screen menu-entity creation (`InitMenuEntity`, `passwd/`). ~22 functions
  converted this session (all under `port/decomp/`, MIPS byte-match build untouched).
  - **Boot-progression bug fixed (NULL .sdata-pointer class):** many `asm("D_xxxx")`
    globals are POINTERS whose PSX `.sdata` init targets a `.bss` struct. Weak-zero
    backing makes them NULL, so guarded inits no-op and later `PTR[field]=x` writes
    crash. `D_800A597C` (player state) was NULL → `initPlayerState` (`if(!p)return;`)
    never ran and `InitGameState` deref'd NULL. Fixed by seeding a static zeroed
    struct in `game_boot.c` before first use. Also: same symbol must be declared with
    the SAME type across TUs (it was `u8*` in one file, `u8[]` in another).
  - **Loading-screen stub semantics:** `DisplayLoadingScreen` must return NON-ZERO
    ("screen still active") so `InitializeAndLoadLevel`'s case-4/5 transition block is
    skipped, keeping the stage index (`s6`) at 1; else it clobbers to
    `PLAYER_STATE[stage]`=0 and the tail read fetches the level entry's padding
    (0 sectors → stale garbage). The MENU (seq[10], level slot 0) now loads correctly.
  - **KEY WORKFLOW (user tip):** `export/SLES_010.90.c` (full Ghidra decompile w/
    NAMED struct fields) + `export/datatypes.txt` are the primary reference for
    INCLUDE_ASM conversions — far cleaner than raw asm + m2c. `src/*.c` is already
    globbed into the port build, so every matched-C function is linked (only
    INCLUDE_ASM funcs abort). Verified `InitLayersAndTileState`'s size-decision tree
    offset-for-offset against the export.
  - Converted: the `InitializeAndLoadLevel` tail (`tile_colors`, `anim_palette_list`,
    `tile_attrib`, `entity_asset501`, `anim_tile_entities`), the layer subsystem
    (`InitLayersAndTileState` + `AddLayerToRenderList_*` [one shared body — all 3
    identical] + `InitLayerRenderContext_*` trio), the GPU-primitive layer
    (`CatPrim` added to `gpu_gl.c` + `InitTilemapLayer16x16` tilemap→SPRT_16 builder),
    `RemapEntityTypesForLevel` (the ~200-case authoring→runtime entity-id table),
    `PlayCDAudioTrack` (no-op, CD-DA is Phase 3), `SpawnPlayerAndEntities` (the
    flag-dispatched avatar/camera/HUD spawner), and `GetTileAttributeAtPosition`.
  - **Deferred to CP-2.4 GPU verify (needs PCSX-Redux pixel-compare):** OT
    termination for the built SPRT_16 chains, SPRT_16 CLUT/4-bit sampling in
    `gpu_gl.c`, the 2 sibling tilemap builders (`InitTilemapLayerRendering`,
    `InitTileLayerPrimitives`) + 3 parallax updaters (per-frame tick callbacks).
- _next (focused session):_ the **menu/UI subsystem** — `InitMenuEntity` (`passwd/`,
  119 lines) calls `InitMenuStage1-4` + `InitEntitySprite` + `UpdateBackgroundColor`;
  this is the title screen itself. Converting it + the 4 stages completes the MENU
  avatar and lets `InitGameState` return → the frame loop runs → CP-2.4 title render
  (then the PCSX-Redux first-frame diff).

- **2026-07-05 (session 3) — CP-2.4 MILESTONE: the title screen renders correctly.**
  Full animated logo (skull-head "O"), all four menu rows, clay-ball bullets, the
  critter selection cursor (moves with input), skeleton-hand ambient sprites — no
  corruption. Three GPU/render bugs fixed to get there:
  - **ABE/TGE flag bits broke prim dispatch**: `render_prim`'s switch matched exact
    code bytes, so any prim with the semi-transparent (0x02) or raw-texture (0x01)
    flag set (`0x66` SPRT, `0x7e` SPRT_16, …) fell to `default` and was silently
    skipped — the menu text and overlay tiles were invisible. Dispatch now masks
    the low 2 bits for 0x20..0x7F codes; ABE blending implemented per tpage rate
    (0: 50/50 via constant-alpha, 1: additive, 2: reverse-subtract, 3: ~additive).
  - **PSX STP-bit semantics in the shader** (two-pass): with ABE set, only texels
    with VRAM bit 15 blend; STP=0 texels draw opaque; texel 0 always transparent.
    `quad_tex_ex` draws ABE prims twice (uStpMode=1 opaque pass, =2 blend pass).
  - **THE big one — entity sprites never uploaded to VRAM** (every code=64/66
    sprite was invisible; menu text only showed via the tile layer): the frame
    loop's post-render dispatch (`(**(gs+0x18 vtable +0x1C))(gs+argOff)`) is the
    per-frame texture/CLUT flush pass, and THREE pieces were missing: (1)
    `ConstructStaticGameState()` was never called at boot (installs vtable
    D_80012100 at GameState+0x18 — src/gstate.c's DEAD_ENTITY_VTABLE alias is a
    misleading clean-room name; it's the live GameState vtable); (2) D_80012100
    itself was weak-zeroed rodata → strong def in `port/decomp/data/
    gamestate_vtable.c` {…, MainNoopCallback_80082844, EntityRemoval}; (3)
    `EntityRemoval` (0x80020D28 — ALSO misnamed: it's the per-frame post-render
    pass that walks the tick list dispatching each entity's vtable+0x1C =
    `UploadEntityTextureIfDirty`) + `UploadEntityTextureIfDirty`/
    `UploadCLUTToVRAM`/`UploadTextureToVRAM` converted (`port/decomp/blb/
    EntityRemoval.c`, `port/decomp/sprite/upload_vram.c`).
  - **VRAM region collision**: the PC-native sprite slot bump-allocator packed
    y<256 across the full 1024 width, marching into the tile atlas (x>=320) →
    garbage-textured sprites. Sprite textures now live at y 288..511 (below the
    CLUT rows at 256..287); tpage y-bit + V-byte addressing stay exact.
  - **Debug tooling added to gpu_gl.c**: `PORT_OT_LOG=N` (dump the Nth OT pass
    prim-by-prim), `PORT_VRAM_DUMP_FRAME=N` (defer the VRAM dump), and the VRAM
    dump now also writes a raw `.bin` (preserves the STP/mask bit the PPM loses).
- _next:_ PCSX-Redux side-by-side pixel diff of the title frame (CP-2.4 verify);
  then menu input → START GAME → level 1 load path (CP-2.5/2.6). Note the black
  box + critter next to the selected row is CORRECT game behaviour (the critter
  eats the selected row's text), not a render bug.
- **2026-07-06 (session 4 cont.) — playst push: the player TICKS.** Converted
  PlayerTickCallback (the per-frame player driver: input-slot dispatch, timers,
  halo/yellow-bird companions, scale easing, damage-flash tint, particle trail),
  PlayerProcessTileCollision + CheckTriggerZoneCollision,
  PlayerCallback_JumpInputAndCounters, FindFrameIndexByValue. The per-frame
  stub surface on level 1 is now exactly: **PlayerCallback_FallingPhysicsMain**
  (0x18CC/818 export lines — the airborne physics + real tile collision core;
  a focused session with ~10 callee deps: FallingPhysicsContinue sibling,
  EntityApplyMovementCallbacks, bounce handlers, platform landing,
  FallingInputHandler), PlayerEntityCollisionHandler (event), and the two
  non-playst ticks (EntityTick_PlatformRideIdle, EntityTick_DigitDisplayUpdate).
  - **CLEAN-ROOM NAME CORRECTIONS (verified against the .s; apply to Ghidra
    when the MCP is up):**
    - `PlayerProcessTileCollision` (0x8005A914) does NO tile collision — it is
      the TRIGGER-ZONE processor (rect list at GameState+0x74/+0x78, BLB asset
      602): level exits, game-mode zones 2-7, wind/conveyor pushes, autoscroll
      toggles, clayball flags, tint zones. Suggest PlayerProcessTriggerZones.
      Real tile collision is inside PlayerCallback_FallingPhysicsMain et al.
    - `g_DefaultBGColorR/G/B` (0x800A5770-72) are NOT colors and Ghidra's
      `_`-prefixed accesses resolve to the WRONG addresses — the actual loads
      are the button-mask config words D_800A596C (0x40 jump), D_800A596E
      (0x20 special), D_800A5970 (0x80 run), already strong C in src/gfx.c's
      migrated sdata island.
    - PlayerTickCallback / PlayerProcessTileCollision's extra Ghidra params
      are register residue; both are 1-arg FSM callbacks.
    - PlayerEntity +0x159 ("pendingStateChange") is specifically a pending
      DEATH request (consumed by the D_800A5D98/9C DeathStart pair).
    - The export's fsmSlot_* aliases map to the src/playst.c data island:
      Hamster=D_800A5D90/94, DeathStart=D_800A5D98/9C, CrouchSlide=D_800A5D88/8C,
      SpecialIdleAnim=D_800A5E68/6C, ExitShrinkWithRestore=D_800A5E70/74,
      ShrinkAndFall=D_800A5E78/7C, GrowFromShrink=D_800A5E80/84 (all verified
      via gp-rel loads in the .s).

- **2026-07-06 (session 4) — LEVEL 1 FIRST LIGHT: direct boot into level 1
  loads the full level and renders its tile layers (scrambled re-stamp, see
  below); the player entity constructs and enters its state machine.**
  - **`PORT_BOOT_LEVEL`/`PORT_BOOT_STAGE`** (LoadBLBHeader.c): boots straight
    into a level by poking the BLB header exactly like `make record LEVEL=n` /
    game_watcher's boot override does (header+0xF36 mode=3, +0xF92 level,
    +0xF93 stage). This bypasses the menu's START flow entirely (note: menu
    START is a whole animation-driven sequence — X only fires the item's
    "highlight" callback; the actual transition chain was never reached and
    remains unconverted).
  - **`PORT_STUBS_NONFATAL=1`** (port_runtime.c): log-and-continue stub mode
    for reconnaissance — one run lists every still-asm function on the
    per-frame path instead of dying on the first.
  - Converted this session (~12 functions): FINN_ClearSubentityState,
    PlaySoundEffect (no-op stub, SPU mix is Phase 3), InitTileLayerPrimitives
    (bounded tile layer builder), InitTilemapLayerRendering +
    RenderTilemapSprites16x16 (wrap-scroll screen-grid layer; NOTE the export
    plate comments mislabel the layer struct — +0x68 tileTable, +0x6C tilemap,
    +0x70 colorTable, +0x74 origin, +0x78 dims, verified against the render
    fn), SetupTilemapPrimitives + RenderTilemapHorizontal/VerticalScroll
    (per-frame OT submission + delta re-stamp — PC version re-stamps the full
    window on any delta), UpdateParallaxScrollWithWrap_Standard/_Small,
    RenderTilemapWithWrapAround (simplified: submits all row chains, single
    wrap copy), EntityCollision_HUDItemActivate (HUD widget-group reveal —
    export comment describes a different function!), CreatePlayerEntity +
    InitPlayerSpriteAvailability + PlayerStateCallback_2 (+ strong data:
    g_PlayerCallbackTable, D_8009C174 spawn blob, g_RunButtonMask=0x26).
  - **ABI traps found (write these down):** (1) `InitTilemapLayerRendering` is
    void in the export but its CALLER consumes $v0 as the layer pointer —
    x86 returns garbage; return `storage` explicitly (crashed the boot).
    (2) `CreatePlayerEntity`'s Ghidra 5-param signature hides that
    `InitEntityWithSprite` takes the clean 5-arg form (entity, D_8009C174,
    0x3E8, x, y) — resolved from the .s, not the decompile. (3) The PSX
    P_TAG len-byte write in the empty-cell path of RenderTilemapSprites16x16
    would corrupt the PC 32-bit tag pointers — drop it, CatPrim carries the
    semantics. (4) PSX fb-half Y bias (buf*0x100) and 11-bit +0x8000 offset
    encoding in DR_OFFSET packets do not apply to the GL backend.
  - **Known-broken / next (focused session):** level tile re-stamp is
    scrambled (ring-slot vs screen-coord mapping in the PC full-window
    re-stamp needs a PCSX-Redux side-by-side); `PlayerTickCallback` (0x6BC,
    pulls in PlayerProcessTileCollision — the tile-collision engine) +
    EntityTick_PlatformRideIdle + EntityTick_DigitDisplayUpdate are the three
    remaining per-frame stubs on the level-1 path (visible via
    PORT_STUBS_NONFATAL). Repro: `PORT_BOOT_LEVEL=1 PORT_STUBS_NONFATAL=1
    ./port/build-linux-debug/skullmonkeys`.

- **2026-07-06 (session 6) — LEVEL-1 ENTITY POPULATION SPAWNS AND RENDERS
  (commit d78cc98): 800-frame run, exit 0, every PHRO-stage0 spawn-table
  entity type constructs — clayballs/ammo/swirlys visible around Klaymen in
  the frame captures.** ~24 functions converted (decor shell, pickup inits,
  collectible/platform inits, engine plumbing — see the commit message for
  the full list). Three crash root-causes worth remembering:
  1. **EntityTickLoop dispatches through the collision VTABLE**
     (`vtable+0x10/0x14`), so a weak-zero vtable = jump to NULL the moment
     a decor entity registers. `gen_port_data.py` now transcribes
     `hud.rodata.s` + `clayball.rodata.s` (new: `.asciz` blocks, MIPS jump
     tables as zeroed placeholders, and splat's `.L00000000_main`
     null-pointer-slot idiom — the latter was silently DROPPING zero words,
     which had truncated `g_PlayerCallbackTable` to 6 of 16 words; the
     hand-written player_vtable.c is deleted, superseded by the generator).
  2. **Pointer-returning init weak stubs return NULL** into
     AddEntityToSortedRenderList/AddToZOrderList. Fixed by mapping the
     level's real needs: 501-record `(category, raw-id)` pairs →
     RemapEntityTypesForLevel tables → runtime types {1,2,7,9,0x16,0x18,
     0x2D,0x34,0x46,0x55,0x5F,0x6A-0x6C} → their 9 inner inits, all
     converted (port/decomp/enemies/collectible_inits.c,
     pickups/pickup_inits.c, bosses/InitGenericSpriteEntity.c).
  3. **Sprite-def lists (D_8009B214 …) are hash arrays, not pointers** —
     the ≥0x80000000 pointer heuristic would corrupt them; hand-transcribed
     as strong data in port/decomp/data/sprite_def_lists.c.
  - Ctor arity trap: the export drops InitEntityWithSprite's tail args —
    the .s shows `(entity, defList, 0x3CA, spawn->x, spawn->y - 1)` for
    collectibles, `(…, 0x3DE, x, y)` for decor. Check asm call sites.
  - `port/decomp/decor/fsm_event.h`: shared event-slot dispatch
    (marker@+8/+A, fn@+C — same offsets in Entity and GameState).
  - Headless verify recipe (desktop runs hang on Wayland occlusion — vsync
    swap never returns when the window is hidden): `nix shell nixpkgs#xvfb-run
    --command xvfb-run -a -s "-screen 0 1024x768x24" env LIBGL_ALWAYS_SOFTWARE=1
    SKULLMONKEYS_BLB=disks/blb/GAME.BLB PORT_BOOT_LEVEL=1
    PORT_STUBS_NONFATAL=1 PORT_MAX_FRAMES=800 PORT_CAPTURE=... skullmonkeys`.
    Note SDL_VIDEODRIVER=dummy has no GL → host_sdl_init fails → the game
    NEVER RUNS (a "no stub hits" result that way is meaningless).
  - _next:_ the per-frame behavior layer, in rough order of visible impact:
    GenericSpriteEntityTickCallback + EntityTick_InterpolateTimedPath
    (platforms move), PlayerState_DeathStart + RemoveEntityFromAllLists
    (death/respawn; the player currently vanishes after falling),
    CollectibleTickCallback/CollectibleOrbTickCallback/AddSwirlys (pickup
    collection), EntityCheckTriggerZone, sound emitter ticks. Then the
    PCSX-Redux side-by-side (framing/palette) and input smoke test.

- **2026-07-06 (session 5 cont.) — PLAYER FALLS ON SCREEN, camera frames him.**
  The "player invisible at y=-65 / camera never follows" symptom was NOT a
  camera bug: UpdateCameraPositionSmooth, its easing tables (real data in
  src/blb.c), spawn position (PHRO stage0 spawns at tile (20,0) → px (328,15),
  flags=0x1000 → legitimately no camera entity) all check out. Root cause: in
  PlayerCallback_FallingPhysicsMain, m2c typed the X/Y position accumulators
  `var_a3`/`var_s1` as s16 — but the .s keeps them in full 32-bit registers
  (`sra v0,a3,16` → worldX; `sh a3,0x6C` stores only the sub-pixel half; the
  Entity +0x6C/+0x6E velocity fields really ARE s16 sub-pixel latches). The s16
  temp truncated `(worldX<<16)` to 0 → player teleported to (0,0) on tick 1 and
  idled there forever, off-screen. Fixed: s32 accumulators. Verified: player
  spawns at (328,15), falls with gravity on screen (capture frame 52 shows
  Klaymen mid-drop over the spires), camera tracks.
  - Debug method that cracked it: gdb hardware watchpoint on player+0x68
    (break in the physics, `watch -l e->sprite.base.worldX`) — caught the
    integration line red-handed after printf-tracing showed sane inputs.
    `PORT_CAM_DEBUG=1` camera-state trace kept in UpdateCameraPositionSmooth.
  - **m2c-conversion lesson for port/decomp: audit accumulator temp types.
    Any `(x << 16) + subpixel` pattern must be s32 even when m2c infers s16.**
  - _next:_ the run now reaches the LANDING at frame ~52 and stops at
    `InitClayballWithRandomColor` (0x8002DBDC, 0x264, pickups; clayball
    entity init — export decompile is clean, InitEntitySprite +
    InitPathFollowingDecorEntity + random tint, callees mostly converted).
    Converting it (+ whatever follows: CollectibleClaySingleTickCallback is
    tiny) unblocks post-landing gameplay. Then: PCSX-Redux side-by-side
    (framing + dark tint check), input smoke test, HUD/entity population.

- **2026-07-06 (session 5) — LEVEL-1 TILE SCRAMBLE FIXED: the level renders
  coherent parallax layers.** The "scrambled re-stamp" was never a re-stamp/
  ring-mapping bug — the tilemaps, tile table, VRAM pixels and CLUTs were all
  verified byte-perfect against the BLB (offline python cross-check of the
  dumped layer inputs vs extracted PHRO/stage0 assets; a python re-render of
  layer 6 from the same data was pixel-coherent). Two GPU HAL bugs caused it:
  1. **`SetDrawTPage` clobbered the chain**: it wrote `p->tag = 0`, but
     `RenderTilemapSprites16x16` calls it AFTER CatPrim-chaining the DR packet,
     so every occupied tile severed the SPRT_16 chain (the title-screen
     builders set the code before chaining, which is why CP-2.4 looked fine).
     Same latent hazard removed from `SetDrawOffset`. Rule: packet-builder
     helpers must never touch the tag word on PC.
  2. **`DR_TPAGE` was 4 bytes in psyq_pc.h** (`{u_int tag;}`); the real packet
     is tag+code (8 bytes) and every pool allocates stride 8. In
     `InitTileLayerPrimitives` the `pt++` walk advanced half a packet, so DR
     code words overlapped the next packet's tag: the OT walker saw garbage
     code bytes, never executed the DR_TPAGEs, and every tile drew with a
     stale `s_cur_tpage` → per-tile wrong art on all row-chain layers.
  - Debug additions: `PORT_LAYER_DUMP=dir` (tilemap_scroll.c) dumps each
    wrap-layer's init inputs (params + tilemap/tiletable/colors bins) for
    offline diffing; the OT logger now hexdumps unhandled prims (that dump —
    consecutive stride-0x10 sprite pointers where a code word should be — is
    what cracked the struct-size bug).
  - Level-1 world identified: **PHRO stage0** (dumped layer tilemaps match the
    extracted `200_tilemap_container.bin` slices exactly; dims 454x42 etc.).
  - Verified: 260-frame boot renders stable, coherent spire/rock parallax
    layers, no scramble, no stub hits (the session-4 falling-physics files
    closed the per-frame stub surface — `PORT_STUBS_NONFATAL` run is clean).
  - _next:_ PCSX-Redux side-by-side for exact framing/palette (scene is dark —
    tint tables give 0x40s; confirm against emulator), player sprite
    visibility at spawn, camera/scroll motion, then CP-2.6 playability pass.

- **2026-07-05 (session 3 cont.) — START GAME accepted; level-1 load runs to the
  player-avatar spawn.** Headless input injection added (`PORT_AUTOINPUT=
  "frame:mask,..."` in pad_sdl.c, e.g. `200:4000` = press X at frame 200; also
  `PORT_CAPTURE_EVERY` + `%`-pattern multi-frame capture in gpu_gl.c). Pressing
  X on START GAME leaves the menu, runs ~200 frames of level-transition logic
  (fade timers, sequence advance, level-1 asset load — loading screen/movies are
  no-op stubs so the display stays on the title), and aborts cleanly at
  **`FINN_ClearSubentityState`** (0x80075858, 0x64 bytes) — the first stub in the
  level-1 player-avatar init. The `finn/` segment has ~20 asm functions; that
  subsystem (plus whatever follows it in SpawnPlayerAndEntities' dispatch) is
  the CP-2.5/2.6 work unit. Animations confirmed advancing (~every 4 frames)
  via the multi-frame capture diff.

- **2026-07-07 (session 7) — LEVEL-1 PER-FRAME BEHAVIOR LAYER: player falls,
  lands, and reaches the grounded idle state; no crash (commits 853f469,
  fe61e0c).** 800-frame headless run exits 0. Converted the per-frame stub
  surface that fires once Klaymen reaches the ground.
  - **Death/land/platform/trigger tick layer (853f469, ~18 fns):**
    `GenericSpriteEntityTickCallback` (platform/prop tick), `PlayerStateInit_
    ShrinkAndFall` + `PlayerState_DeathStart` (the shrink/death state installers
    — landing on a hazard reached these), the list-unlink family
    (`RemoveEntityFromAllLists`/`RemoveFromTickList`/`RemoveEntityFromUpdateQueue`/
    `FreeEntityLists`), `EntitySetCallback` + `EntityProcessCallbackQueue` (the
    shared tagged-slot dispatchers), the platform ride easing
    (`PlatformRideStartUp`/`StartDown`/`PlatformInterpolatePosition`,
    `EntityTick_InterpolateTimedPath`), `SoundEmitterWithPanningTick`,
    `AddSwirlys`, `EntityCheckTriggerZone`, `IsEntityNearSoundTrigger`,
    `FreeTextureResource`.
  - **Grounded idle-input dispatch (fe61e0c):** `PlayerCallback_IdleInputHandler`
    + its `CheckWallCollision` dependency. The idle state now reads the pad and
    routes to swirly-toss / jump / walk / turn / ledge-fall via EntitySetState.
    Removed the 723-hit/frame idle stub; with the FSM advancing,
    PlayerCallback_HorizontalWallCollision fell from 723 to 29 hits/frame.
  - **Two PC-divergence guards** (PSX reads harmlessly at ~addr 0; PC faults),
    both TARGET_PC-gated so the MIPS byte-match is untouched (`make check`
    green, SHA1 5a14b65c...): `src/finn.c` ClearEntityStateFlag NULL glide-entity
    guard; `anim/UpdateSpriteFrameData` skip when a missed-TOC sprite id leaves
    the frame-metadata base NULL. **Sprite-miss note:** level-1's loaded TOC
    (77+22 entries) does NOT contain many player/HUD sprite hashes incl. the
    shrunk-fall sprite 0x987101B9 — the full 96-entry variant lives in BLB dir
    entry 18's block, so the current level-1 (dir entry 1 / PHRO stage0) simply
    doesn't ship them; the guard makes that a no-op instead of a crash. Worth a
    later look at whether a second container should be loaded.
  - _next:_ `PlayerCallback_HorizontalWallCollision` (466 lines, the grounded
    event slot; m2c gives clean structure) is the largest remaining per-frame
    stub. Then `PlayerEntityEventHandler` (167) and the one-shot
    death/VRAM helpers (SpawnPlayerDeathEffect, PlayerApplyPositionWithCollision,
    FreeAndCoalesceVRAMSlot). `EntityCollision_SpawnSwitchBlock` stays stubbed
    until `InitSwitchBlockEntity` (still asm) is converted. Then the PCSX-Redux
    side-by-side (framing/palette) + input smoke test.
