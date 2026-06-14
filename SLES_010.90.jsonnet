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
        c('B1C', 'Game/ENGINE/early_stub'),
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
        c('39F0', 'Game/ENGINE'),              // graphics init, OT clear, buffer swap
        c('3EC8', 'Game/ENGINE/prim_alloc'),   // AllocPrim20..AllocPrim36 (7 funcs)
        asm('40F0', 'Game/ENGINE_40F0'),       // AllocateVRAMSlot onward
        c('5C34', 'Game/ENGINE/stub_vibrate_off'),
        asm('5C3C', 'Game/ENGINE_5C3C'),       // render init, tilemap, sprite context
        c('909C', 'Game/ENGINE/sprite_accessors'),
        c('954C', 'Game/ENGINE/empty_stub_18d4c'),
        asm('9554', 'Game/ENGINE_9554'),       // menu entity init, sprite object
        c('A8C8', 'Game/ENGINE/entity_system'),  // entity system core
        c('D880', 'Game/ENGINE/sprite_setters'),
        c('D8C0', 'Game/ENGINE/animation_setters'),

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
        asm('2AB94', 'Game/OBJECT/enemies'),      // 0x8003A394 enemy AI, projectiles, platforms   [N]
        asm('37A88', 'Game/OBJECT/bosses'),       // 0x80047288 Klogg/MonkeyMage/Glenn/Shriney/Joe [N]

        // -----------------------------------------------------------------
        // UNIT 3: Game/BOSS — boss state machines (ShrineyGuard, JoeHeadJoe, Klogg)
        // -----------------------------------------------------------------
        c('48968', 'Game/BOSS/boss'),
        // PROPOSED: name is misleading. Rodata anchor 0x80011628 [R] confirms a
        // real object here, but its code is clayball/circular-platform/Shriney-
        // sound, NOT the boss FSMs (those are in Game/OBJECT/bosses above).
        // player.c create/collision primitives likely begin mid-segment:
        //   c('48968', 'Game/BOSS/clayball_platform'),// 0x80058168 clayball + circular platform     [R]
        //   asm('49EA4', 'Game/PLAYER/player'),        // 0x800596A4 CreatePlayerEntity, collision    [N]

        // -----------------------------------------------------------------
        // UNIT 4: Game/PLAYER — player state machine, physics, input
        // -----------------------------------------------------------------
        asm('4AE30', 'Game/PLAYER'),
        c('617D8', 'Game/PLAYER/destructor_spu_at10c'),
        // PROPOSED: PLAYER (0x8005A630 [R]) is the big player FSM
        // (PlayerTickCallback, PlayerState_*, PlayerCallback_*). FINN vehicle +
        // glide subentity split off near the end:
        //   asm('5E808', 'Game/PLAYER/finn'),          // 0x8006E008 Finn/FINN subentity + glide      [N]

        // -----------------------------------------------------------------
        // UNIT 5: Game/PLAYER_STATES — player platform state machine (~50KB blob)
        // -----------------------------------------------------------------
        c('61848', 'Game/PLAYER_STATES/player_states'),
        // PROPOSED: this blob is really ~9 source files in link order:
        //   c('61848', 'Game/PLAYER_STATES/vehicle'),  // 0x80071048 Runn/Soar/Finn modes + platforms [R]
        //   asm('65798', 'Game/UI/menu'),              // 0x80074F98 cursor/buttons/options/lvl-select [N]
        //   asm('667F4', 'Game/UI/password'),          // 0x80075FF4 password entry UI                 [N]
        //   asm('687E8', 'Game/UI/hud_results'),       // 0x80077FE8 HUD digits + results screen       [N]
        //   asm('69E3C', 'Game/UI/ending'),            // 0x8007963C ending / credits                  [N]
        //   asm('6A9BC', 'Game/MAIN/level'),           // 0x8007A1BC level-data ctx + playback seq     [N]
        //   asm('6B1B0', 'Game/MAIN/blb_accessors'),   // 0x8007A9B0 ~120 BLB/level/tile/sprite getters[N]
        //   asm('6C7B8', 'Game/AUDIO/sound'),          // 0x8007BFB8 SPU upload, SFX, voice, CD audio  [N]
        //   asm('6D4FC', 'Game/MAIN/gamestate'),       // 0x8007CCFC InitGameState, respawn, lvl start [N]

        // -----------------------------------------------------------------
        // UNIT 6: Game/VEHICLE — vehicle modes (FINN/RUNN/SOAR), tile collision
        // -----------------------------------------------------------------
        c('6D9D0', 'Game/VEHICLE/vehicle'),
        c('73690', 'Game/VEHICLE/static_game_state'),
        c('736E0', 'Game/VEHICLE/empty_callbacks'),
        // PROPOSED: this unit (rodata anchor 0x80012140 [R]) is level lifecycle,
        // not vehicle code. Really ~3 files:
        //   c('6D9D0', 'Game/MAIN/level_load'),        // 0x8007D1D0 load/setup level, game-mode loop  [R]
        //   asm('6F7D0', 'Game/MAIN/entity_init'),     // 0x8007EFD0 ~120 EntityType###_*_Init + remap [N]
        //   asm('728B4', 'Game/MAIN/main'),            // 0x800820B4 cheats, main(), debug menu        [N]

        // -----------------------------------------------------------------
        // UNIT 7: Game/MAIN — main(), menus, passwords, audio, level loading
        // -----------------------------------------------------------------
        c('736F0', 'Game/MAIN/blb_memory'),
        asm('73754', 'Game/MAIN'),
        c('73794', 'Game/MAIN/memmove'),

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

        // -----------------------------------------------------------------
        // UNIT 10: LIBSPU — PSY-Q SPU library
        // -----------------------------------------------------------------
        asm('80260', 'LIBSPU'),
        c('80F24', 'LIBSPU/libspu_voice'),
        asm('80FD0', 'LIBSPU_tail'),

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
