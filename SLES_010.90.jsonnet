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
        // =================================================================

        // -----------------------------------------------------------------
        // UNIT 1: Game/ENGINE — render/entity engine core (~107KB)
        // Boot code, graphics init, sprite/tilemap rendering, entity
        // lifecycle, resource management, vtable definitions.
        // -----------------------------------------------------------------
        asm('800', 'Game/ENGINE_boot'),        // hand-written memset, RLE decode
        c('B1C', 'system/early_stub'),
        rodata('B24', 'Game/ENGINE'),          // entity vtables (16 archetype tables)
        rodata('E2C', 'Game/OBJECT'),
        rodata('1E28', 'Game/BOSS'),
        rodata('2044', 'Game/PLAYER'),
        rodata('2554', 'Game/PLAYER_STATES'),
        rodata('2940', 'Game/VEHICLE'),
        rodata('2F58', 'Game/MAIN'),           // main game callbacks
        rodata('2F78', 'LIBCD'),
        rodata('34F4', 'LIBGPU'),
        rodata('3970', 'LIBSPU'),

        // --- .text ---

        // UNIT 1 continued (text body)
        asm('39F0', 'Game/ENGINE'),            // graphics, VRAM, sprites, tiles
        c('5C34', 'render/stub_vibrate_off'),
        asm('5C3C', 'Game/ENGINE_5C3C'),       // render init, tilemap, sprite context
        c('909C', 'render/sprite_accessors'),
        c('954C', 'render/empty_stub_18d4c'),
        asm('9554', 'Game/ENGINE_9554'),       // menu entity init, sprite object
        asm('A8C8', 'Game/ENGINE_ENTITY'),     // entity system core
        c('D880', 'entity/sprite_setters'),
        c('D8C0', 'entity/animation_setters'),

        // -----------------------------------------------------------------
        // UNIT 2: Game/OBJECT — enemies, items, decor, bosses
        // -----------------------------------------------------------------
        asm('1AB78', 'Game/OBJECT'),

        // -----------------------------------------------------------------
        // UNIT 3: Game/BOSS — boss state machines (ShrineyGuard, JoeHeadJoe, Klogg)
        // -----------------------------------------------------------------
        asm('48968', 'Game/BOSS'),

        // -----------------------------------------------------------------
        // UNIT 4: Game/PLAYER — player state machine, physics, input
        // -----------------------------------------------------------------
        asm('4AE30', 'Game/PLAYER'),
        c('617D8', 'entity/destructor_spu_at10c'),

        // -----------------------------------------------------------------
        // UNIT 5: Game/PLAYER_STATES — player platform state machine
        // -----------------------------------------------------------------
        asm('61848', 'Game/PLAYER_STATES'),

        // -----------------------------------------------------------------
        // UNIT 6: Game/VEHICLE — vehicle modes (FINN/RUNN/SOAR), tile collision
        // -----------------------------------------------------------------
        asm('6D9D0', 'Game/VEHICLE'),
        c('73690', 'world/static_game_state'),
        c('736E0', 'system/empty_callbacks'),

        // -----------------------------------------------------------------
        // UNIT 7: Game/MAIN — main(), menus, passwords, audio, level loading
        // -----------------------------------------------------------------
        c('736F0', 'assets/blb_memory'),
        asm('73754', 'Game/MAIN'),
        c('73794', 'libs/memmove'),

        // -----------------------------------------------------------------
        // UNIT 8: LIBCD — PSY-Q CD-ROM library
        // -----------------------------------------------------------------
        asm('73800', 'LIBCD'),

        // -----------------------------------------------------------------
        // UNIT 9: LIBGPU — PSY-Q GPU library
        // -----------------------------------------------------------------
        asm('79AE4', 'LIBGPU'),

        // -----------------------------------------------------------------
        // UNIT 10: LIBSPU — PSY-Q SPU library
        // -----------------------------------------------------------------
        asm('80260', 'LIBSPU'),
        c('80F24', 'libs/libspu_voice'),
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
