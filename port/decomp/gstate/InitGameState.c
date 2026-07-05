/* =============================================================================
 * InitGameState.c  --  PC-port level-state initialiser (TARGET_PC)
 * =============================================================================
 * PC replacement for asm/nonmatchings/gstate/InitGameState.s (173 lines),
 * transcribed from the m2c decompilation. This is the level-load entry: it
 * bootstraps the BLB header, loads the level, sets up the initial game-mode
 * dispatch (event_marker @ +0x0 = 0, event_callback @ +0x4 = &GameModeCallback),
 * copies level metadata into the GameState, builds the sub-level asset list, and
 * spawns the player + entities.
 *
 * Field writes use raw byte offsets (byte-accurate to the asm) to stay correct
 * regardless of GameState struct-header naming. arg0 = GameState*, arg1 = the
 * active input-state pointer (D_800A5964), stashed at +0x140.
 *
 * Still-asm callees below (LoadBLBHeader, InitializeAndLoadLevel,
 * SpawnPlayerAndEntities, RemapEntityTypesForLevel, ...) resolve to weak stubs
 * until converted -- the first one reached is the next Phase-2 task.
 * ========================================================================== */
#include "common.h"
#include "globals.h"

/* GameState pointer + player-state alias. */
extern GameState *g_pGameState;

/* Player state at D_800A597C: first two bytes are level/stage index, +0x4 world. */
extern u8 *RESPAWN_PLAYER_STATE asm("D_800A597C");
extern void *GameModeCallback;
/* Level base-data blob at D_8009D5F8 (declared via asm alias so the port stub
 * generator backs it with weak storage). */
extern u8 D_8009D5F8[] asm("D_8009D5F8");

/* Level-load callees (mix of matched C in blbacc.c and still-asm). */
extern void LoadBLBHeader(void *gs);
extern void InitializeAndLoadLevel(void *gs, int arg);
extern s8   GetCurrentLevelAssetIndex(void *ctx);
extern s8   GetCurrentStageIndex(void *ctx);
extern s8   GetLevelAssetIndex(void *ctx, u32 idx);
extern s32  GetLevelCount(void *ctx);
extern s32  GetLevelFlagByIndex(void *ctx, u32 idx);
extern s16  GetTileHeaderField16(void *ctx);
extern s8   GetTileHeaderWorldIndex(void *ctx);
extern s32  GetVehicleDataPtr(void *ctx);
extern void RemapEntityTypesForLevel(void *gs);
extern void ClearAlternateEntitySpawnFlags(void *gs);
extern void StartCDAudioForLevel(int assetIdx, int stageIdx);
extern void SpawnPlayerAndEntities(void *gs);

void InitGameState(void *arg0, s32 arg1) {
    u8 *gs = (u8 *)arg0;
    u8 *ctx = gs + 0x84;               /* embedded LevelDataContext */
    s8 assetIdx;
    s32 i;
    u32 sub;

    LoadBLBHeader(g_pGameState);
    InitializeAndLoadLevel(arg0, 0x63);

    assetIdx = GetCurrentLevelAssetIndex((u8 *)g_pGameState + 0x84);
    if (assetIdx & 0xFF) {
        RESPAWN_PLAYER_STATE[0] = (u8)assetIdx;
        RESPAWN_PLAYER_STATE[1] = (u8)GetCurrentStageIndex(ctx);
    } else {
        RESPAWN_PLAYER_STATE[0] = 1;
        RESPAWN_PLAYER_STATE[1] = 1;
    }

    /* Clear the 8-entry input/cheat ring at +0x17C (stride 2). */
    for (i = 0; i < 8; i++) {
        gs[i * 2 + 0x17C] = 0;
    }

    gs[0x18C] = 0; gs[0x18F] = 0; gs[0x190] = 0; gs[0x18E] = 0; gs[0x18D] = 0;
    gs[0x161] = 0; gs[0x198] = 0;
    gs[0x199] = 0x40; gs[0x19A] = 0x40; gs[0x19B] = 0x40;
    gs[0x16C] = 0;
    *(s32 *)(gs + 0x140) = arg1;
    gs[0x14B] = 0; gs[0x13C] = 0; gs[0x134] = 0;
    gs[0x144] = 0; gs[0x145] = 0; gs[0x146] = 0; gs[0x147] = 0;
    gs[0x148] = 0; gs[0x149] = 0; gs[0x14A] = 0;
    gs[0x150] = 0; gs[0x151] = 0; gs[0x152] = 0;
    gs[0x19C] = 0; gs[0x19D] = 0;

    /* Initial game-mode dispatch: marker 0, callback = GameModeCallback. */
    *(s16 *)(gs + 0x0) = 0;
    *(void **)(gs + 0x4) = &GameModeCallback;

    RemapEntityTypesForLevel(arg0);
    *(s32 *)(gs + 0x164) = GetVehicleDataPtr(ctx);
    *(s16 *)(gs + 0x168) = GetTileHeaderField16(ctx);
    ClearAlternateEntitySpawnFlags(arg0);
    *(void **)(gs + 0x7C) = D_8009D5F8;
    gs[0x80] = 0x79;

    StartCDAudioForLevel(GetCurrentLevelAssetIndex(ctx) & 0xFF,
                         GetCurrentStageIndex(ctx) & 0xFF);

    /* Build the sub-level asset list at +0x171 (capacity 0xA). */
    gs[0x17B] = 0;
    for (i = 0; (u32)(i & 0xFF) < (u32)(GetLevelCount(ctx) & 0xFF); i++) {
        sub = (u32)(i & 0xFF);
        if ((u8)gs[0x17B] < 0xA && (GetLevelFlagByIndex(ctx, sub) & 0xFF)) {
            s8 a = GetLevelAssetIndex(ctx, sub);
            if (a & 0xFF) {
                gs[gs[0x17B] + 0x171] = (u8)a;
                gs[0x17B] = (u8)(gs[0x17B] + 1);
            }
        }
    }

    RESPAWN_PLAYER_STATE[4] = (u8)GetTileHeaderWorldIndex(gs + 0x84);
    SpawnPlayerAndEntities(arg0);
}
