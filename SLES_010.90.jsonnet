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
local sdata(start, name) = subsegment(start, 'sdata', name);
local dotsdata(start, name) = subsegment(start, '.sdata', name);
local data(start, kind) = subsegment(start, kind);
local dotdata(start, name) = subsegment(start, '.data', name);
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
        // Flat lowercase TU layout (Glover / 1990s PSY-Q convention).
        // Each rodata segment anchors the .obj boundary of the text segment
        // that shares its name (rodata order == text order == link order).
        //
        // Evidence tags (see docs/architecture/compilation-units.md):
        //   [R] rodata-anchored (hard)  [L] PSY-Q *_OBJ_* marker (hard)
        //   [N] subsystem name-cluster (strong)  [g] inter-function gap
        //   [A] addressing-mode boundary (strong — same symbol referenced via
        //       both gp_rel and lui+lo within the segment; see
        //       tools/find-tu-boundaries.py and docs/compiler-quirks.md).
        // =================================================================

        asm('800', 'crt0'),                    // hand-written memset, RLE decode
        c('B1C', 'crt0stub'),
        rodata('B24', 'gfx'),                  // entity vtables (16 archetype tables)
        rodata('E2C', 'hud'),
        rodata('1E28', 'clayball'),
        rodata('2044', 'playst'),
        rodata('2554', 'vehicle'),
        rodata('2940', 'lvlload'),
        rodata('2F58', 'libc'),                // main game callbacks
        rodata('2F78', 'libs/libcd'),
        rodata('34F4', 'libs/libgpu'),
        rodata('3970', 'libs/libspu'),

        // --- .text ---

        c('39F0', 'gfx'),                      // graphics init, OT clear, buffer swap
        c('3EC8', 'prim'),                     // AllocPrim20..AllocPrim36 (7 funcs)
        c('40F0', 'vram'),                     // AllocateVRAMSlot onward
        c('5C34', 'vibrate'),
        c('5C3C', 'sprite'),                   // render init, tilemap, sprite context
        c('909C', 'spracc'),
        c('954C', 'nullfn'),
        c('9554', 'layer'),                    // menu entity init, sprite object
        c('A8C8', 'entity'),                   // entity system core
        c('D880', 'sprset'),
        c('D8C0', 'anim'),                     // 0x8001D8C0 anim/sprite setters body
        c('11048', 'blb'),                     // 0x80020848 BLB load + entity lifecycle [A]
        c('1AB78', 'hud'),            // 0x8002A378 HUD + pause menu                   [R]
        c('1C7F0', 'edtor'),          // 0x8002BFF0 generic entity destructors         [N]
        c('1CFD8', 'decor'),          // 0x8002C7D8 path/decor entities                [N]
        c('1DC74', 'pickups'),        // 0x8002D474 pickups (clayball/willie/phart...) [N]
        c('2150C', 'effects'),        // 0x80030D0C particles/grid/ripple/beam FX      [N]
        c('291DC', 'gamecd'),         // 0x800389DC game CD/BLB I/O + audio track      [N]
        c('29950', 'movie'),          // 0x80039150 STR movie streaming/decode         [N]
        c('2AB94', 'enemies'),        // 0x8003A394 enemy AI, projectiles, platforms   [N]
        c('37A88', 'bosses'),         // 0x80047288 Klogg/MonkeyMage/Glenn/Shriney/Joe [N]
        c('48968', 'clayball'),                     // 0x80058168 clayball + circular platform  [R]
        c('49EA4', 'player'),                       // 0x800596A4 CreatePlayerEntity + collision [N]
        c('4AE30', 'playst'),                      // 0x8005A630 player FSM (PlayerState_*/PlayerCallback_*)
        c('5E808', 'finn'),                        // 0x8006E008 FINN/glide subentity                [N]
        c('617D8', 'playdtor'),
        c('61848', 'vehicle'),                           // 0x80071048 Runn/Soar/Finn vehicle modes + platforms [R]
        c('65798', 'menu'),                              // 0x80074F98 cursor/buttons/options/level-select
        c('667F4', 'passwd'),                            // 0x80075FF4 password entry UI
        c('687E8', 'results'),                           // 0x80077FE8 HUD digits + results screen
        c('69E3C', 'ending'),                            // 0x8007963C ending / credits
        c('6A9BC', 'level'),                             // 0x8007A1BC level-data ctx + playback sequence
        c('6B1B0', 'blbacc'),                          // 0x8007A9B0 BLB/level/tile/sprite TOC getters
        c('6C7B8', 'sound'),                             // 0x8007BFB8 SPU upload, SFX, voice, CD audio
        c('6D4FC', 'gstate'),                            // 0x8007CCFC InitGameState/respawn/level-start
        c('6D9D0', 'lvlload'),                    // 0x8007D1D0 load/setup level, game-mode loop  [R]
        c('6F7D0', 'entinit'),                    // 0x8007EFD0 ~120 EntityType###_*_Init + remap [N]
        c('728B4', 'main'),                       // 0x800820B4 cheats, main(), debug menu        [N]
        c('73690', 'gstctor'),
        c('736E0', 'emptycb'),
        c('736F0', 'blbmem'),
        c('73754', 'libc'),
        c('73794', 'memmove'),

        // =====================================================================
        // PSY-Q - LEAVE AS IS - NO POINT YET IN TOUCHING SONY LIBRARY
        // =====================================================================
        c('73800', 'libs/libcd'),
        asm('79AE4', 'libs/libgpu'),
        asm('80260', 'libs/libspu'),
        c('80F24', 'libs/libvoice'),
        asm('80FD0', 'libs/libspu2'),

        // =====================================================================
        // .data section: 0x80090FEC - 0x800A5953
        // =====================================================================
        data('80FEC', 'data'),         // 0x800907EC zero region up to first table
        // Phase 4 .data migration: carve per-TU pure-data islands (every member
        // >8 bytes, non-zero, single-owner, no relocs) out of the pooled blob
        // into build/src/<TU>.o(.data). Gaps and mixed/zero/tiny runs stay asm.
        dotdata('8B640', 'vram'),      // 0x8009AE40 (24B)  -> build/src/vram.o(.data)
        data('8B658', 'data'),         // 0x8009AE58 layer scratch buffer + gap (asm)
        dotdata('8B838', 'blb'),       // 0x8009B038 (268B) -> build/src/blb.o(.data)
        data('8B944', 'data'),         // 0x8009B144 gap (asm)
        dotdata('8BA40', 'decor'),     // 0x8009B240 (132B incl 2B pad; D_8009B242 aliased) -> build/src/decor.o(.data)
        dotdata('8BAC4', 'pickups'),   // 0x8009B2C4 (220B: D_8009B2C4 2B + D_8009B2C6 218B grouped) -> build/src/pickups.o(.data)
        dotdata('8BBA0', 'effects'),   // 0x8009B3A0 (56B)  -> build/src/effects.o(.data)
        data('8BBD8', 'data'),         // 0x8009B3D8 gap (asm)
        dotdata('8BC3C', 'gamecd'),    // 0x8009B43C (144B) -> build/src/gamecd.o(.data)
        data('8BCCC', 'data'),         // 0x8009B4CC gap up to bosses island (asm)
        dotdata('8C408', 'bosses'),    // 0x8009BC08 (1388B: 9 boss cos/sin + hitbox tables grouped, D_8009BC0A..D_8009C11C aliased) -> build/src/bosses.o(.data)
        data('8C974', 'data'),         // 0x8009C174 gap up to finn island (asm)
        dotdata('8D234', 'finn'),      // 0x8009CA34 (168B: finn rotation sprite tables grouped, D_8009CA7C/D_8009CABC aliased) -> build/src/finn.o(.data)
        dotdata('8D2DC', 'vehicle'),   // 0x8009CADC (36B: vehicle config tables grouped, D_8009CAEC aliased) -> build/src/vehicle.o(.data)
        dotdata('8D300', 'passwd'),    // 0x8009CB00 (124B: passwd tables grouped, 7 .set aliases) -> build/src/passwd.o(.data)
        dotdata('8D37C', 'results'),   // 0x8009CB7C (48B: results tables grouped, D_8009CB7E aliased) -> build/src/results.o(.data)
        data('8D3AC', 'data'),         // 0x8009CBAC gap up to sound island (asm)
        dotdata('8D418', 'sound'),     // 0x8009CC18 (1552B: sound bank/voice tables grouped, D_8009CC2F..D_8009D224 aliased) -> build/src/sound.o(.data)
        data('8DA28', 'data'),         // 0x8009D228 gap up to entinit island (asm)
        dotdata('8E1FC', 'entinit'),   // 0x8009D9FC (228B: collectible/enemy config + sprite hash tables grouped, D_8009DA18..D_8009DAB4 aliased) -> build/src/entinit.o(.data)
        dotdata('8E2E0', 'main'),      // 0x8009DAE0 (352B: main .data island grouped) -> build/src/main.o(.data)
        data('8E440', 'data'),         // 0x8009DC40 remainder of .data (asm)

        // =====================================================================
        // .sdata section: 0x800A5954 - 0x800A611F (GP-relative small data)
        // Phase 1 (sdata-under-split): the pooled blob is split into per-TU
        // NAMED asm subsegments at the single-owner boundaries found by
        // tools/map_sdata_ownership.py. Still asm, still byte-neutral — this is
        // a pure relabel that gives every future data migration a clean per-TU
        // target. Boundaries verified contiguous & symbol-aligned; the leading
        // engine-globals block and the interleaved ending..main tail stay in
        // unnamed shared pieces (cannot be attributed to a single TU).
        // =====================================================================
        data('96154', 'sdata'),        // 0x800A5954 shared engine globals (g_pBlbHeapBase, g_pGameState, D_800A595C, ...)
        dotsdata('96180', 'blb'),      // 0x800A5980 -> MIGRATED to build/src/blb.o(.sdata) (Phase 2)
        dotsdata('961A8', 'pickups'),  // 0x800A59A8 -> MIGRATED to build/src/pickups.o(.sdata) (Phase 4)
        sdata('961E8', 'gamecd'),      // 0x800A59E8
        sdata('96210', 'movie'),       // 0x800A5A10
        dotsdata('96250', 'enemies'),  // 0x800A5A50 -> MIGRATED to build/src/enemies.o(.sdata) (Phase 4)
        dotsdata('96360', 'bosses'),   // 0x800A5B60 -> MIGRATED to build/src/bosses.o(.sdata) (Phase 4)
        data('96510', 'sdata'),        // 0x800A5D10 unowned gap (16B)
        dotsdata('96520', 'player'),   // 0x800A5D20 -> MIGRATED to build/src/player.o(.sdata) (Phase 4)
        dotsdata('96538', 'playst'),   // 0x800A5D38 -> MIGRATED to build/src/playst.o(.sdata) (Phase 4)
        dotsdata('96764', 'finn'),     // 0x800A5F64 -> MIGRATED to build/src/finn.o(.sdata) (Phase 4)
        dotsdata('967AC', 'vehicle'),  // 0x800A5FAC -> MIGRATED to build/src/vehicle.o(.sdata) (Phase 4)
        data('9683C', 'sdata'),        // 0x800A603C interleaved ending/menu/passwd/level/blbacc/sound/lvlload/main tail

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
