// Splat config source for Skullmonkeys PAL (SLES-01090).
//
// This file is the editable source of truth. Run `make config` to render the
// generated YAML consumed by splat.

local Project = 'SLES_010.90';
local Hex(value) = std.parseHex(value);

local subsegment(start, kind, name=null) =
  if name == null then [Hex(start), kind] else [Hex(start), kind, name];

local asm(start, name) = subsegment(start, 'asm', name);
local c(start, name) = subsegment(start, 'c', name);
local rodata(start, name) = subsegment(start, 'rodata', name);
local data(start, kind) = subsegment(start, kind);
local bss(start, kind, vram) = {
  start: Hex(start),
  type: kind,
  vram: Hex(vram),
};

{
  name: Project,
  sha1: '5a14b65cb44813bfed1ee53c6a3f4456bc230f97',

  options: {
    basename: Project,
    target_path: 'bin/' + Project,
    elf_path: 'build/' + Project + '.elf',
    base_path: '.',
    platform: 'psx',
    compiler: 'PSYQ',
    migrate_rodata_to_functions: false,

    asm_path: 'asm',
    src_path: 'src',
    nonmatchings_path: 'nonmatchings',
    build_path: 'build',
    hasm_in_src_path: true,

    ld_script_path: Project + '.ld',
    ld_dependencies: true,

    find_file_boundaries: true,
    gp_value: Hex('800A5954'),

    o_as_suffix: true,
    use_legacy_include_asm: false,

    // Generate the maspsx-keep INCLUDE_ASM macro so matched C and INCLUDE_ASM
    // can interleave anywhere in a file (lifts the cc1 tail-only constraint;
    // see docs/compiler-quirks.md Quirk 1). Without this, splat regenerates
    // include/include_asm.h as the classic top-level-asm macro on every
    // extract, which floats included asm above compiled C and breaks the match.
    include_asm_macro_style: 'maspsx_hack',

    section_order: ['.rodata', '.text', '.data', '.sdata', '.sbss', '.bss'],
    auto_link_sections: ['.data', '.rodata', '.sdata', '.sbss', '.bss'],

    // Follow YAML order exactly; do not reorder by section type.
    ld_legacy_generation: true,

    // Disable section alignment to match original binary layout.
    ld_align_section_vram_end: false,
    ld_align_segment_vram_end: false,

    symbol_addrs_path: ['symbol_addrs.txt'],
    reloc_addrs_path: ['reloc_addrs.txt'],

    undefined_funcs_auto_path: 'undefined_funcs.txt',
    undefined_syms_auto_path: 'undefined_syms.txt',

    extensions_path: 'tools/splat_ext',

    subalign: 2,

    string_encoding: 'ASCII',
    data_string_encoding: 'ASCII',
    rodata_string_guesser_level: 2,
    data_string_guesser_level: 2,
  },

  segments: [
    {
      name: 'header',
      type: 'header',
      start: Hex('0'),
    },

    {
      name: 'main',
      type: 'code',
      start: Hex('800'),
      vram: Hex('80010000'),
      bss_size: Hex('8278'),

      subsegments: [
        // =================================================================
        // Segments named per original compilation unit (10 units total).
        // Rodata and text segments sharing a name belong to the same .obj.
        // C segments retain their src/ paths (preserving matched code).
        //
        // PROPOSED FINER STRUCTURE (2026-06-14, comments only — no config
        // change yet). The 10 rodata-anchored units below are CONFIRMED real
        // object boundaries (rodata order == text order == link order; each
        // rodata sub-segment's first item is referenced by the text where that
        // unit begins). But the big *text* blobs (OBJECT, PLAYER_STATES,
        // VEHICLE) bundle several no-rodata source files that are invisible in
        // the rodata section. The "// PROPOSED:" blocks below subdivide those
        // blobs at real function starts. Evidence + full reasoning:
        // docs/architecture/compilation-units.md. Evidence tags:
        //   [R] rodata-anchored (hard)  [L] PSY-Q *_OBJ_* marker (hard)
        //   [N] subsystem name-cluster (strong)  [g] inter-function gap
        //   [A] addressing-mode boundary (strong — same symbol referenced via
        //       both gp_rel and lui+lo within the segment; see
        //       tools/find-tu-boundaries.py and docs/compiler-quirks.md).
        // To apply a block: replace the single active asm()/c() line with the
        // commented lines, then `make check` (should stay byte-identical since
        // splits are at function starts and migrate_rodata_to_functions=false).
        // =================================================================

        // -----------------------------------------------------------------
        // UNIT 1: Game/ENGINE — render/entity engine core (~107KB)
        // Boot code, graphics init, sprite/tilemap rendering, entity
        // lifecycle, resource management, vtable definitions.
        // -----------------------------------------------------------------
        asm('800', 'Game/ENGINE_boot'),        // hand-written memset, RLE decode
        c('B1C', 'crt0stub'),
        rodata('B24', 'Game/ENGINE'),          // entity vtables (16 archetype tables)
        rodata('E2C', 'Game/OBJECT'),
        rodata('1E28', 'Game/BOSS'),
        rodata('2044', 'Game/PLAYER'),
        rodata('2554', 'Game/PLAYER_STATES'),
        rodata('2940', 'Game/VEHICLE/vehicle'),
        rodata('2F58', 'Game/MAIN'),           // main game callbacks
        rodata('2F78', 'LIBCD'),
        rodata('34F4', 'LIBGPU'),
        rodata('3970', 'LIBSPU'),

        // --- .text ---

        // UNIT 1 continued (text body)
        c('39F0', 'gfx'),                      // graphics init, OT clear, buffer swap
        c('3EC8', 'prim'),                     // AllocPrim20..AllocPrim36 (7 funcs)
        asm('40F0', 'Game/ENGINE_40F0'),       // AllocateVRAMSlot onward
        c('5C34', 'vibrate'),
        asm('5C3C', 'Game/ENGINE_5C3C'),       // render init, tilemap, sprite context
        c('909C', 'spracc'),
        c('954C', 'nullfn'),
        asm('9554', 'Game/ENGINE_9554'),       // menu entity init, sprite object
        c('A8C8', 'entity'),                   // entity system core
        c('D880', 'sprset'),
        c('D8C0', 'anim'),                     // 0x8001D8C0 anim/sprite setters body
        c('11048', 'blb'),                     // 0x80020848 BLB load + entity lifecycle [A]
        // UNIT 1 split applied 2026-06-14: animation_setters had mixed addressing
        // [A] for g_pGameState (LoadBLBHeader @ 0x800208B0 used gp_rel, all 255
        // other references used lui+lw). The BLB-helper cluster
        // (BLB_ReadSectorsWrapper / InitEntityWithTable / LoadBLBHeader / entity
        // destruct + tick/render-loop family) is now in its own TU
        // (blb_runtime.c), matching the original per-file addressing-mode split.
        // Evidence: tools/find-tu-boundaries.py + Ghidra MCP xref audit.

        // -----------------------------------------------------------------
        // UNIT 2: Game/OBJECT — split into ~9 source files in link order
        // (evidence: docs/architecture/compilation-units.md). ROM offset; VRAM
        // in comment. Boss STATE MACHINES live in OBJECT/bosses below; the
        // c('48968','Game/BOSS/boss') unit further down is clayball/platform code.
        // -----------------------------------------------------------------
        asm('1AB78', 'Game/OBJECT/hud'),          // 0x8002A378 HUD + pause menu                   [R]
        asm('1C7F0', 'Game/OBJECT/entity_dtor'),  // 0x8002BFF0 generic entity destructors         [N]
        asm('1CFD8', 'Game/OBJECT/decor'),        // 0x8002C7D8 path/decor entities                [N]
        asm('1DC74', 'Game/OBJECT/collectibles'), // 0x8002D474 pickups (clayball/willie/phart...) [N]
        asm('2150C', 'Game/OBJECT/effects'),      // 0x80030D0C particles/grid/ripple/beam FX      [N]
        asm('291DC', 'Game/OBJECT/cd'),           // 0x800389DC game CD/BLB I/O + audio track      [N]
        asm('29950', 'Game/OBJECT/movie'),        // 0x80039150 STR movie streaming/decode         [N]
        // PROPOSED: movie has mixed addressing [A] for D_800A5A30 / D_800A5A24.
        // InitMovieStreamingBuffers @ 0x80039CE0 uses gp_rel, but it is wedged
        // between hilo siblings (DisplayLoadingScreen @ 0x800399A8 before, and
        // MovieFrameDecodeCallback @ 0x80039E5C after) so the original almost
        // certainly used a function-local `extern` shape inside Init*Buffers,
        // not a real TU boundary. Linker would not interleave .o text sections
        // to produce the observed function order from separate TUs. Action:
        // leave the segment as-is and reproduce the gp_rel store via a local
        // extern when decomping InitMovieStreamingBuffers (see compiler-quirks).
        asm('2AB94', 'Game/OBJECT/enemies'),      // 0x8003A394 enemy AI, projectiles, platforms   [N]
        asm('37A88', 'Game/OBJECT/bosses'),       // 0x80047288 Klogg/MonkeyMage/Glenn/Shriney/Joe [N]

        // -----------------------------------------------------------------
        // UNIT 3: Game/BOSS — clayball/circular-platform + player primitives
        // (originally bundled under 'Game/BOSS/boss'; split at the
        // CreatePlayerEntity boundary per [R] rodata anchor 0x80011628 +
        // function-name clustering — Clayball/Shriney funcs are in the lower
        // half, CreatePlayerEntity/CheckCollision* are in the upper half).
        // -----------------------------------------------------------------
        c('48968', 'Game/BOSS/clayball_platform'),  // 0x80058168 clayball + circular platform  [R]
        c('49EA4', 'Game/PLAYER/player'),           // 0x800596A4 CreatePlayerEntity + collision [N]

        // -----------------------------------------------------------------
        // UNIT 4: Game/PLAYER — player state machine, physics, input
        // -----------------------------------------------------------------
        asm('4AE30', 'Game/PLAYER'),               // 0x8005A630 player FSM (PlayerState_*/PlayerCallback_*)
        asm('5E808', 'Game/PLAYER/finn'),          // 0x8006E008 FINN/glide subentity                [N]
        c('617D8', 'Game/PLAYER/destructor_spu_at10c'),
        // PROPOSED: the PLAYER blob (0x8005A630-0x8006E008, ~80KB) is a single
        // asm() with no rodata anchor [R] inside it, so internal boundaries are
        // inferred only from function-name clusters [N]. No addressing-mode
        // mixing [A] has been detected for PLAYER yet — find-tu-boundaries.py
        // returns clean. Likely natural sub-units, in link order:
        //   asm('4AE30', 'Game/PLAYER/player_callbacks'), // 0x8005A630 PlayerProcessTileCollision, PlayerTickCallback, PlayerApplyPositionWithCollision  [N]
        //   asm('XXXXX', 'Game/PLAYER/player_states'),    // 0x8005XXXX PlayerState_* FSM nodes  [N]
        //   asm('YYYYY', 'Game/PLAYER/player_sound'),     // 0x8005YYYY player SFX/voice helpers [N]
        // Concrete addresses TBD — needs symbol-cluster scan (similar to the
        // PLAYER_STATES proposed split below). Update when investigated.

        // -----------------------------------------------------------------
        // UNIT 5: Game/PLAYER_STATES blob — split into vehicle + UI/level/audio
        // source files in link order (docs/architecture/compilation-units.md).
        // -----------------------------------------------------------------
        // player_states keeps c() (has matched C: PlatformEvent_*, func_80071778);
        // the rest are unmatched code so they are asm() (whole-range extraction,
        // robust against anonymous funcs + jump tables) until decomp converts them.
        c('61848', 'Game/PLAYER_STATES/player_states'),  // 0x80071048 Runn/Soar/Finn vehicle modes + platforms [R]
        asm('65798', 'Game/UI/menu'),                    // 0x80074F98 cursor/buttons/options/level-select
        asm('667F4', 'Game/UI/password'),                // 0x80075FF4 password entry UI
        asm('687E8', 'Game/UI/hud_results'),             // 0x80077FE8 HUD digits + results screen
        asm('69E3C', 'Game/UI/ending'),                  // 0x8007963C ending / credits
        asm('6A9BC', 'Game/MAIN/level'),                 // 0x8007A1BC level-data ctx + playback sequence
        asm('6B1B0', 'Game/MAIN/blb_accessors'),         // 0x8007A9B0 BLB/level/tile/sprite TOC getters
        asm('6C7B8', 'Game/AUDIO/sound'),                // 0x8007BFB8 SPU upload, SFX, voice, CD audio
        asm('6D4FC', 'Game/MAIN/gamestate'),             // 0x8007CCFC InitGameState/respawn/level-start

        // -----------------------------------------------------------------
        // UNIT 6: Game/MAIN — level lifecycle, entity init table, main loop
        //   APPLIED: was Game/VEHICLE/vehicle (single 0x55C0 c()); rodata anchor
        //   0x80012140 [R] revealed this is level lifecycle code, not vehicles.
        //   Split into 3 TUs at function-boundary addresses verified by both
        //   tools/find-tu-boundaries.py and Ghidra MCP xref audit.
        // -----------------------------------------------------------------
        c('6D9D0', 'Game/MAIN/level_load'),       // 0x8007D1D0 load/setup level, game-mode loop  [R]
        c('6F7D0', 'Game/MAIN/entity_init'),      // 0x8007EFD0 ~120 EntityType###_*_Init + remap [N]
        c('728B4', 'Game/MAIN/main'),             // 0x800820B4 cheats, main(), debug menu        [N]
        c('73690', 'Game/VEHICLE/static_game_state'),
        c('736E0', 'Game/VEHICLE/empty_callbacks'),

        // -----------------------------------------------------------------
        // UNIT 7: Game/MAIN — main(), menus, passwords, audio, level loading
        // -----------------------------------------------------------------
        c('736F0', 'Game/MAIN/blb_memory'),
        asm('73754', 'Game/MAIN'),
        c('73794', 'Game/MAIN/memmove'),
        // PROPOSED: unit is mostly already carved — the matched C files
        // blb_memory (0x736F0) and memmove (0x73794) bracket a 64-byte asm
        // remainder. No mixed-mode addressing [A] flagged for this range, and
        // no rodata anchors [R] inside Game/MAIN beyond what UNIT 1 already
        // owns (rodata at 0x2F58 is for the MAIN unit but lives upstream).
        // No further subdivision currently proposed; leave as-is until either
        // (a) the 64-byte 'Game/MAIN' asm stub is decompiled, or (b) other
        // evidence emerges.

        // -----------------------------------------------------------------
        // UNIT 8: LIBCD — PSY-Q CD-ROM library
        // -----------------------------------------------------------------
        c('73800', 'LIBCD/libcd'),
        // PROPOSED: PSY-Q *_OBJ_* symbol prefixes [L] mark real library object
        // boundaries. LIBCD spans several internal objects, and a LIBETC unit
        // (pad+vsync+intr) sits between LIBCD and LIBGPU:
        //   c('73800', 'LIBCD/libcd'),                 // 0x80083000 CdInit/CdControl/CdSync API       [L]
        //   asm('74150', 'LIBCD/bios_cdrom'),          // 0x80083950 BIOS_OBJ_*/CD_* cdrom internals   [L]
        //   asm('758E4', 'LIBCD/iso9660'),             // 0x800850E4 CdSearchFile/ISO9660_OBJ_*         [L]
        //   asm('76204', 'LIBCD/cdread'),              // 0x80085A04 CdRead/CDREAD_OBJ_*                [L]
        //   asm('76E0C', 'LIBCD/stream'),              // 0x8008660C St*/C_011_OBJ_*/dma_execute        [L]
        //   asm('77E2C', 'LIBETC/pad'),                // 0x8008762C PadInit/PadRead/send_pad           [L]
        //   asm('78424', 'LIBETC/vsync_intr'),         // 0x80087C24 VSync/*Intr*/FlushCache/setjmp     [L]
        //   asm('79158', 'LIBGPU/gpu_font'),           // 0x80088958 SetVideoMode/LoadClut/Fnt*         [L]

        // -----------------------------------------------------------------
        // UNIT 9: LIBGPU — PSY-Q GPU library
        // -----------------------------------------------------------------
        asm('79AE4', 'LIBGPU'),
        // PROPOSED: this segment is the PSY-Q libgpu.lib body (~26KB ending at
        // LIBSPU's start at ROM 0x80260). PSY-Q libraries are archives of many
        // small .obj files marked by *_OBJ_* symbols [L]. Known anchors here
        // from symbol_addrs.txt:
        //   ResetGraph  @ 0x80089DD8   (init)
        //   SetDispMask @ 0x8008A1FC   (env)
        //   DrawSync    @ 0x8008A298   (sync)
        //   ClearImage  @ 0x8008A42C   (image_ops)
        //   LoadImage   @ 0x8008A55C
        //   StoreImage  @ 0x8008A5C0
        //   MoveImage   @ 0x8008A624
        //   PutDrawEnv  @ 0x8008A90C   (env_apply)
        //   PutDispEnv  @ 0x8008AB64
        // Splitting along *_OBJ_* boundaries (look for PSY-Q '_OBJ_' prefixed
        // labels in the disassembly) would let a future libgpu re-link match
        // byte-for-byte against a recovered PSY-Q SDK. Defer until a candidate
        // libgpu.lib source is identified; addresses TBD.

        // -----------------------------------------------------------------
        // UNIT 10: LIBSPU — PSY-Q SPU library
        // -----------------------------------------------------------------
        asm('80260', 'LIBSPU'),
        c('80F24', 'LIBSPU/libspu_voice'),
        asm('80FD0', 'LIBSPU_tail'),
        // PROPOSED: LIBSPU is already partially carved (libspu_voice.c is
        // matched C, with asm head/tail pieces around it). Like LIBGPU above,
        // the head asm('80260') and tail asm('80FD0') correspond to multiple
        // PSY-Q *_OBJ_* archive members [L]. Known anchors:
        //   SpuInit  @ 0x8008E81C  (in 'LIBSPU' head — note: this lives BEFORE
        //                           ROM 0x80260, so it actually belongs to the
        //                           tail of the 'LIBGPU' segment above. The
        //                           current LIBGPU/LIBSPU boundary may be off
        //                           by one *_OBJ_* — worth re-anchoring.)
        //   SpuStart @ 0x8008E914
        // Action: when investigating LIBGPU per the proposal above, re-derive
        // the true LIBSPU start by walking back from SpuInit's *_OBJ_* marker
        // and adjust both segments accordingly. Until then leave as-is — the
        // current split byte-matches even if the naming is slightly off.

        // =====================================================================
        // .data section: 0x80090FEC - 0x800A5953
        // =====================================================================
        data('80FEC', 'data'),

        // =====================================================================
        // .sdata section: 0x800A5954 - 0x800A611F (GP-relative small data)
        // =====================================================================
        data('96154', 'sdata'),

        // =====================================================================
        // .sbss section: 0x800A6120 - 0x800AD953 (GP-relative uninitialized data)
        // .bss section: 0x800AD954 - 0x800AE397 (regular uninitialized data)
        // Total bss_size: 0x8278
        // =====================================================================
        bss('96920', 'sbss', '800A6120'),
        bss('96920', 'bss', '800AD954'),
      ],
    },

    [Hex('97000')],  // End of file (618496 bytes)
  ],
}
