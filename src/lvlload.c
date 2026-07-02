#include "common.h"
#include "functions.h"
extern s32 GetLevelFlags();
#include "Game/game_state.h"
#include "globals.h"
#include "Game/lvlload_records.h"

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

extern u8 DEAD_ENTITY_VTABLE[] asm("D_80012100");
extern void DestroyEntity(GameState *entity, s32 arg);

/* Entity teardown helper. Replaces the entity's vtable (+0x18) with the
 * dead-entity stub D_80012100, frees the depth-bucket allocation at +0x16C
 * back to the BLB heap, calls DestroyEntity to detach it from all lists,
 * and optionally frees the entity body itself (flags bit 0). */
void DestroyEntityAndFreeResources(GameState *gameState, s32 flags) {
    void *depth_buckets = gameState->entity_pool_ptr;
    gameState->postRenderCallbackContext = DEAD_ENTITY_VTABLE;
    if (depth_buckets) {
        FreeFromHeap(g_pBlbHeapBase, depth_buckets, 0, 0);
    }
    DestroyEntity(gameState, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, gameState, 0, 0);
    }
}

/* Per-frame game-mode tick installed in GameState. Decrements the pause
 * countdown (+0x160), reads PAD_START (input flag 0x800) to open the pause
 * menu, runs the debug frame-step cheat, and routes to
 * UnpauseGameAndRestoreEntities, SetNextLevelFlagWithFade, level-clear,
 * level-advance, or checkpoint-restore based on flag state. */
INCLUDE_ASM("asm/nonmatchings/lvlload", GameModeCallback);

extern void AddToZOrderList(GameState *list, Entity *entity);

/* Checkpoint snapshot. Saves the live tick_list_head (+0x1C) and
 * frame_counter (+0x10C) into the checkpoint slots (+0x134 / +0x138),
 * sets checkpoint_active (+0x14A) and pause_freeze (+0x63), clears the
 * tick list, then re-adds the GameState entity via AddToZOrderList. Run
 * when the player touches a checkpoint pickup. */
void SaveCheckpointState(GameState *gameState) {
    gameState->checkpoint_saved_frame_counter = gameState->frame_counter;
    gameState->checkpoint_entity_list = gameState->tick_list_head;
    gameState->checkpoint_active = 1;
    gameState->pause_freeze_flag = 1;
    gameState->tick_list_head = NULL;
    AddToZOrderList(gameState, gameState->player_entity_alt);
}

/* Inverse of SaveCheckpointState. Restores the saved tick list (+0x134 ->
 * +0x1C) and frame_counter (+0x138 -> +0x10C), then walks the live tick-list
 * nodes (captured before the swap), re-adding each entity via AddToZOrderList
 * and freeing the wrapper node from the BLB heap. Called on respawn.
 *
 * The explicit `goto` loop reproduces cc1's unrotated top-test + back-jump
 * loop form (a plain while/for rotates to a bottom-test); the `do {} while(0)`
 * around the body is a basic-block barrier that pins node->s0 / gameState->s1. */
void RestoreCheckpointEntities(GameState *gameState) {
    EntityListNode *node;
    EntityListNode *cur;

    gameState->checkpoint_active = 0;
    gameState->pause_freeze_flag = 0;
    gameState->frame_counter = gameState->checkpoint_saved_frame_counter;
    RemoveFromTickList((Entity *)gameState, gameState->player_entity_alt);
    node = gameState->tick_list_head;
    gameState->tick_list_head = gameState->checkpoint_entity_list;
    gameState->checkpoint_entity_list = NULL;
loop:
    if (node != NULL) {
        do {
            cur = node;
            AddToZOrderList(gameState, node->entity);
            node = node->next;
            FreeFromHeap(g_pBlbHeapBase, cur, 8, 0);
        } while (0);
        goto loop;
    }
}

/* Walks the checkpoint-saved entity list (singly-linked 8-byte {next,id}
 * nodes at gs+0x134), unlinks the node whose id matches target, and frees
 * it. Returns 1 on hit, 0 if not found. Called when a respawnable pickup
 * is consumed so it won't reappear on the next checkpoint respawn. */
s32 RemoveCheckpointEntityById(GameState *gameState, s32 target) {
    CheckpointEntityNode *node = (CheckpointEntityNode *)gameState->checkpoint_entity_list;
    CheckpointEntityNode *prev = NULL;

    while (node != NULL) {
        if (node->entity_id == target) {
            if (prev == NULL) {
                gameState->checkpoint_entity_list = (EntityListNode *)node->next;
            } else {
                prev->next = node->next;
            }
            FreeFromHeap(g_pBlbHeapBase, node, 8, 0);
            return 1;
        }
        prev = node;
        node = node->next;
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
void PauseGameWithFadeOut(GameState *gameState) {
    PlaySoundEffect(FX_BUTTON_UNPAUSE, 0xA0, 1);
    gameState->fade_out_active = 1;
    HidePauseMenuHUD((s32)gameState->hud_entity_ptr);
    gameState->pause_countdown = 0x16;
}

/* Triggers a fade-out and queues advance-to-next-level: sets fade_out_active
 * (+0x151), starts the 22-frame fade countdown (+0x160), and sets
 * next_level_flag (+0x147). Used by the pause menu's Quit/Continue option. */
void SetNextLevelFlagWithFade(GameState *gameState) {
    gameState->fade_out_active = 1;
    gameState->pause_countdown = 0x16;
    gameState->next_level_flag = 1;
}

extern void ResumeAllVoicePitches(void);
extern void ClearTickList(GameState *gameState);

/* Unpause. Resumes SPU voices, restores saved frame_counter and
 * pause_freeze byte (+0x154/+0x158 -> +0x10C/+0x63), clears menu_active and
 * fade_out flags, swaps the saved tick list back via ClearTickList, and if
 * the cheat flag at +0x18D is set re-adds the player to the render list. */
void UnpauseGameAndRestoreEntities(GameState *gameState) {
    ResumeAllVoicePitches();
    gameState->frame_counter = gameState->saved_frame_counter;
    gameState->pause_freeze_flag = gameState->saved_freeze_flag;
    gameState->fade_out_active = 0;
    gameState->menu_active = 0;
    ClearTickList(gameState);
    gameState->tick_list_head = gameState->saved_tick_list;
    gameState->saved_tick_list = NULL;
    if (gameState->player_readd_flag) {
        AddToZOrderList(gameState, gameState->player_entity_alt);
        gameState->player_readd_flag = 0;
    }
}


/* End-of-level fade-to-black. Sets level_clear_pending (+0x144), arms a BG
 * color update to RGB=0/0/0 via bg_color_change_flag (+0x130), pulls the
 * GameState entity out of the tick list, frees its entity lists, re-adds
 * it to the sorted render list, and stashes the entity-defs pointer
 * (+0x28 -> +0x13C) so SetupAndStartLevel can restore it on the next level. */
void ClearEntitiesAndFadeToBlack(GameState *gameState) {
    void *player_entity = gameState->player_entity_alt;
    gameState->level_transition_complete = 1;
    gameState->bg_color_change_flag = 1;
    gameState->bg_color_r_pending = 0;
    gameState->bg_color_g_pending = 0;
    gameState->bg_color_b_pending = 0;
    RemoveFromTickList(gameState, player_entity);
    FreeEntityLists(gameState);
    AddEntityToSortedRenderList(gameState, gameState->player_entity_alt);
    gameState->entity_defs_backup = gameState->entity_spawn_list;
    gameState->entity_spawn_list = NULL;
}

/* Trigger-zone subId -> RGB tint lookup. Reads the subId parameter,
 * subtracts 0x15, and indexes a 3-byte-stride table at D_8009D9C0
 * (20 entries, subIds 0x15..0x28), writing R/G/B to the three out-byte
 * pointers. Returns 1 on hit, 0 if out of range. The leading unused
 * argument is genuinely unused -- the function immediately reloads
 * a0 from sp+0x10 (the 5th-arg slot) to use as the B output ptr.
 *
 * Source quirk: the table is named via three distinct gp_rel symbols
 * D_8009D9C0 / D_8009D9C1 / D_8009D9C2 -- one per color channel -- each
 * indexed by `idx * 3`. */
extern u8 TRIGGER_ZONE_R_TABLE[] asm("D_8009D9C0");
extern u8 TRIGGER_ZONE_G_TABLE[] asm("D_8009D9C1");
extern u8 TRIGGER_ZONE_B_TABLE[] asm("D_8009D9C2");

s32 HandleGenericTriggerZone(s32 unused, u8 zoneId, u8 *outR, u8 *outG,
                              u8 *outB) {
    if ((u8)(zoneId - 0x15) < 0x14) {
        u8 idx = zoneId - 0x15;
        *outR = TRIGGER_ZONE_R_TABLE[idx * 3];
        *outG = TRIGGER_ZONE_G_TABLE[idx * 3];
        *outB = TRIGGER_ZONE_B_TABLE[idx * 3];
        return 1;
    }
    return 0;
}

/* Post-death cleanup that strips one-shot entities before respawn. Walks
 * the live tick list (+0x1C) removing type-0x08 entities whose +0x70 flag
 * is 0 and type-0x20 entities whose +0x70 == 1; then walks the entity spawn
 * list (+0x28) dropping types 0x39/0x3A, 0x3B, and 0x1C/0x1D so they don't
 * re-spawn after the player dies. */
INCLUDE_ASM("asm/nonmatchings/lvlload", CleanupRespawnEntities);
