/* =============================================================================
 * LoadBLBHeader.c  --  PC-port BLB header bootstrap (TARGET_PC)
 * =============================================================================
 * PC replacement for asm/nonmatchings/blb/LoadBLBHeader.s (0x800208B0),
 * transcribed from the disassembly. Bootstraps the level: inits SPU defaults,
 * clears the GameState scratch fields, writes the 0x01234567 sentinels, reads
 * the first two GAME.BLB header sectors (0x1000 bytes) into the scratch buffer
 * (set up at heap+0xA650 by InitGraphicsSystem), then hands off to
 * InitLevelDataContext (with BLB_ReadSectorsWrapper as the streaming-read
 * callback) and SetSpriteTables.
 *
 * CdBLB_ReadSectors + BLB_ReadSectorsWrapper are matched C (gamecd.c / blb.c)
 * and route through the direct-GAME.BLB CD backend. Still-asm callees
 * (InitSPUDefaults, InitLevelDataContext, SetSpriteTables) resolve to weak stubs
 * until converted. See docs/plans/pc-port.md CP-2.2.
 * ========================================================================== */
#include <stdlib.h>
#include "common.h"
#include "globals.h"

extern void InitSPUDefaults(void);
extern s32  CdBLB_ReadSectors(u16 sector_offset, u16 num_sectors, u8 *buffer);
extern void InitLevelDataContext(void *ctx, u8 *buffer, void *readCb);
extern void SetSpriteTables(int a0, void *ctx);
/* Fixed-address sentinel word at 0x801FC400 (declared via asm alias so the port
 * stub generator backs it with weak storage). */
extern u32  D_801FC400 asm("D_801FC400");

/* PC BLB sector-read callback: the 3-arg reader the asset loader expects
 * (sectorOffset, sectorCount, dst). The decomp's BLB_ReadSectorsWrapper is a
 * MIPS-only 2-arg trampoline that relied on the caller pre-latching $a2=dst;
 * on PC cdecl that passthrough breaks, so LoadBLBHeader installs THIS proper
 * 3-arg wrapper instead (LevelDataContext.sector_read_callback @ +0x64). */
static s32 port_blb_read_sectors(u32 sector, u32 count, u8 *dst) {
    return CdBLB_ReadSectors((u16)sector, (u16)count, dst);
}

/* Dump-parity: this static stands in for BLB_ReadSectorsWrapper (0x80020848),
 * whose address the PS1 keeps in LevelDataContext.sector_read_callback (the
 * GameState +0xE8 word). Register it so trace pointer translation maps the
 * stored host address back to the PSX one (port_trace.c). */
extern void port_trace_map_add(void *host, unsigned size, unsigned psx_addr);
#define PSX_BLB_READ_SECTORS_WRAPPER 0x80020848u

void LoadBLBHeader(void *arg0) {
    u8 *gs = (u8 *)arg0;
    u8 *buffer;

    InitSPUDefaults();

    g_pGameState = (GameState *)gs;
    gs[0x114] = 0;
    gs[0x130] = 0;
    *(s32 *)(gs + 0x11C) = 0x10000;
    *(s32 *)(gs + 0x28) = 0;
    *(s16 *)(gs + 0x104) = 0;
    *(s32 *)(gs + 0x108) = 0;
    *(s32 *)(gs + 0x110) = 0;
    *(s16 *)(gs + 0x64) = 0;
    *(s16 *)(gs + 0x66) = 0;
    *(s32 *)(gs + 0x12C) = 0x1234567;

    /* BLB header read buffer, stashed at heap+0xA650 by InitGraphicsSystem. */
    buffer = *(u8 **)((u8 *)g_pBlbHeapBase + 0xA650);
    D_801FC400 = 0x1234567;
    *(u8 **)(gs + 0x40) = buffer;
    *(u8 **)(gs + 0x3C) = buffer + 0x1000;

    CdBLB_ReadSectors(0, 2, buffer);

    /* PORT_BOOT_LEVEL / PORT_BOOT_STAGE: boot straight into a level (same
     * mechanism as scripts/game_watcher.lua's boot_level_override, which pokes
     * 0x800AF316/0xF372/0xF373 = header+0xF36 game_mode, +0xF92 level, +0xF93
     * stage). Setting mode 3 makes the initial InitializeAndLoadLevel load the
     * level instead of the menu. */
    {
        const char *bl = getenv("PORT_BOOT_LEVEL");
        if (bl != NULL) {
            const char *bs = getenv("PORT_BOOT_STAGE");
            buffer[0xF36] = 3;                          /* game mode: level */
            buffer[0xF92] = (u8)atoi(bl);               /* level index */
            buffer[0xF93] = (u8)(bs ? atoi(bs) : 0);    /* stage index */
        }
    }

    port_trace_map_add((void *)port_blb_read_sectors, 0x24,
                       PSX_BLB_READ_SECTORS_WRAPPER);
    InitLevelDataContext(gs + 0x84, *(u8 **)(gs + 0x40), (void *)port_blb_read_sectors);
    SetSpriteTables(0, gs + 0x84);
}
