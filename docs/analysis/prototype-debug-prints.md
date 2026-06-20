# Prototype debug prints (recoverable instrumentation)

Status: **in-flight / partial retail mapping confirmed (2026-06-21).** Source =
beta debug build `disks/prototype/ext/SLUS_006.01` (US, 639 KB, Dec 16 1997).
Retail target = `SLES_010.90` (PAL). Addresses below are **prototype vaddrs**
unless marked "retail". The prototype is *not* symbol-rich — no `.SYM`, no
`assert(__FILE__)` strings. The only recoverable instrumentation is game
`printf`/`FntPrint` call sites whose format strings name the values they print.

## Two debug output channels

- **`printf`** (proto `0x8008cd78`) — TTY/console. **Stripped from retail**:
  none of the `printf` format strings below survive in the retail binary
  (verified: `Layers = %d`, `TUs = %d`, `ram u=%d`, `FC8B=%d`, `ProgEnd = `,
  `MISSING SEQ` all return zero hits in `bin/SLES_010.90`). The whole call
  blocks were `#ifdef`-compiled out, so there is usually **no retail function
  to add them back to** — they'd be reconstructed as DEBUG-only code.
- **`FntPrint`** (on-screen font, retail `0x800892E4`) — **partly survives in
  retail**. Retail has exactly **5** `FntPrint` call sites (verified via `jal`
  scan of `.text`):
  - `0x80061014` + `0x80061150` — `PlayerCallback_DebugCameraUpdate` (#4 below)
  - `0x80082C90` + `0x80082CA8` + `0x80082CB8` — `ProcessDebugMenuInput`
    (new finding, see §5).

  No other HUD survives.

## Shared debug globals (prototype)

Both proto HUD overlays gate and read through the same two globals:

- `0x800AAE54` — debug-flag word; bit `0x40` enables the memory/render HUD
  (`lhu; andi v0,0x40; beqz skip`). *(Doc previously said `0x800Bae54` — was a
  typo; re-verified from proto disasm at `0x80085D44`: `lui v0,0x800b; lhu
  v0,-20908(v0)` → `0x800AAE54`.)*
- `0x800AAE50` — base pointer to a per-frame stats/context struct. HUD reads are
  double-buffered: `base + (frame_parity * 0x10000)` (`lui at,0x1; addu`).
  *(Same vaddr-prefix typo correction.)*

Retail analogues of these two globals are the highest-value thing to find — they
are written by normal render/heap code and give meaning to otherwise-opaque
retail globals. **Not yet located in retail** (BSS shifted between proto and
retail; the proto `0x800AAE5x` cluster is the heap/render-stats system which was
likely `#ifdef`-compiled out, so the retail BSS slot may not exist at all).

## Recovered call sites

### 1. Audio — `"MISSING SEQ $%x"`  (printf)
- Proto format string: `0x80010578` (file off `0xd78`).
- Proto call site: `0x8001F388`, inside a sound-sequence loader (error path,
  not gated by the HUD flag — prints whenever a load fails).
- Disasm confirms object struct (`s1`): `+0xCC` = sequence id (`%x` arg),
  `+0xD8` = loaded / handle flag (`sh` at +0xD8 right after `jal 0x1b310` /
  `jal 0x1b39c` populate it; checked `!=0` to skip the error). The cleanup
  path after the print calls `subsys_a(s1)` then `subsys_b(s1)` with the
  global `*0x800AAE50` as `a0`, then spins on `subsys_yield()`.
- Naming leads: the host fn is a "load sound sequence" routine; `+0xCC`/`+0xD8`
  are sequence-id and load-state fields. Retail status: format string is
  stripped, but the host function logic survives — find the retail loader by
  the `+0xCC`/`+0xD8` access pattern and the call to the audio yield helper.

### 2. Renderer HUD — `"Layers = %d, AL# = %d"` + `"TUs = %d, (%d%%)"`  (printf)
- One function. `Layers`/`AL#` at `0x80023220`, `TUs`/`%` at `0x8002325C`.
- Gated by `0x800AAE54 & 0x40`.
- `TUs` percentage computed by reciprocal-multiply (magic `0x2e8ba2e9`) =
  `TUs * 100 / budget` → there is a fixed texture-upload budget per frame.
- Naming leads: per-frame render stats — layer count, "AL#" (active list?), and
  texture-upload count vs budget.
- Retail status: stripped — the `0x2e8ba2e9` magic constant has zero hits in
  the retail binary (verified). The whole HUD was compiled out.

### 3. Heap HUD — `"ram u=%d mu=%d f=%d"`, `"FC8B=%d, CA=%d"`, `"ProgEnd = 0x%x"`  (printf)
- One function (`0x80085D44`–`0x80085DF4`), gated by `0x800AAE54 & 0x40`.
- Heap accounted in **16-byte units** (`<<4`). Stats struct fields (relative to
  `base + parity*0x10000`):
  - `+0xC094` lw — heap top / limit (`v1`)
  - `+0xC09C` lhu — used (units) → `u = top - used*16`
  - `+0xC09E` lhu — max-used (units) → `mu = top - maxused*16`; `f = used*16`
  - `+0xC0A0` lw — `ProgEnd` (program end address)
  - `+0xBC08` lhu / `+0xBC0C` lhu — `FC8B` / `CA` counters (unknown subsystem)
- Candidate struct definition (proto-only; not yet applied to a retail global):

  ```c
  /* Proto per-frame stats / heap-accounting struct (double-buffered by
   * frame parity: read at base + parity*0x10000). 0xBC08/0xBC0C are in
   * an earlier sub-region than the heap fields. */
  typedef struct {
      /* ... */
      /* 0xBC08 */ u16 fc8b_counter;     /* "FC8B" — unknown subsystem  */
      /* 0xBC0A */ u16 _pad_bc0a;
      /* 0xBC0C */ u16 ca_counter;       /* "CA"   — unknown subsystem  */
      /* ... */
      /* 0xC094 */ u32 heap_top;         /* heap top / limit address    */
      /* 0xC098 */ u32 _pad_c098;
      /* 0xC09C */ u16 heap_used_units;  /* heap allocated, /16 units   */
      /* 0xC09E */ u16 heap_peak_units;  /* heap peak-used, /16 units   */
      /* 0xC0A0 */ u32 program_end;      /* `ProgEnd` (low water mark?) */
  } ProtoStatsBuffer;
  ```
- Naming leads: a heap/memory accounting struct; these directly name several
  retail globals once the struct base is located.

### 4. On-screen coords — `"%d:(%d,%d)"`  (FntPrint, **survives in retail**)
- Retail format string: `D_80011774 = "%d:(%d,%d)\n"` (verified at file off
  `0x1f74`).
- Retail callers: `PlayerCallback_DebugCameraUpdate` (`0x80060FE8`, prints
  `(stage_index, x_pixel, y_pixel)` via two `FntPrint`s — first one is the
  `"%s\n"` level-name formatter at `D_800A5EC8`, second is the coord print).
- This one can be re-added to the matched function directly under a guard.

### 5. Debug menu (level / sub-stage selector) — `"> "`, `"  "`, `"\n"` + `"sub NN"`  (FntPrint, **survives in retail**)
**New in 2026-06-21 pass.** This is the *only* fully-intact debug HUD in the
retail build. All three remaining `FntPrint` call sites in retail live inside
one function:

- Retail host: `ProcessDebugMenuInput` (`0x80082C10`, size `0x280`), called
  once per frame from `main` near the end of the tick.
- Gate: `g_GameFlags & 0x80` (retail debug-flag word at `0x800A5958`, distinct
  from the proto's `0x800AAE54`).
- Render loop (visible window of up to 20 items starting at scroll-top):
  ```
  FntPrint(cursor==i ? "> " : "  ");   /* D_800A6114 / D_800A6118 */
  FntPrint(g_DebugMenuItemNames[i]);   /* D_8009DDE0[i] -> "sub 01".."sub 10" */
  FntPrint("\n");                       /* D_800A611C */
  ```
- Input read via active-pad pointer `g_pCurrentInputState` at `0x800A6120`
  (`lhu 0x0` = buttons held, `lhu 0x2` = buttons pressed this frame). D-Up
  `0x1000`, D-Down `0x4000`, Start `0x40` (commit). On commit, calls
  `SetSequenceIndexByMode(&g_pGameState->stage_ctx, mode, level)` and stuffs
  the chosen level index into `GameState.direct_level_load (+0x148)`.
- "END2" sentinel item (`D_800A60A8 = "END2"`) is `strcmp`-tested against
  `g_DebugMenuItemNames[cursor]` to pick between sequence-mode `1` and `4`.

**Retail globals named from this analysis (added to `symbol_addrs.txt`):**

| vaddr        | name                      | type   | role |
|--------------|---------------------------|--------|------|
| `0x800A5958` | `g_GameFlags`             | `u16`  | bit `0x80` = debug menu enabled |
| `0x800A60B8` | `g_DebugMenuScrollTop`    | `u16`  | first visible item index |
| `0x800A60BA` | `g_DebugMenuCursor`       | `u16`  | currently selected item |
| `0x800A60C0` | `g_TotalMenuItems`        | `u8`   | upper bound on cursor |
| `0x800A6120` | `g_pCurrentInputState`    | `InputState *` | active input ptr (defaults to player 1), read by debug-menu nav |
| `0x8009DDE0` | `g_DebugMenuItemNames`    | `char *[10]` | "sub 01" … "sub 10" |
| `0x800A60A8` | `g_DebugMenuSentinelStr`  | `char[8]`    | `"END2"` — terminator marker |
| `0x800A60C4` | `g_DebugMenuSubLevelStrs` | `char[10][8]`| 10 × `"sub NN\0"` |
| `0x800A6114` | `g_DebugMenuArrowStr`     | `char[4]`    | `"> "` cursor prefix |
| `0x800A6118` | `g_DebugMenuIndentStr`    | `char[4]`    | `"  "` indent prefix |
| `0x800A611C` | `g_DebugMenuNewlineStr`   | `char[2]`    | `"\n"` |
| `0x80011774` | `g_FntFmt_StagePos`       | `char[12]`   | `"%d:(%d,%d)\n"` (#4) |
| `0x800A5EC8` | `g_FntFmt_PercentS`       | `char[4]`    | `"%s\0"` (#4 level-name) |

## Next steps

1. **DONE 2026-06-21** — Locate the retail debug HUD that survives → it's
   `ProcessDebugMenuInput` (§5). 13 retail globals named.
2. Locate retail analogues of `0x800AAE50` / `0x800AAE54` (proto stats base +
   debug flag) — would unlock #2 and #3 as global/struct documentation. May
   not exist in retail at all if the heap/render HUD was stripped at link
   time, not just at compile time.
3. Find the retail sound-sequence loader for #1 by signature: callee that
   `sh`'s into `+0xD8` and `lw`'s sequence-id from `+0xCC`; calls a pair of
   audio-subsystem helpers with `*g_pAudioCtx` then spins on a yield helper.
4. Add a `DEBUG`-guarded print macro + debug make target; re-add #4 first (it
   maps to an already-matched function), then the reconstructed HUD fns.
