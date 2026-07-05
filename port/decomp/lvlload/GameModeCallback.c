/* =============================================================================
 * GameModeCallback.c  --  PC-port per-frame game-mode dispatcher
 * =============================================================================
 * Faithful transcription of export/SLES_010.90.c GameModeCallback (0x8007E654,
 * INCLUDE_ASM in src/lvlload.c). This is THE per-frame mode callback (installed
 * at GameState+0x04 by InitializeAndLoadLevel); the frame loop invokes it via
 * the GameState FSM header. It handles pause/menu input, level-transition
 * requests (advance / next / respawn / boss-clear / direct-load), checkpoint
 * save/restore, demo-return, entity spawning and the entity tick pass.
 *
 * On the title screen menu_active != 0, so the level-transition block is skipped
 * (jump to after_level_logic) and only the menu-input handling + tick pass run.
 *
 * The export's `(undefined1[20])*input_state_ptr & 0x50000000` test reads the
 * word at InputState+0 (buttons_held | buttons_pressed<<16) and masks the high
 * half, i.e. `buttons_pressed & 0x5000` (the up/down menu navigation bits). It
 * is transcribed as such.
 *
 * Control flow uses function-scope gotos matching the export's LAB_* targets;
 * all locals are declared up front so no goto jumps over an initializer.
 * ---------------------------------------------------------------------------*/
#include "common.h"
#include "globals.h"
#include "Game/game_state.h"
#include <stdint.h>

extern void UnpauseGameAndRestoreEntities(GameState *gs);
extern u32  ToggleMenuOptionState(void *entity);
extern void PauseGameWithFadeOut(GameState *gs);
extern void SetNextLevelFlagWithFade(int gs);
extern void ToggleMenuSelectionHighlight(int entity);
extern u32  CheckCheatCodeInput(GameState *gs);
extern void PauseGameAndShowMenu(GameState *gs);
extern void SeekToLevelInSequence(void *ctx, s32 a, s32 b, s32 c);
extern u8   SetupAndStartLevel(GameState *gs, s32 mode);
extern void AdvanceLevelSequence(void *ctx);
extern void RespawnAfterDeath(GameState *gs);
extern void RestoreCheckpointEntities(GameState *gs);
extern void SaveCheckpointState(GameState *gs);
extern void ClearEntitiesAndFadeToBlack(GameState *gs);
extern void SpawnEntitiesAlternateSystem(GameState *gs);
extern void SpawnOnScreenEntities(GameState *gs);
extern void EntityTickLoopWithCamera(GameState *gs);
extern void initPlayerState(void *state);

void GameModeCallback(GameState *gameState) {
    u8  pauseCd;
    u16 pressed;
    u32 result;

    pauseCd = gameState->pause_countdown;
    if (pauseCd != 0) {
        gameState->pause_countdown = pauseCd - 1;
        if ((pauseCd == 1) && (gameState->fade_out_active != 0)) {
            UnpauseGameAndRestoreEntities(gameState);
        }
        goto after_menu;
    }

    pressed = gameState->input_state_ptr->buttons_pressed;
    if ((pressed & 0x800) == 0) {
        if (gameState->menu_active == 0) {
            goto after_menu;
        }
        if ((gameState->input_state_ptr->buttons_pressed & 0x5000) == 0) {
            if ((pressed & 0x40) != 0) {
                result = ToggleMenuOptionState(gameState->hud_entity_ptr);
                if ((result & 0xff) == 1) {
                    PauseGameWithFadeOut(gameState);
                } else if ((result & 0xff) == 2) {
                    SetNextLevelFlagWithFade((int)(intptr_t)gameState);
                }
                goto after_toggle;
            }
        } else {
            ToggleMenuSelectionHighlight((int)(intptr_t)gameState->hud_entity_ptr);
after_toggle:
            if ((gameState->input_state_ptr->buttons_pressed & 0x40) != 0) {
                goto after_menu;
            }
        }
        result = CheckCheatCodeInput(gameState);
        if ((result & 0xff) != 0) {
            PauseGameWithFadeOut(gameState);
        }
    } else if (gameState->menu_active == 0) {
        if (gameState->debug_pause_enable == 0) {
            if (gameState->level_active != 0) {
                PauseGameAndShowMenu(gameState);
            }
        } else {
            gameState->debug_pause_active = (gameState->debug_pause_active == 0);
        }
    } else {
        result = ToggleMenuOptionState(gameState->hud_entity_ptr);
        if ((result & 0xff) == 1) {
            PauseGameWithFadeOut(gameState);
        } else if ((result & 0xff) == 2) {
            SetNextLevelFlagWithFade((int)(intptr_t)gameState);
        }
    }

after_menu:
    if ((gameState->debug_pause_active != 0) &&
        ((gameState->input_state_ptr->buttons_pressed & 0x800) == 0) &&
        ((result = CheckCheatCodeInput(gameState), (result & 0xff) != 0))) {
        if (gameState->debug_pause_active == 0) {
            PauseGameWithFadeOut(gameState);
        } else {
            gameState->debug_pause_active = 0;
        }
    }
    if (gameState->menu_active != 0) {
        goto after_level_logic;
    }
    if (gameState->advance_level_flag == 0) {
        if (gameState->next_level_flag != 0) {
            SeekToLevelInSequence(&gameState->level_context, 0, 1, 0);
            if (!SetupAndStartLevel(gameState, 99)) {
                initPlayerState(g_pPlayerState);
            }
        }
    } else if (gameState->boss_defeated == 0) {
        if (g_pPlayerState->lives == 0) {
            AdvanceLevelSequence(&gameState->level_context);
            if (!SetupAndStartLevel(gameState, 99)) {
                initPlayerState(g_pPlayerState);
            }
        } else {
            RespawnAfterDeath(gameState);
        }
    } else {
        SetupAndStartLevel(gameState, gameState->boss_facing);
    }
    if (gameState->direct_level_load != 0) {
        SetupAndStartLevel(gameState, gameState->direct_level_load);
    }
    if (gameState->checkpoint_restore_pending != 0) {
        if (gameState->checkpoint_active == 0) {
            SaveCheckpointState(gameState);
        }
        if (gameState->checkpoint_restore_pending != 0) {
            goto skip_restore;
        }
    }
    if (gameState->checkpoint_active != 0) {
        RestoreCheckpointEntities(gameState);
    }
skip_restore:
    if ((gameState->level_clear_pending != 0) &&
        (gameState->level_transition_complete == 0)) {
        ClearEntitiesAndFadeToBlack(gameState);
    }

after_level_logic:
    if ((gameState->demo_return_flag != 0) &&
        (gameState->input_state_ptr->playback_active == 0)) {
        gameState->demo_return_flag = 0;
        SeekToLevelInSequence(&gameState->level_context, 0, 1, 0);
        if (!SetupAndStartLevel(gameState, 99)) {
            initPlayerState(g_pPlayerState);
        }
    }
    if ((gameState->menu_active == 0) && (gameState->checkpoint_active == 0)) {
        SpawnEntitiesAlternateSystem(gameState);
        SpawnOnScreenEntities(gameState);
    }
    if (gameState->debug_pause_active == 0) {
        EntityTickLoopWithCamera(gameState);
    }
    if ((gameState->menu_active == 0) && (gameState->level_clear_pending != 0) &&
        (gameState->level_transition_complete == 0)) {
        ClearEntitiesAndFadeToBlack(gameState);
    }
}
