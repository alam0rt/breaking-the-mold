#include "common.h"
#include "functions.h"
extern s32 GetLevelFlags();
#include "Game/game_state.h"
#include "globals.h"

extern u8 D_80012120[];

/* Top-level level-load driver. Clears the prior level's entity lists, then
 * drives the BLB playback FSM (AdvancePlaybackSequence / SeekToLevelInSequence)
 * to stream CD sectors into the LevelDataContext (gs+0x84), dispatching each
 * mode through jtbl_80012140 until all assets are resident. */
INCLUDE_ASM("asm/nonmatchings/lvlload", InitializeAndLoadLevel);

/* Per-level setup that runs once the BLB load completes. Stops SPU/CD,
 * draws the transition curtain, reloads the debug font, restores checkpoint
 * state if active, resets BG color to 0x40 grey, clears level-clear /
 * advance / next-level flags, then calls SpawnPlayerAndEntities. */
INCLUDE_ASM("asm/nonmatchings/lvlload", SetupAndStartLevel);

/* Creates the full-screen transition curtain sprite shown while the next
 * level streams in. Allocates a HUD-sized (0x7530 byte) entity with sprite
 * hash 0x8AB92024 and 0x40 RGB tint, hooked to GetWorldPositionX/Y. */
INCLUDE_ASM("asm/nonmatchings/lvlload", DisplayTransitionSprite);

/* Player-creation dispatcher. Reads GetLevelFlags() and picks the player
 * variant: FINN (0x400), MENU (0x200), RESULTS/BOSS (0x2000), RUNN (0x100),
 * SOAR (0x10), GLIDE (0x4), else normal platformer. Also sets
 * camera_scroll_speed (gs+0x11C) and validates the spawn tile via
 * GetTileAttributeAtPosition. */
INCLUDE_ASM("asm/nonmatchings/lvlload", SpawnPlayerAndEntities);

extern u8 D_80012100[];
extern void DestroyEntity(void *entity, s32 arg);

/* Entity teardown helper. Replaces the entity's vtable (+0x18) with the
 * dead-entity stub D_80012100, frees the depth-bucket allocation at +0x16C
 * back to the BLB heap, calls DestroyEntity to detach it from all lists,
 * and optionally frees the entity body itself (flags bit 0). */
void DestroyEntityAndFreeResources(void *entity, s32 flags) {
    void *buckets = *(void **)((u8 *)entity + 0x16C);
    *(u32 *)((u8 *)entity + 0x18) = (u32)D_80012100;
    if (buckets) {
        FreeFromHeap(g_pBlbHeapBase, buckets, 0, 0);
    }
    DestroyEntity(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

/* Per-frame game-mode tick installed in GameState. Decrements the pause
 * countdown (+0x160), reads PAD_START (input flag 0x800) to open the pause
 * menu, runs the debug frame-step cheat, and routes to
 * UnpauseGameAndRestoreEntities, SetNextLevelFlagWithFade, level-clear,
 * level-advance, or checkpoint-restore based on flag state. */
INCLUDE_ASM("asm/nonmatchings/lvlload", GameModeCallback);

extern void AddToZOrderList(u8 *obj, s32 zOrder);

/* Checkpoint snapshot. Saves the live tick_list_head (+0x1C) and
 * frame_counter (+0x10C) into the checkpoint slots (+0x134 / +0x138),
 * sets checkpoint_active (+0x14A) and pause_freeze (+0x63), clears the
 * tick list, then re-adds the GameState entity via AddToZOrderList. Run
 * when the player touches a checkpoint pickup. */
void SaveCheckpointState(void *entity) {
    *(u32 *)((u8 *)entity + 0x138) = *(u32 *)((u8 *)entity + 0x10C);
    *(u32 *)((u8 *)entity + 0x134) = *(u32 *)((u8 *)entity + 0x1C);
    *(u8 *)((u8 *)entity + 0x14A) = 1;
    *(u8 *)((u8 *)entity + 0x63) = 1;
    *(u32 *)((u8 *)entity + 0x1C) = 0;
    AddToZOrderList(entity, *(void **)((u8 *)entity + 0x2C));
}

/* Inverse of SaveCheckpointState. Restores the saved tick list (+0x134 ->
 * +0x1C) and frame_counter (+0x138 -> +0x10C), then walks the saved-list
 * nodes, re-adding each entity via AddToZOrderList and freeing the wrapper
 * node from the BLB heap. Called on respawn. */
INCLUDE_ASM("asm/nonmatchings/lvlload", RestoreCheckpointEntities);

/* Walks the checkpoint-saved entity list (singly-linked 8-byte {next,id}
 * nodes at gs+0x134), unlinks the node whose id matches target, and frees
 * it. Returns 1 on hit, 0 if not found. Called when a respawnable pickup
 * is consumed so it won't reappear on the next checkpoint respawn. */
s32 RemoveCheckpointEntityById(s32 gameState, s32 target) {
    s32 *node = *(s32 **)(gameState + 0x134);
    s32 *prev = 0;

    while (node != 0) {
        if (*(s32 *)((s32)node + 4) == target) {
            if (prev == 0) {
                *(s32 *)(gameState + 0x134) = *node;
            } else {
                *prev = *node;
            }
            FreeFromHeap(g_pBlbHeapBase, (void *)node, 8, 0);
            return 1;
        }
        prev = node;
        node = *(s32 **)(s32)node;
    }
    return 0;
}

/* Opens the pause menu. Mutes SPU voices, plays the pause SFX, swaps the
 * live tick list into saved_tick_list (+0x15C) so the world freezes, starts
 * the 22-frame fade countdown, and calls ShowPauseMenuHUD. Type-0x10
 * entities (menu HUD children) are spliced back into the live tick list so
 * they keep updating while the world is paused. */
INCLUDE_ASM("asm/nonmatchings/lvlload", PauseGameAndShowMenu);

/* Begins the pause-to-fade-out transition: plays FX_BUTTON_UNPAUSE, sets
 * fade_out_active (+0x151), hides the pause menu HUD, and arms the 22-frame
 * fade countdown (+0x160 = 0x16). GameModeCallback completes the transition
 * when the countdown reaches 0. */
void PauseGameWithFadeOut(u8 *obj) {
    PlaySoundEffect(FX_BUTTON_UNPAUSE, 0xA0, 1);
    obj[0x151] = 1;
    HidePauseMenuHUD(*(s32 *)&obj[0x14C]);
    obj[0x160] = 0x16;
}

/* Triggers a fade-out and queues advance-to-next-level: sets fade_out_active
 * (+0x151), starts the 22-frame fade countdown (+0x160), and sets
 * next_level_flag (+0x147). Used by the pause menu's Quit/Continue option. */
void SetNextLevelFlagWithFade(u8 *obj) {
    obj[0x151] = 1;
    obj[0x160] = 0x16;
    obj[0x147] = 1;
}

extern void ResumeAllVoicePitches(void);
extern void ClearTickList(void *entity);

/* Unpause. Resumes SPU voices, restores saved frame_counter and
 * pause_freeze byte (+0x154/+0x158 -> +0x10C/+0x63), clears menu_active and
 * fade_out flags, swaps the saved tick list back via ClearTickList, and if
 * the cheat flag at +0x18D is set re-adds the player to the render list. */
void UnpauseGameAndRestoreEntities(void *entity) {
    ResumeAllVoicePitches();
    *(u32 *)((u8 *)entity + 0x10C) = *(u32 *)((u8 *)entity + 0x154);
    *(u8 *)((u8 *)entity + 0x63) = *(u8 *)((u8 *)entity + 0x158);
    *(u8 *)((u8 *)entity + 0x151) = 0;
    *(u8 *)((u8 *)entity + 0x150) = 0;
    ClearTickList(entity);
    *(u32 *)((u8 *)entity + 0x1C) = *(u32 *)((u8 *)entity + 0x15C);
    *(u32 *)((u8 *)entity + 0x15C) = 0;
    if (*(u8 *)((u8 *)entity + 0x18D)) {
        AddToZOrderList(entity, *(void **)((u8 *)entity + 0x2C));
        *(u8 *)((u8 *)entity + 0x18D) = 0;
    }
}


/* End-of-level fade-to-black. Sets level_clear_pending (+0x144), arms a BG
 * color update to RGB=0/0/0 via bg_color_change_flag (+0x130), pulls the
 * GameState entity out of the tick list, frees its entity lists, re-adds
 * it to the sorted render list, and stashes the entity-defs pointer
 * (+0x28 -> +0x13C) so SetupAndStartLevel can restore it on the next level. */
void ClearEntitiesAndFadeToBlack(void *entity) {
    void *field2c = *(void **)((u8 *)entity + 0x2C);
    *(u8 *)((u8 *)entity + 0x145) = 1;
    *(u8 *)((u8 *)entity + 0x130) = 1;
    *(u8 *)((u8 *)entity + 0x131) = 0;
    *(u8 *)((u8 *)entity + 0x132) = 0;
    *(u8 *)((u8 *)entity + 0x133) = 0;
    RemoveFromTickList(entity, field2c);
    FreeEntityLists(entity);
    AddEntityToSortedRenderList(entity, *(void **)((u8 *)entity + 0x2C));
    *(u32 *)((u8 *)entity + 0x13C) = *(u32 *)((u8 *)entity + 0x28);
    *(u32 *)((u8 *)entity + 0x28) = 0;
}

/* Trigger-zone subId -> RGB tint lookup. Reads the subId parameter,
 * subtracts 0x15, and indexes a 3-byte-stride table at D_8009D9C0
 * (20 entries, subIds 0x15..0x28), writing R/G/B to the three out-byte
 * pointers. Returns 1 on hit, 0 if out of range. */
INCLUDE_ASM("asm/nonmatchings/lvlload", HandleGenericTriggerZone);

/* Post-death cleanup that strips one-shot entities before respawn. Walks
 * the live tick list (+0x1C) removing type-0x08 entities whose +0x70 flag
 * is 0 and type-0x20 entities whose +0x70 == 1; then walks the entity spawn
 * list (+0x28) dropping types 0x39/0x3A, 0x3B, and 0x1C/0x1D so they don't
 * re-spawn after the player dies. */
INCLUDE_ASM("asm/nonmatchings/lvlload", CleanupRespawnEntities);
