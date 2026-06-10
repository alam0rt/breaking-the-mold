#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "common.h"
#include "Game/entity.h"
#include "Game/input_state.h"
#include "Game/level_data_context.h"

/* =============================================================================
 * GameState
 * 
 * Central game state structure at 0x8009DC40 (PAL).
 * Initialized by InitGameState @ 0x8007CD34.
 * Size: 0x1A0 (416 bytes)
 * 
 * Key functions that use this struct:
 *   - InitGameState @ 0x8007CD34 (initialization, password list building)
 *   - SetupAndStartLevel @ 0x8007D8A0 (level loading, player type selection)
 *   - SpawnPlayerAndEntities @ 0x8007DF38 (player/camera creation)
 *   - SaveCheckpointState @ 0x8007EAAC (checkpoint backup)
 *   - EntityTickLoop @ 0x80020E1C (entity updates via tick_list_head)
 * 
 * Player type selection based on level flags (SpawnPlayerAndEntities):
 *   0x0400 -> FINN vehicle
 *   0x0200 -> Menu entity
 *   0x2000 -> Results/ending screen (CreateResultsScreenEntity; credits variant when boss_player_type set - formerly mislabeled "Boss player")
 *   0x0100 -> RUNN vehicle
 *   0x0010 -> SOAR flying
 *   0x0004 -> GLIDE platformer
 *   default -> Normal platformer (CreatePlayerEntity)
 * ============================================================================= */
typedef struct {
    /* 0x00 */ s32  mode_base_offset;            /* Always 0xFFFF0000, sets entity callback mode */
    /* 0x04 */ void *mode_callback_ptr;          /* GameModeCallback function pointer */
    
    /* Layer list heads (0x08-0x18) - from ClearTickAndRenderLists @ 0x80020c54
       Each layer type has its own linked list for z-sorting with entities */
    /* 0x08 */ void *layer_list_static;          /* Static layer list head (AddLayerToRenderList_Static) */
    /* 0x0C */ void *layer_list_scrolling;       /* Scrolling layer list head (AddLayerToRenderList_Scrolling) */
    /* 0x10 */ void *layer_list_parallax;        /* Parallax layer list head (AddLayerToRenderList_Parallax) */
    /* 0x14 */ void *layer_list_standard;        /* Standard layer list head (AddLayerToRenderList_Standard) */
    /* 0x18 */ void *layer_render_context_ptr;   /* Layer render context pointer */
    
    /* Entity lists (0x1C-0x33) */
    /* 0x1C */ EntityListNode *tick_list_head;   /* Z-sorted entity tick list (iterated by EntityTickLoop) */
    /* 0x20 */ EntityListNode *render_list_head; /* Z-sorted entity render list */
    /* 0x24 */ EntityListNode *collision_list_head; /* Entity collision/update queue */
    /* 0x28 */ EntityListNode *entity_spawn_list;   /* Raw 24-byte entity defs from Asset 501 */
    /* 0x2C */ void *player_entity_alt;          /* Player entity (alternate reference for collision) */
    /* 0x30 */ void *player_entity_ptr;          /* Main player entity pointer */
    
    /* Deferred entity removal (0x34-0x3B) - from MarkEntityForDeferredRemoval @ 0x80020d74 */
    /* 0x34 */ void *pending_removal_entity;     /* Entity marked for deferred removal */
    /* 0x38 */ u32  removal_mode;                /* 0=all lists, 1=keep render, 2=normal */
    
    /* Previous spawn list (0x3C-0x43) - from SetupAndStartLevel @ 0x8007d8a0 */
    /* 0x3C */ void *previous_spawn_list;        /* Previous spawn list (copied from 0x13C during setup) */
    /* 0x40 */ void *blb_header_ptr;             /* Pointer to BLB header buffer (blbHeaderBufferBase @ 0x800AE3E0) */
    
    /* Camera position (0x44-0x4B) - from UpdateCameraPositionSmooth @ 0x800233c0 */
    /* 0x44 */ s16  camera_x;                    /* Camera X position (pixels) */
    /* 0x46 */ s16  camera_y;                    /* Camera Y position (pixels) */
    /* 0x48 */ s16  camera_limit_x;              /* Level width for camera X clamping */
    /* 0x4A */ s16  camera_limit_y;              /* Level height for camera Y clamping */
    
    /* Camera velocity (0x4C-0x53) - 16.16 fixed-point */
    /* 0x4C */ s32  camera_velocity_x;           /* Camera X velocity (16.16 fixed-point) */
    /* 0x50 */ s32  camera_velocity_y;           /* Camera Y velocity (16.16 fixed-point) */
    
    /* Player render offset (0x54-0x57) */
    /* 0x54 */ s16  player_render_offset_x;
    /* 0x56 */ s16  player_render_offset_y;
    
    /* Scroll limit flags (0x58-0x5B) */
    /* 0x58 */ u8   scroll_limit_left;
    /* 0x59 */ u8   scroll_limit_top;
    /* 0x5A */ u8   scroll_limit_right;
    /* 0x5B */ u8   scroll_limit_bottom;
    
    /* Camera sub-pixel (0x5C-0x5F) */
    /* 0x5C */ u16  camera_subpixel_x;
    /* 0x5E */ u16  camera_subpixel_y;
    
    /* Player state flags (0x60-0x63) - Used by PlayerCallback functions */
    /* 0x60 */ u8   bounce_active_flag;          /* Set to 1 during bounce/pickup, cleared on landing */
    /* 0x61 */ u8   camera_mode_flags;
    /* 0x62 */ u8   camera_follow_direction;     /* Set to player facing for camera tracking, cleared on knockback end */
    /* 0x63 */ u8   pause_freeze_flag;           /* Freezes spawning, also set by SaveCheckpointState */
    
    /* Player hitbox (0x64-0x67) */
    /* 0x64 */ s16  player_hitbox_width;         /* 0x28 for normal, 0 for boss/glide */
    /* 0x66 */ s16  player_hitbox_y_offset;      /* 0xFFD0 (-48) normal, 0xFFB0 (-80) FINN, 0 glide */
    
    /* Tile collision state (0x68-0x73) - from InitTileAttributeState @ 0x80024cf4 */
    /* 0x68 */ void *tile_collision_data_ptr;    /* Tile collision attribute array (from Asset 500) */
    /* 0x6C */ s16  tile_collision_offset_x;     /* Tile collision offset X (tiles) */
    /* 0x6E */ s16  tile_collision_offset_y;     /* Tile collision offset Y (tiles) */
    /* 0x70 */ s16  tile_collision_width;        /* Tile collision map width (tiles) */
    /* 0x72 */ s16  tile_collision_height;       /* Tile collision map height (tiles) */
    
    /* Level/layer state (0x74-0x7B) - from InitLayersAndTileState @ 0x80024778 */
    /* 0x74 */ void *animated_tile_data_ptr;     /* LevelDataContext+0x3C (AnimOffsets/ctx[12] for animated tiles) */
    /* 0x78 */ u16  scroll_limit_height;         /* TileHeader+0x1C (vertical scroll limit, 0 for bosses) */
    /* 0x7A */ u16  _pad_7A;                     /* Padding to 4-byte alignment */
    
    /* Entity callback system (0x7C-0x83) */
    /* 0x7C */ void *entity_callback_table;      /* EntityTypeCallback[121] at 0x8009D5F8 */
    /* 0x80 */ u16  entity_type_count;           /* Always 0x79 (121) */
    /* 0x82 */ u16  _pad_82;                     /* Padding to 4-byte alignment */
    
    /* Embedded LevelDataContext (0x84-0x103, 128 bytes) */
    /* 0x84 */ LevelDataContext level_context;
    
    /* Post-LevelDataContext fields (0x104-0x10F) */
    /* 0x104 */ u16  tile_render_state_count;    /* Total tile count + 1 (from GetTotalTileCount) */
    /* 0x106 */ u16  _pad_106;                   /* Padding to 4-byte alignment */
    /* 0x108 */ void *tile_render_state_ptr;     /* Tile lookup table (8 bytes per tile: TPage, CLUT, UV) */
    /* 0x10C */ u32  frame_counter;              /* Frame counter (incremented in EntityTickLoopWithCamera) */
    
    /* Palette/spawn info (0x110-0x11F) - from LoadTileDataToVRAM @ 0x80025240 */
    /* 0x110 */ void *palette_group_ptrs;        /* Palette render context pointer array */
    /* 0x114 */ u8   palette_group_count;        /* Count of palette groups (byte) */
    /* 0x115 */ u8   _pad_115;                   /* Padding to 2-byte alignment */
    /* 0x116 */ s16  spawn_x;                    /* Player spawn X position */
    /* 0x118 */ s16  spawn_y;                    /* Player spawn Y position */
    /* 0x11A */ u16  screen_shake_index;         /* Index into shake lookup table */
    /* 0x11C */ u32  camera_scroll_speed;        /* 0x8000, 0xC000, or 0x10000 based on level flags */
    /* 0x120 */ s16  glide_boss_state_x;         /* Glide/boss/FINN state X (zeroed in SpawnPlayerAndEntities) */
    /* 0x122 */ s16  glide_boss_state_y;         /* Glide/boss/FINN state Y (zeroed in SpawnPlayerAndEntities) */
    /* 0x124 */ u8   layer0_tint_r;              /* Layer 0 R color tint (from LayerEntry+0x2C) */
    /* 0x125 */ u8   layer0_tint_g;              /* Layer 0 G color tint (from LayerEntry+0x2D) */
    /* 0x126 */ u8   layer0_tint_b;              /* Layer 0 B color tint (from LayerEntry+0x2E) */
    /* 0x127 */ u8   layer1_tint_r;              /* Layer 1 R color tint (observed same as layer0 in dumps) */
    /* 0x128 */ u8   layer1_tint_g;              /* Layer 1 G color tint */
    /* 0x129 */ u8   layer1_tint_b;              /* Layer 1 B color tint */
    /* 0x12A */ u8   glide_boss_flag;            /* Glide/boss/FINN flag (zeroed in SpawnPlayerAndEntities) */
    /* 0x12B */ u8   _pad_12B;                   /* Padding to 4-byte alignment */
    /* 0x12C */ u32  heap_debug_marker;          /* Debug sentinel (set to 0x01234567 during init, changes at runtime) */
    
    /* Background color update (0x130-0x133) - from RenderEntities */
    /* 0x130 */ u8   bg_color_change_flag;       /* Triggers BG color update in RenderEntities */
    /* 0x131 */ u8   bg_color_r_pending;         /* Pending BG R component */
    /* 0x132 */ u8   bg_color_g_pending;         /* Pending BG G component */
    /* 0x133 */ u8   bg_color_b_pending;         /* Pending BG B component */
    
    /* Checkpoint system (0x134-0x14F) */
    /* 0x134 */ void *checkpoint_entity_list;    /* Saved entity tick list for checkpoint respawn */
    /* 0x138 */ u32  checkpoint_saved_score;     /* Saves frame_counter at checkpoint */
    /* 0x13C */ void *entity_defs_backup;        /* Backed up entity defs ptr in ClearEntitiesAndFadeToBlack */
    /* 0x140 */ InputState *input_state_ptr;     /* Input state pointer, also stored in player entity[0x40] */
    /* 0x144 */ u8   level_clear_pending;        /* Level clear pending flag (triggers ClearEntitiesAndFadeToBlack) */
    /* 0x145 */ u8   level_transition_complete;  /* Set after ClearEntitiesAndFadeToBlack runs */
    /* 0x146 */ u8   advance_level_flag;         /* Advance to next level (calls AdvanceLevelSequence) */
    /* 0x147 */ u8   next_level_flag;            /* Next level flag (set by SetNextLevelFlagWithFade) */
    /* 0x148 */ u8   direct_level_load;
    /* 0x149 */ u8   checkpoint_restore_pending; /* Checkpoint restore pending, zeroed after restore */
    /* 0x14A */ u8   checkpoint_active;          /* Checkpoint active flag, set by SaveCheckpointState */
    /* 0x14B */ u8   checkpoint_powerup_state;   /* Saved powerup state, restored to PlayerState+0x18 on respawn */
    
    /* HUD/Menu entity (0x14C-0x14F) */
    /* 0x14C */ void *hud_entity_ptr;            /* HUD entity created by CreateMenuEntities */
    
    /* Pause system (0x150-0x163) - from PauseGameAndShowMenu */
    /* 0x150 */ u8   menu_active;                /* Pause menu active flag */
    /* 0x151 */ u8   fade_out_active;            /* Fade-out in progress flag */
    /* 0x152 */ u8   demo_return_flag;           /* Return from demo to menu flag */
    /* 0x153 */ u8   _pad_153;                   /* Padding */
    /* 0x154 */ u32  saved_frame_counter;        /* Saved frame counter for pause restore */
    /* 0x158 */ u8   saved_freeze_flag;          /* Saved pause state byte */
    /* 0x159 */ u8   _pad_159[3];                /* Padding to 4-byte alignment */
    /* 0x15C */ void *saved_tick_list;           /* Saved tick list for pause restore */
    /* 0x160 */ u8   pause_countdown;            /* Pause fade countdown (0x16 = 22 frames) */
    /* 0x161 */ u8   spawn_freeze_flag;          /* Spawn freezing flag (set by checkpoint) */
    /* 0x162 */ u8   _pad_162[2];                /* Padding to 4-byte alignment */
    
    /* Alternate entity spawn system (0x164-0x16F) - from SpawnEntitiesAlternateSystem @ 0x80081d0c */
    /* 0x164 */ void *alternate_entity_data;     /* 64-byte stride entity array (from GetVehicleDataPtr) */
    /* 0x168 */ u16  alternate_entity_count;     /* Count of alternate entities (TileHeader field 16) */
    /* 0x16A */ u16  _pad_16A;                   /* Padding to 4-byte alignment */
    /* 0x16C */ void *entity_pool_ptr;           /* Entity pool (256-entry array, allocated for special modes) */
    
    /* Level active + password (0x170-0x17B) */
    /* 0x170 */ u8   level_active;               /* Set to 1 if level has valid asset index, 0 otherwise */
    /* 0x171 */ u8   password_levels[10];        /* Password-selectable level list (max 10, built by InitGameState) */
    /* 0x17B */ u8   password_level_count;       /* Count of password-selectable levels */
    
    /* Cheat code input buffer (0x17C-0x18B) - 8 entries x 2 bytes
       Used by CheckCheatCodeInput @ 0x800820b4 to store last 8 button presses.
       Cheat sequences are compared against g_CheatCodeTable entries.
       Despite name, this is NOT score display - it's a circular cheat input buffer. */
    /* 0x17C */ u16  cheat_input_buffer[8];      /* Last 8 button presses for cheat detection */
    
    /* Cheat/debug flags (0x18C-0x198) - from CheckCheatCodeInput @ 0x800820b4 */
    /* 0x18C */ u8   cheat_input_index;          /* Circular index into cheat_input_buffer (0-7) */
    /* 0x18D */ u8   player_readd_flag;          /* Player re-add to render list flag (cheat 0x13, UnpauseGame) */
    /* 0x18E */ u8   boss_player_type;           /* Boss player type index (0=KLOGG,1=BIRDHEAD,2=JOE_HEAD) */
    /* 0x18F */ u8   debug_pause_enable;         /* Debug frame-step enable (cheat 0x0F toggles) */
    /* 0x190 */ u8   debug_pause_active;         /* Debug frame-step state (toggled when debug_pause_enable set) */
    /* 0x191 */ u8   _pad_191[3];                /* Padding (unused) */
    /* 0x194 */ u32  _reserved_194;              /* Reserved/unused - cleared in InitGameState, always 0 */
    /* 0x198 */ u8   _reserved_198;              /* Reserved/unused - cleared in InitGameState, always 0 */
    
    /* Background colors (0x199-0x19B) */
    /* 0x199 */ u8   bg_color_r;                 /* Background R component (default 0x40) */
    /* 0x19A */ u8   bg_color_g;                 /* Background G component (default 0x40) */
    /* 0x19B */ u8   bg_color_b;                 /* Background B component (default 0x40) */
    
    /* Boss defeat state (0x19C-0x19F) - Set by boss defeat callback @ 0x8004906c */
    /* 0x19C */ u8   boss_defeated;              /* Set to 1 when boss HP reaches 0 */
    /* 0x19D */ u8   boss_facing;                /* Boss facing direction for cutscene (from entity+0x50) */
    /* 0x19E */ u8   _pad_19E[2];                /* Padding to 0x1A0 total size */
} GameState;  /* Size: 0x1A0 (416 bytes) */

#endif /* GAME_STATE_H */
