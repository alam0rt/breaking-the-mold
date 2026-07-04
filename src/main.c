#include "common.h"
#include "functions.h"
extern s32 GetLevelFlags();
#include "Game/game_state.h"
#include "globals.h"

extern u8 SPECIAL_ENTITY_FALLBACK_VTABLE[] asm("D_80012120");
void FreeSpecialEntity2120Memory(Entity *ptr, s32 size);


/* Per-frame cheat-code detector: appends the latest pad-edge word to GameState's
 * 8-entry circular buffer (+0x17C, idx @ +0x18C) and scans g_CheatCodeTable for
 * any matching sequence, dispatching cheat side-effects (sfx + jump table). */
INCLUDE_ASM("asm/nonmatchings/main", CheckCheatCodeInput);

/* GameState accessor: read background-color RGB triple from +0x199..0x19B
 * (bg_color_r/g/b). Wrapper used by code holding GameState as an opaque u8*. */
void GetGameStateBgColor(GameState *obj, u8 *outA, u8 *outB, u8 *outC) {
    *outA = obj->bg_color_r;
    *outB = obj->bg_color_g;
    *outC = obj->bg_color_b;
}

/* GameState accessor: write background-color RGB triple to +0x199..0x19B
 * (bg_color_r/g/b). Sibling setter to GetGameStateBgColor. */
void SetGameStateBgColor(GameState *obj, u8 a, u8 b, u8 c) {
    obj->bg_color_r = a;
    obj->bg_color_g = b;
    obj->bg_color_b = c;
}

/* GameState accessor: read byte at +0x198 (_reserved_198, normally 0). */
u8 GetGameStateReserved198(GameState *obj) {
    return obj->_reserved_198;
}

/* GameState accessor: write byte at +0x198 (_reserved_198, normally 0). */
void SetGameStateReserved198(GameState *obj, u8 val) {
    obj->_reserved_198 = val;
}

extern u8 SAVE_SLOT_NAME_TABLE[] asm("D_800A608C");
extern s32 FindSaveSlotByName(u8 *name, u8 *slots);

/* Look up the password/save slot index for the level currently embedded in
 * GameState. Forwards the LevelDataContext at +0x84 plus the global save-slot
 * name table (D_800A608C) to the generic lookup. */
s32 FindSaveSlotForCurrentLevel(GameState *obj) {
    return FindSaveSlotByName((u8 *)&obj->level_context, SAVE_SLOT_NAME_TABLE);
}

/* GameState accessor: write boss defeat record (+0x19C boss_defeated,
 * +0x19D boss_facing). Used by the boss defeat callback @ 0x8004906c. */
void SetGameStateBossDefeatRecord(GameState *obj, u8 a, u8 b) {
    obj->boss_defeated = a;
    obj->boss_facing = b;
}

/* GameState accessor: read boss_defeated flag at +0x19C. */
u8 GetGameStateBossDefeated(GameState *obj) {
    return obj->boss_defeated;
}

/* Extract the level's "debug enable" bit (bit 15) from the embedded
 * LevelDataContext flags. Returns 1 when debug overlays/menus are allowed. */
u8 GetLevelDebugFlag(GameState *gameState) {
    return (u16)GetLevelFlags(&gameState->level_context) >> 0xf;
}

/* Extract the level's "show HUD" bit (bit 14) from the embedded
 * LevelDataContext flags. */
u8 GetLevelShowHUDFlag(GameState *gameState) {
    return ((u16)GetLevelFlags(&gameState->level_context) >> 0xe) & 1;
}

/* GameState accessor: read checkpoint_active flag at +0x14A. */
u8 GetGameStateCheckpointActive(GameState *obj) {
    return obj->checkpoint_active;
}

/* GameState accessor: write demo_return_flag at +0x152 (returns to menu
 * when the attract-mode demo finishes). */
void SetGameStateDemoReturnFlag(GameState *obj, u8 val) {
    obj->demo_return_flag = val;
}

/* GameState accessor: write level_active flag at +0x170 (whether the current
 * level slot has a valid asset index). */
void SetGameStateLevelActive(GameState *obj, u8 val) {
    obj->level_active = val;
}

/* GameState accessor: read spawn_freeze_flag at +0x161 (suppresses entity
 * spawning while a checkpoint restore is in progress). */
u8 GetGameStateSpawnFreezeFlag(GameState *obj) {
    return obj->spawn_freeze_flag;
}

/* GameState accessor: write spawn_freeze_flag at +0x161. */
void SetGameStateSpawnFreezeFlag(GameState *obj, u8 val) {
    obj->spawn_freeze_flag = val;
}

/* GameState accessor: install alternate-entity-system spawn list -- stores the
 * 64-byte-stride entity array pointer at +0x164 (alternate_entity_data) and the
 * entry count at +0x168 (alternate_entity_count). Used by vehicle/boss modes. */
void SetGameStateAlternateEntityList(GameState *obj, s32 val32, s16 val16) {
    obj->alternate_entity_data = (void *)val32;
    obj->alternate_entity_count = val16;
}

/* GameState accessor: read hud_entity_ptr at +0x14C (entity created by
 * CreateMenuEntities for HUD rendering). Return type is s32 in the
 * decomp but semantically this is a pointer. */
s32 GetGameStateHudEntityPtr(GameState *obj) {
    return (s32)obj->hud_entity_ptr;
}

/* GameState accessor: read checkpoint_powerup_state at +0x14B (powerup byte
 * snapshotted at checkpoint, restored to PlayerState+0x18 on respawn). */
u8 GetGameStateCheckpointPowerupState(GameState *obj) {
    return obj->checkpoint_powerup_state;
}

/* GameState accessor: write checkpoint_powerup_state at +0x14B. */
void SetGameStateCheckpointPowerupState(GameState *obj, u8 val) {
    obj->checkpoint_powerup_state = val;
}

/* GameState accessor: write level_clear_pending flag at +0x144 (triggers
 * ClearEntitiesAndFadeToBlack at end-of-frame). */
void SetGameStateLevelClearPending(GameState *obj, u8 val) {
    obj->level_clear_pending = val;
}

/* GameState accessor: read checkpoint_restore_pending flag at +0x149. */
u8 GetGameStateCheckpointRestorePending(GameState *obj) {
    return obj->checkpoint_restore_pending;
}

/* GameState accessor: write checkpoint_restore_pending flag at +0x149
 * (cleared after the respawn has been performed). */
void SetGameStateCheckpointRestorePending(GameState *obj, u8 val) {
    obj->checkpoint_restore_pending = val;
}

/* GameState accessor: write direct_level_load flag at +0x148 (bypasses normal
 * level sequencing, used by debug menu and cheats). */
void SetGameStateDirectLevelLoad(GameState *obj, u8 val) {
    obj->direct_level_load = val;
}

/* GameState accessor: write advance_level_flag at +0x146 (causes the next
 * frame to advance the level sequence via AdvanceLevelSequence). */
void SetGameStateAdvanceLevelFlag(GameState *obj, u8 val) {
    obj->advance_level_flag = val;
}

/* Extract the level's "auto-scroll camera" bit (bit 11) from the embedded
 * LevelDataContext flags (set on rail/scroller levels). */
u8 GetLevelAutoScrollFlag(GameState *gameState) {
    return ((u16)GetLevelFlags(&gameState->level_context) >> 0xb) & 1;
}

/* Empty stub. Likely a debug or build-variant hook compiled to a no-op in
 * the retail build (e.g. dev-only profiling/logging callback). */
void MainNoopCallback_8008283C(void) {
}

/* Empty stub. Sibling of MainNoopCallback_8008283C; another dev/profiling slot left
 * as a no-op for retail. */
void MainNoopCallback_80082844(void) {
}

/* Destroy/cleanup callback for a "special" entity kind whose post-render
 * callback context lives at the static block D_80012120. Restores entity+0x18
 * to that fallback context, and (when flags&1) releases the entity body via
 * the BLB heap free wrapper. Installed via the standard entity destroy hook. */
void SpecialEntityDestroyCallback_2120(Entity *entity, s32 flags) {
    entity->collisionVtable = SPECIAL_ENTITY_FALLBACK_VTABLE;
    if (flags & 1) {
        FreeSpecialEntity2120Memory(entity, 0x1C);
    }
}

/* Thin wrapper around FreeFromHeap that releases a block back to the BLB heap
 * (g_pBlbHeapBase). Name is historical -- the function itself is generic and
 * the called signature accepts a second size arg (0x1C above) that this
 * decompiled prototype ignores; likely should be FreeFromBlbHeap. */
void FreeSpecialEntity2120Memory(Entity *ptr, s32 size) {
    FreeFromHeap(g_pBlbHeapBase, ptr, 0, 0);
}

/* PSX boot + main frame loop entry point (@ 0x800828B0).
 * Boot sequence: main_8008E6E0 (early init) -> SsUtReverbOn -> ResetCallback ->
 * LoadGameAssetLocations -> InitGraphicsSystem -> PadInit -> InitGeom ->
 * SetDispMask(1) -> FntLoad/FntOpen/SetDumpFnt (debug overlay) ->
 * initPlayerState -> InitGameState -> level/asset/movie name table build.
 * Frame loop: TickCDStreamBuffer -> PadRead+UpdateInputState (pads 1 & 2) ->
 * GameMode FSM dispatch (event_marker/event_callback @ +0x00..+0x04) ->
 * EntityTickLoop -> WaitForVBlankIfNeeded -> RenderEntities -> DrawSync(0) ->
 * postRenderCallbackContext dispatch ((**(ctx+0x1C))(ctx,0)) -> DrawSync(0) ->
 * optional VSync stall when g_GameFlags & 6 -> ProcessDebugMenuInput ->
 * FlushDebugFontAndEndFrame (which performs the OT flush + buffer swap).
 *
 * SHELVED C draft (see docs/analysis/decomp-drafts/main.c). Annotator-assisted
 * decompilation got within ~7 instructions (frame size 0x30 vs 0x40, missing
 * s2/s3 register usage), but the mode-dispatch register allocation (s4=argOff,
 * s5=cb) is hard to coerce from cc1 without specific register pinning. */
/* Spans 0x800828B0..0x80082C10 (0x360). main() is an infinite loop, so its
 * (unreachable) register-restore epilogue at 0x80082BE8 was carved off by
 * splat as a bogus func_80082BE8 — re-absorbed by extending main's size. */
INCLUDE_ASM("asm/nonmatchings/main", main);

/* Debug menu / level-select overlay -- runs once per frame at end-of-tick when
 * the global debug flag bit (g_GameFlags & 0x80) is set. Renders the level-name
 * list (g_DebugMenuItemNames[i] -> "sub 01".."sub 10") via FntPrint with a
 * selector arrow ("> " / "  "), handles up/down nav via the pad flags read
 * out of g_pCurrentInputState, and on Start (button bit 0x40) commits the
 * selection by stuffing the chosen level index into GameState.direct_level_load
 * (+0x148) via SetSequenceIndexByMode.
 *
 * See docs/analysis/prototype-debug-prints.md §5 for the global/struct naming
 * basis (this is the only intact debug HUD in retail). */
INCLUDE_ASM("asm/nonmatchings/main", ProcessDebugMenuInput);

/* .data island 0x8009DAE0..0x8009DC40 (352B, cheat code table) migrated from asm. */
/* 22 cheat codes, each an 8-entry sequence of PSX pad button masks, matched
 * against the 8-button input buffer at GameState+0x17C. */
u16 CHEAT_CODE_TABLE[22][8] asm("D_8009DAE0") = {
    {0x4000, 0x0020, 0x0002, 0x0008, 0x0080, 0x0001, 0x0010, 0x8000},
    {0x0004, 0x0010, 0x4000, 0x0008, 0x0020, 0x2000, 0x1000, 0x0100},
    {0x0002, 0x0020, 0x0020, 0x4000, 0x0008, 0x0020, 0x2000, 0x1000},
    {0x4000, 0x0020, 0x1000, 0x0002, 0x8000, 0x0010, 0x0100, 0x0100},
    {0x0002, 0x0020, 0x0020, 0x4000, 0x8000, 0x0020, 0x2000, 0x4000},
    {0x0004, 0x0010, 0x4000, 0x8000, 0x0020, 0x0100, 0x0080, 0x2000},
    {0x8000, 0x0010, 0x2000, 0x4000, 0x0010, 0x0100, 0x0100, 0x0100},
    {0x0080, 0x0010, 0x0002, 0x8000, 0x0100, 0x0020, 0x0010, 0x2000},
    {0x0008, 0x8000, 0x0080, 0x0010, 0x0004, 0x0010, 0x0002, 0x0100},
    {0x0008, 0x8000, 0x1000, 0x0004, 0x0004, 0x0080, 0x2000, 0x0100},
    {0x0008, 0x2000, 0x0020, 0x0002, 0x0002, 0x0080, 0x2000, 0x0100},
    {0x0100, 0x0020, 0x1000, 0x8000, 0x4000, 0x1000, 0x4000, 0x0080},
    {0x0010, 0x0004, 0x0004, 0x0080, 0x2000, 0x0020, 0x0010, 0x4000},
    {0x1000, 0x0008, 0x0020, 0x2000, 0x2000, 0x0080, 0x0010, 0x8000},
    {0x0100, 0x8000, 0x0020, 0x0004, 0x0004, 0x0020, 0x0100, 0x0100},
    {0x0001, 0x8000, 0x0020, 0x0002, 0x4000, 0x0080, 0x0010, 0x4000},
    {0x0001, 0x0020, 0x0020, 0x8000, 0x0100, 0x0001, 0x1000, 0x4000},
    {0x4000, 0x2000, 0x0010, 0x0001, 0x1000, 0x8000, 0x0010, 0x0100},
    {0x0004, 0x0010, 0x8000, 0x4000, 0x0002, 0x0010, 0x8000, 0x0100},
    {0x8000, 0x0080, 0x0002, 0x0020, 0x0008, 0x4000, 0x0020, 0x0002},
    {0x0008, 0x8000, 0x0080, 0x0010, 0x0008, 0x8000, 0x0080, 0x0010},
    {0x4000, 0x0080, 0x0010, 0x4000, 0x4000, 0x0080, 0x0080, 0x2000},
};
