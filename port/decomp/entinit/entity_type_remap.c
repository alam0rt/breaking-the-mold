/* =============================================================================
 * entity_type_remap.c  --  PC-port entity-type remapper (TARGET_PC)
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/entinit/RemapEntityTypesForLevel.s
 * (0x80081570, 549 lines; reference = export/SLES_010.90.c 114060). Walks the
 * level's raw spawn list (GameState+0x28, the 24-byte EntityDefinition records
 * from BLB asset 501, chained as {next @ +0x0, def @ +0x4}) and rewrites each
 * definition's runtime entity-type id (u16 @ def+0x12) from the BLB "authoring"
 * id into the engine's internal id, dispatched by the record's category byte
 * (def+0x14): 1 = generic actor, 2 = enemy/boss, 3 = decoration. Category 2's
 * authoring ids 0xC9..0xE4 are pass-through bosses (def+0xC receives the id,
 * type becomes 0x5F); category-2 id 0xA7 flags a JOE_HEAD-style boss player when
 * the level carries flag 0x2000 (GameState+0x18E). Every unmapped id falls
 * through to the generic type 8.
 *
 * The mapping tables are pure data transcribed case-for-case from the Ghidra
 * decompile; they encode the game's authoring->runtime id dictionary.
 * ========================================================================== */
#include "common.h"
#include <stdint.h>

extern u16 GetLevelFlags(void *ctx);

void RemapEntityTypesForLevel(void *arg0) {
    u8 *gs = (u8 *)arg0;
    s32 *node = *(s32 **)(gs + 0x28);      /* entity_spawn_list head */

    gs[0x18E] = 0;                          /* boss_player_type = KLOGG */

    for (; node != NULL; node = (s32 *)*node) {
        u8 *def = (u8 *)(uintptr_t)node[1];
        u8  cat = def[0x14];
        u16 *typ = (u16 *)(def + 0x12);

        if (cat == 2) {
            u32 id = *(u16 *)(def + 0x12);
            if (id - 0xC9 < 0x1C) {         /* 0xC9..0xE4: pass-through boss */
                *(u32 *)(def + 0xC) = id;
                *typ = 0x5F;
                continue;
            }
            switch (id) {
            case 0x02: *typ = 0x07; break;
            case 0x03: *typ = 0x02; break;
            case 0x08: *typ = 0x16; break;
            case 0x09: *typ = 0x0B; break;
            case 0x0A: *typ = 0x09; break;
            case 0x0B: *typ = 0x4B; break;
            case 0x0C: *typ = 0x3D; break;
            case 0x0D: *typ = 0x19; break;
            case 0x0E: *typ = 0x48; break;
            case 0x10: *typ = 0x17; break;
            case 0x15: *typ = 0x1C; break;
            case 0x16: *typ = 0x1D; break;
            case 0x17: *typ = 0x45; break;
            case 0x18: *typ = 0x46; break;
            case 0x19: *typ = 0x4F; break;
            case 0x1A: *typ = 0x51; break;
            case 0x1B: *typ = 0x61; break;
            case 0x1C: *typ = 0x59; break;
            case 0x1D: *typ = 0x62; break;
            case 0x1E: *typ = 0x6E; break;
            case 0x1F: *typ = 0x6F; break;
            case 0x29: *typ = 0x53; break;
            case 0x2A: *typ = 0x55; break;
            case 0x2B: *typ = 0x68; break;
            case 0x2C: *typ = 0x69; break;
            case 0x2D: *typ = 0x77; break;
            case 0x3D: *typ = 0x5E; break;
            case 0x3E: *(u32 *)(def + 0xC) = 0xC9; *typ = 0x5F; break;
            case 0x51: *typ = 0x41; break;
            case 0xA7:
                if ((GetLevelFlags(gs + 0x84) & 0x2000) != 0) {
                    gs[0x18E] = 1;
                }
                break;
            default: *typ = 8; break;
            }
        } else if (cat < 3) {
            if (cat == 1) {
                switch (*(u16 *)(def + 0x12)) {
                case 0x05: *typ = 0x70; break;
                case 0x06: *typ = 0x71; break;
                case 0x07: *typ = 0x72; break;
                case 0x08: *typ = 0x73; break;
                case 0x09: *typ = 0x74; break;
                case 0x0A: *typ = 0x75; break;
                case 0x15: *typ = 0x76; break;
                case 0x16: *typ = 0x1B; break;
                case 0x17: *typ = 0x0A; break;
                case 0x18: *typ = 0x1A; break;
                case 0x19: *typ = 0x00; break;
                case 0x1A: *typ = 0x03; break;
                case 0x1B: *typ = 0x04; break;
                case 0x1C: *typ = 0x29; break;
                case 0x1D: *typ = 0x63; break;
                case 0x1E: *typ = 0x6D; break;
                case 0x21: *typ = 0x54; break;
                case 0x22: *typ = 0x60; break;
                case 0x23: *typ = 0x56; break;
                case 0x24: *typ = 0x57; break;
                case 0x25: *typ = 0x58; break;
                case 0x2D: *typ = 0x18; break;
                case 0x30: *typ = 0x6A; break;
                case 0x31: *typ = 0x6B; break;
                case 0x32: *typ = 0x6C; break;
                case 0x40: *typ = 0x2D; break;
                case 0x41: *typ = 0x2A; break;
                case 0x42: *typ = 0x2B; break;
                case 0x43: *typ = 0x2C; break;
                case 0x44: *typ = 0x2F; break;
                case 0x45: *typ = 0x30; break;
                case 0x46: *typ = 0x50; break;
                case 0x51: *typ = 0x1E; break;
                case 0x52: *typ = 0x1F; break;
                case 0x53: *typ = 0x20; break;
                case 0x54: *typ = 0x21; break;
                case 0x55: *typ = 0x22; break;
                case 0x56: *typ = 0x23; break;
                case 0x57: *typ = 0x24; break;
                case 0x58: *typ = 0x25; break;
                case 0x59: *typ = 0x26; break;
                case 0x5A: *typ = 0x2E; break;
                case 0x65: *typ = 0x27; break;
                case 0x66: *typ = 0x28; break;
                case 0x67: *typ = 0x34; break;
                case 0x69: *typ = 0x35; break;
                case 0x6A: *typ = 0x36; break;
                case 0x6B: *typ = 0x37; break;
                case 0x6D: *typ = 0x78; break;
                case 0x7F: *typ = 0x3C; break;
                case 0x8D: *typ = 0x47; break;
                case 0x8E: *typ = 0x65; break;
                case 0x8F: *typ = 0x64; break;
                case 0x90: *typ = 0x66; break;
                case 0x91: *typ = 0x67; break;
                default:   *typ = 8;    break;
                }
            } else {
                *typ = 8;
            }
        } else if (cat == 3) {
            switch (*(u16 *)(def + 0x12)) {
            case 0x04: *typ = 0x5C; break;
            case 0x05: *typ = 0x5D; break;
            case 0x06: *typ = 0x44; break;
            case 0x07: *typ = 0x33; break;
            case 0x08: *typ = 0x06; break;
            case 0x0B: *typ = 0x15; break;
            case 0x0E: *typ = 0x12; break;
            case 0x0F: *typ = 0x3B; break;
            case 0x10: *typ = 0x40; break;
            case 0x18: *typ = 0x5B; break;
            case 0x19: *typ = 0x4C; break;
            case 0x1A: *typ = 0x43; break;
            case 0x1B: *typ = 0x32; break;
            case 0x1C: *typ = 0x05; break;
            case 0x1F: *typ = 0x14; break;
            case 0x22: *typ = 0x11; break;
            case 0x23: *typ = 0x3A; break;
            case 0x24: *typ = 0x3F; break;
            case 0x2B: *typ = 0x52; break;
            case 0x2C: *typ = 0x5A; break;
            case 0x2E: *typ = 0x42; break;
            case 0x2F: *typ = 0x31; break;
            case 0x30: *typ = 0x01; break;
            case 0x33: *typ = 0x13; break;
            case 0x36: *typ = 0x0C; break;
            case 0x37: *typ = 0x39; break;
            case 0x38: *typ = 0x3E; break;
            default:   *typ = 8;    break;
            }
        } else {
            *typ = 8;
        }
    }
}
