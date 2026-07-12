# PSX-mirror arena: byte-comparable RAM/VRAM dumps from the PC port

**Status:** Tier 1 + Tier 2 *mapping* done (2026-07-12). `port_heap.c` mmaps the
arena AT `0x80000000` (`MAP_FIXED_NOREPLACE`, works on this host; malloc
fallback kept, `PORT_NO_FIXED_ARENA=1` forces it), `g_pBlbHeapBase = 0x800907EC`
bit-identical to PS1, and the `D_800AE3E0` level-staging buffer moved to its PSX
offset — so heap, staged level data and the level sub-heap all reproduce the PS1
RAM layout. `PORT_TRACE_REGION` (full | gamestate | 0xADDR:SIZE) dumps any RAM
slice, with the GameState object overlaid at `0x8009DC40`. Measured on the SCIE
demo replay: GameState pointer noise dropped from 36 words to 6 (the remaining
6 = function pointers → Tier 3, plus non-arena globals → Tier 2's defsym step,
both still open). Tier 2 *globals migration* (defsym + .sdata init copy) and
Tier 3 fn-ptr translation are not started.
**Motivation:** the demo-replay diff pipeline (`make port-trace` vs `make trace`,
`pcsx_stream.py diffdb`) currently compares only the 0x1A0 GameState object and
has to mask ~146 bytes of "pointer noise" per frame (host addresses vs PSX
addresses). A PSX-mirrored memory arena makes the port's RAM dumpable in the
exact shape the emulator sees, so whole-RAM `diffdb` works with little or no
masking — while the game code stays plain portable C.

## Why this is cheap here

Three properties of the current port make this unusually low-cost:

1. **The build is already 32-bit** (`-m32`, `port/CMakeLists.txt`). Pointers are
   4 bytes, struct layouts already match the PSX ABI, and the 32-bit user
   address space *includes* 0x80000000.
2. **The game brings its own allocator.** `InitHeapConfig` /
   `InitHeapFreeList` / `AllocateFromHeap` are the game's decompiled arena
   allocator, already running inside one host block (`port/spec/port_heap.c`).
   Allocation order is deterministic (proven: two port runs produce
   byte-identical GameState traces), so block offsets inside the arena already
   reproduce the PSX heap layout. Only the base address differs.
3. **Every global's PSX address and size are already known.**
   `tools/gen_port_stubs.py` emits `_autoglobals.c` weak blobs from
   `symbol_addrs.txt`; the same data can place globals at their PSX offsets
   instead.

## Tier 1 — fixed-offset arena (fallback, trivial)

Replace the `malloc(16MB)` in `port_heap.c` with one 2 MB arena representing
PSX RAM (`0x80000000..0x80200000`), plus the existing slack region for
not-yet-sized overruns:

    host = arena_base + (psx_addr - 0x80000000)

- `g_pBlbHeapBase` points at the PSX heap carve offset, not an arbitrary block.
- "Dump RAM" = one 2 MB memcpy, streamed in the existing FRM1 format with real
  PSX region addresses (`port_trace.c` grows a `PORT_TRACE_REGION` env mirroring
  `RAMDUMP_REGION`).
- Stored pointers still differ from PS1 by the constant
  `arena_base - 0x80000000`; the dumper (or `diffdb`) applies a constant-delta
  rewrite to 4-byte-aligned words that look like arena pointers, or diffs with
  masking as today.

## Tier 2 — map the arena AT 0x80000000 (the target)

    mmap((void *)0x80000000, 0x200000, PROT_READ|PROT_WRITE,
         MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE, -1, 0)

In a `-m32` process this address is normally free. Then:

- Host pointers to game data **are** PSX addresses. Stored data pointers in RAM
  become bit-identical to the emulator's; the GameState diff's pointer-noise
  baseline collapses to fn-pointer fields only.
- No translation layer exists anywhere. Game code is untouched portable C; the
  only platform-specific line is the mmap itself (fall back to Tier 1 when the
  mapping is refused — e.g. hardened kernels, or a future 64-bit build).
- **Globals move into the arena.** `gen_port_stubs.py` stops emitting weak
  byte blobs for data symbols and instead emits a linker fragment of
  `--defsym D_xxxxxxxx=0x8xxxxxxx` (data symbols only; functions keep their
  stub/strong-C story). Code references resolve to arena addresses directly.
  - Real statics that currently shadow PSX globals (`s_respawnPlayerState`,
    `s_inputStateA/B` in `game_boot.c`) become plain writes to their PSX
    addresses; the re-seeding comments there mostly disappear.
  - Deletes the PC memory-model hacks, e.g. the clamped `0x801FC000` RAM-top
    constant in `InitializeAndLoadLevel` becomes exact.
  - `.sdata` initialised values (ROM initializers) need a one-time init copy
    into the arena at boot — generate an init table from the same data
    `gen_port_data.py` already extracts.

## Tier 3 — function-pointer translation (dump-time only)

The residue after Tier 2: FSM slots and vtables store *code* addresses — host
function pointers on the port, 0x80xxxxxx on PS1. Code cannot be mapped at PSX
addresses, so translate at dump time instead:

- `symbol_addrs.txt` already maps every function to its PSX address. Build a
  host-fn-ptr → PSX-addr table at startup (one registration macro per converted
  function, or walk the dynamic symbol table and join by name).
- The dumper rewrites 4-byte words that match registered host fn pointers to
  their PSX values before emitting the frame.
- After this, whole-RAM `diffdb` against a PCSX-Redux trace needs **no masking
  at all**: any remaining diff is a real behavioral divergence.

## VRAM (already solved)

The GL backend keeps a live 1024x512 VRAM image (the CLUT-expansion shader
samples it) and PCSX-Redux exposes `read_vram`. Add `PORT_TRACE_VRAM=<file>` to
dump the port's image; compare rectangles against the emulator's with a small
`tools/` script. No allocator work needed.

## Suggested order of work

1. Tier 1 arena in `port_heap.c` + full-RAM `port-trace` region (half a day;
   immediately widens the diff surface from 0x1A0 to everything the heap owns).
2. Tier 2 mmap + defsym generation in `gen_port_stubs.py` (the big win; ~a day,
   mostly regenerating and re-linking `_autoglobals`).
3. Tier 3 fn-ptr table (small, incremental; only needed when fn-slot noise
   becomes the dominant masking cost).

## Risks / caveats

- `MAP_FIXED_NOREPLACE` can fail under `vm.mmap_min_addr`-style hardening or if
  the 32-bit loader placed something there — hence the Tier 1 fallback path
  must stay functional (single `PSX2HOST()` macro, identity under Tier 2).
- Host-only state (SDL, GL handles, FILE*) must never live in the arena; today
  it doesn't (it all sits in port/spec statics).
- Function-pointer *comparison* semantics inside game code (e.g.
  `stateFn == PlayerStateInit_Walking`) are unaffected — both sides are host
  pointers at runtime; only the dump is translated.
- A future 64-bit port build breaks Tier 2 (and PSX ABI struct layout
  generally); the arena design keeps that decision independent — Tier 1 math
  works at any pointer width the structs survive.
