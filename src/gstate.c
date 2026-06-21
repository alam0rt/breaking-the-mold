#include "common.h"
#include "Game/entity.h"
#include "Game/player_state.h"
#include "Game/game_state.h"

extern Entity *InitEntityWithTable(Entity *e);
extern void *DEAD_ENTITY_VTABLE asm("D_80012100");

extern u8 *g_pBlbHeapBase;
extern u8 *BLB_HEAP_CONTEXT asm("D_800A595C");
extern PlayerState *RESPAWN_PLAYER_STATE asm("D_800A597C");

extern void StopAllSPUVoices(void);
extern void ClearOrderingTables(u8 *heap);
extern void DrawSync(int mode);
extern void FlushDebugFontAndEndFrame(u8 *heap);
extern void WaitForVBlankIfNeeded(u8 *heap);
extern int FntLoad(int tx, int ty);
extern int FntOpen(int x, int y, int w, int h, int isbg, int n);
extern void SetDumpFnt(int id);
extern void RestoreCheckpointEntities(GameState *gs);
extern void FreeAllLayerRenderSlots(u8 *ctx);
extern void CleanupDeadEntities(GameState *gs);
extern void ClearAlternateEntitySpawnFlags(GameState *gs);
extern void DecrementPlayerLives(PlayerState *ps);
extern void LoadBGColorFromTileHeader(GameState *gs);
extern void AddPreInitEntitiesToList(GameState *gs);
extern void InitAnimatedTileEntities(GameState *gs);
extern void InitLayersAndTileState(GameState *gs);
extern void SpawnPlayerAndEntities(GameState *gs);

Entity *InitEntityWithTableAndSprite(Entity *e) {
    InitEntityWithTable(e);
    e->collisionVtable = &DEAD_ENTITY_VTABLE;
    return e;
}

INCLUDE_ASM("asm/nonmatchings/gstate", InitGameState);

void RespawnAfterDeath(GameState *gs) {
    u8 *heap;
    StopAllSPUVoices();
    ClearOrderingTables(g_pBlbHeapBase);
    DrawSync(0);
    heap = g_pBlbHeapBase;
    heap[0x1D] = 0;
    heap[0x1E] = 0;
    heap[0x1F] = 0;
    heap[0x505D] = 0;
    heap[0x505E] = 0;
    heap[0x505F] = 0;
    FlushDebugFontAndEndFrame(g_pBlbHeapBase);
    WaitForVBlankIfNeeded(g_pBlbHeapBase);
    gs->bg_color_change_flag = 1;
    FntLoad(0x3C0, 0x100);
    SetDumpFnt(FntOpen(0x10, 0x20, 0x120, 0xC8, 0, 0x200));
    if (gs->checkpoint_active) {
        RestoreCheckpointEntities(gs);
        gs->checkpoint_restore_pending = 0;
    }
    if (gs->entity_defs_backup) {
        gs->entity_spawn_list = gs->entity_defs_backup;
        gs->entity_defs_backup = 0;
    }
    RESPAWN_PLAYER_STATE->shrink_mode = gs->checkpoint_powerup_state;
    gs->advance_level_flag = 0;
    gs->next_level_flag = 0;
    gs->level_clear_pending = 0;
    gs->level_transition_complete = 0;
    gs->direct_level_load = 0;
    gs->glide_boss_state_x = 0;
    gs->glide_boss_state_y = 0;
    gs->bounce_active_flag = 0;
    gs->frame_counter = 0;
    FreeEntityLists((Entity *)gs);
    FreeAllLayerRenderSlots(BLB_HEAP_CONTEXT);
    CleanupDeadEntities(gs);
    ClearAlternateEntitySpawnFlags(gs);
    DecrementPlayerLives(RESPAWN_PLAYER_STATE);
    LoadBGColorFromTileHeader(gs);
    AddPreInitEntitiesToList(gs);
    InitAnimatedTileEntities(gs);
    InitLayersAndTileState(gs);
    SpawnPlayerAndEntities(gs);
    FlushDebugFontAndEndFrame(g_pBlbHeapBase);
}

extern void AdvanceLevelSequence(u8 *p);
extern void SeekToLevelInSequence(u8 *p, s32 a, s32 b, s32 c);
extern u8 SetupAndStartLevel(u8 *p, s32 mode);
extern void initPlayerState(PlayerState *p);
extern PlayerState *PLAYER_STATE_DATA asm("D_800A597C");

void StartLevelWithResetOrAdvance(u8 *p, u32 mode) {
    if (mode & 0xFF) {
        AdvanceLevelSequence(p + 0x84);
    } else {
        SeekToLevelInSequence(p + 0x84, 0, 1, 0);
    }
    if (SetupAndStartLevel(p, 0x63) == 0) {
        initPlayerState(PLAYER_STATE_DATA);
    }
}

