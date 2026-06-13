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
        // =====================================================================
        // .rodata section: 0x80010000 - 0x800131EF
        // NOTE: Original binary has code bytes stored in rodata section!
        // These are NOT actual rodata - they are code bytes the linker placed here.
        // =====================================================================
        asm('800', 'Game/INIT'),
        c('B1C', 'system/early_stub'),
        asm('B24', 'Game/INIT_TABLES'),
        rodata('E2C', 'Game/OBJECT'),  // OBJECT switch/entity callback tables (0x80010624)
        rodata('1E28', 'Game/PLAYER_EARLY'),  // PLAYER early rodata (0x80011628)
        rodata('2044', 'Game/PLAYER'),  // PLAYER aligned rodata split (0x80011844)
        rodata('2554', 'Game/GAMELOOP_EARLY'),  // GAMELOOP early rodata (0x80011D54)
        rodata('2940', 'Game/GAMELOOP'),  // GAMELOOP aligned rodata split (0x80012140)

        // PSY-Q library rodata: jump tables, strings.
        rodata('2F58', 'assets/blb_memory'),  // BLB entity callback table (0x80012758)
        rodata('2F78', 'LIBCD'),  // libcd rodata
        rodata('34F4', 'LIBGPU'),  // libgpu rodata
        rodata('3970', 'LIBSPU'),  // libspu rodata

        // =====================================================================
        // .text section: 0x800131F0 - 0x80090FEB
        // Game code organized by subsystem (soul-re style uppercase names)
        // =====================================================================

        // --- RENDER: Graphics system, VRAM, tiles, sprites (0x800131F0 - 0x8001A0C7) ---
        // InitGraphicsSystem, VRAM slots, tile rendering, sprite rendering.
        asm('39F0', 'Game/RENDER'),  // all stubs via INCLUDE_ASM
        c('5C34', 'render/stub_vibrate_off'),
        asm('5C3C', 'Game/RENDER_5C3C'),
        c('93D8', 'render/sprite_accessors'),
        c('954C', 'render/empty_stub_18d4c'),
        asm('9554', 'Game/RENDER_9554'),

        // --- ENTITY: Entity system core (0x8001A0C8 - 0x8002A377) ---
        // InitEntityStruct, animation, collision, state machine, callbacks.
        asm('A8C8', 'Game/ENTITY'),

        // --- OBJECT: Game objects - enemies, items, decor (0x8002A378 - 0x80058167) ---
        // HUD, decor entities, collectibles, enemies, projectiles, bosses.
        asm('1AB78', 'Game/OBJECT'),

        // --- PLAYER: Player character system (0x80058168 - 0x80071047) ---
        // PlayerTickCallback, state machine, physics, input handling.
        asm('48968', 'Game/PLAYER_EARLY'),
        asm('4AE30', 'Game/PLAYER'),
        c('617D8', 'entity/destructor_spu_at10c'),

        // --- GAMELOOP: Main game flow, menus, level loading (0x80071048 - 0x80082FFF) ---
        // Tile collision, FINN vehicle, menu system, BLB loading, main().
        asm('61848', 'Game/GAMELOOP_EARLY'),
        asm('6D9D0', 'Game/GAMELOOP'),
        c('73690', 'world/static_game_state'),
        c('736E0', 'system/empty_callbacks'),
        c('736F0', 'assets/blb_memory'),
        asm('73754', 'libs/bios_trampolines'),
        c('73794', 'libs/memmove'),

        // --- LIBS: PSY-Q SDK libraries (0x80083000 - 0x80090FEB) ---
        // libcd, libgpu, libgte, libspu, libetc.
        asm('73800', 'LIBCD'),
        asm('79AE4', 'LIBGPU'),
        asm('80260', 'LIBSPU'),
        c('80F24', 'libs/libspu_voice'),
        asm('80FD0', 'LIBS_80FD0'),

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
