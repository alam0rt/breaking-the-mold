/* =============================================================================
 * Skullmonkeys (PAL SLES-01090) Global Variables
 * 
 * This header documents global variables in the data/sdata sections.
 * 
 * Memory Layout:
 *   .data  : 0x8009B000 - 0x800A5950 (~43KB)
 *   .sdata : 0x800A5950 - 0x800A60C4 (~2.9KB, GP-relative)
 *   .rodata: 0x800A60C4 - ~0x800A6500 (strings)
 * 
 * See docs/reference/data-section-map.md for full memory map.
 * ============================================================================= */

#ifndef GLOBALS_H
#define GLOBALS_H

#include "common.h"
#include "Game/game_state.h"

/* =============================================================================
 * PRIMARY GAME STRUCTURES
 * ============================================================================= */

/* Main game state structure
 * Address: 0x8009DC40
 * Size: 0x1A0 (416 bytes)
 * See game_state.h for full struct definition
 */
extern GameState g_GameState;

/* LevelDataContext is embedded at g_GameState + 0x84 (0x8009DCC4)
 * Size: 0x80 (128 bytes)
 * See level_data_context.h for struct definition
 */

/* =============================================================================
 * ENTITY TYPE CALLBACK TABLE
 * 
 * Address: 0x8009D5F8
 * Size: 968 bytes (121 entries × 8 bytes)
 * 
 * Maps entity types (0-120) to initialization callbacks.
 * Stored in GameState+0x7C at runtime.
 * ============================================================================= */

extern EntityTypeCallback g_EntityTypeCallbackTable[121];

/* =============================================================================
 * PLAYER SPRITE TABLES
 * ============================================================================= */

/* Main player sprite ID table (56 entries)
 * Address: 0x8009C174
 * Used by CreatePlayerEntity to select animation sprites
 */
extern u32 g_PlayerSpriteTable[56];

/* Player sprite variants (7 entries)
 * Address: 0x8009C3A8
 */
extern u32 g_PlayerSpriteVariants[7];

/* =============================================================================
 * MENU/UI SPRITE TABLES
 * ============================================================================= */

/* Menu cursor sprites (3 entries)
 * Address: 0x8009CBDC
 */
extern u32 g_MenuCursorSprites[3];

/* Menu button sprites (4 entries)
 * Address: 0x8009CBE8
 */
extern u32 g_MenuButtonSprites[4];

/* Menu logo sprites (4 entries)
 * Address: 0x8009CBF8
 */
extern u32 g_MenuLogoSprites[4];

/* Menu background sprites (4 entries)
 * Address: 0x8009CC08
 */
extern u32 g_MenuBackgroundSprites[4];

/* Menu background color presets (5 colors × 3 bytes RGB)
 * Address: 0x8009CBAC
 */
extern u8 g_MenuBackgroundColorTable[15];

/* =============================================================================
 * COLLECTIBLE SPRITE TABLES
 * ============================================================================= */

/* Special pickup sprites (4 entries) - InitSpecialPickupEntity @ 0x8003cfd8 */
extern u32 g_SpecialPickupSprites[4];         /* 0x8009B4DC */

/* Walking collectible sprites (7 entries) - InitWalkingCollectibleEnemy @ 0x8003d638 */
extern u32 g_WalkingCollectibleSprites[7];    /* 0x8009B4EC */

/* Tempest pulsing monkey sprites (7 entries) - InitTempestPulsingMonkey @ 0x8003e0fc */
extern u32 g_TempestPulsingMonkeySprites[7];  /* 0x8009B508 */

/* Sound emitting enemy sprites (3 entries) - InitSoundEmittingEnemy @ 0x8003e950 */
extern u32 g_SoundEmittingEnemySprites[3];    /* 0x8009B524 */

/* CUT CONTENT - Unused sprite IDs not in BLB
 * Address: 0x8009B530
 */
extern u32 g_CutEntitySprites_UNUSED[3];

/* Level state collectible sprites (4 entries) - InitLevelStateCollectible @ 0x8003f704 */
extern u32 g_LevelStateCollectibleSprites[4]; /* 0x8009B53C */

/* Collectible variant sprites (4 entries) - InitCollectibleVariant @ 0x8003fe90 */
extern u32 g_CollectibleVariantSprites[4];    /* 0x8009B54C */

/* =============================================================================
 * BOSS SPRITE TABLES
 * ============================================================================= */

/* Monkey Mage boss sprites (7 entries) - InitMonkeyMageBoss @ 0x80047fb8 */
extern u32 g_MonkeyMageBossSprites[7];        /* 0x8009BA48 */

/* Glenn Yntis boss sprites (9 entries) - InitGlennYntisBoss @ 0x80049a54 */
extern u32 g_GlennYntisBossSprites[9];        /* 0x8009BA64 */

/* Shriney Guard boss sprites (9 entries) - InitShrineyGuardBoss @ 0x8004af64 */
extern u32 g_ShrineyGuardBossSprites[9];      /* 0x8009BA88 */

/* Joe Head Joe boss sprites (8 entries) - InitJoeHeadJoeBoss @ 0x8004c0e0 */
extern u32 g_JoeHeadJoeBossSprites[8];        /* 0x8009BAAC */

/* Klogg boss sprites (6 entries) - InitKloggBoss @ 0x8004d88c */
extern u32 g_KloggBossSprites[6];             /* 0x8009BACC */

/* =============================================================================
 * ENEMY SPRITE TABLES
 * ============================================================================= */

/* Enemy AI sprites (3 entries) - InitEnemyEntityWithAI @ 0x8004f8dc */
extern u32 g_EnemyAISprites[3];               /* 0x8009BBE0 */

/* Joe Head Joe ball regular sprites (4 entries) - InitJoeHeadJoeBallRegular @ 0x80053afc */
extern u32 g_JoeHeadJoeBallRegularSprites[4]; /* 0x8009BBEC */

/* Projectile sprites (3 entries) - InitProjectileWithTimer @ 0x80050970 */
extern u32 g_ProjectileSprites[3];            /* 0x8009BBFC */

/* Joe Head Joe ball special sprites (3 entries) */
extern u32 g_JoeHeadJoeBallSprites[3];        /* 0x8009C0E8 */

/* =============================================================================
 * TRIGGER ZONE COLORS
 * 
 * Address: 0x8009D9C0
 * Size: 60 bytes (20 entries × 3 bytes RGB)
 * Used by HandleGenericTriggerZone for color-coded zones
 * ============================================================================= */

extern u8 g_TriggerZoneColorTable[60];

/* =============================================================================
 * CD-ROM / BLB STATE (.data section)
 * ============================================================================= */

/* CdlFILE structure for GAME.BLB
 * Address: 0x8009B4B4
 * Size: 40 bytes (PSY-Q CdlFILE struct)
 */
extern u8 g_GameBLBFile[40];

/* =============================================================================
 * SDATA SECTION (GP-relative globals)
 * ============================================================================= */

/* Frame sync flags */
extern u32 g_FrameReady;           /* 0x800A5948 - Frame complete flag */
extern u32 g_SkipVSync;            /* 0x800A594C - Skip VSync flag */

/* Game flags (bit 0x80 = debug menu enabled) */
extern u32 g_GameFlags;            /* 0x800A5950 */

/* Active game state pointer */
extern GameState* g_pGameState;    /* 0x800A5960 */

/* Input state pointers */
extern void* g_pPlayerState;       /* 0x800A5754 - Player persistent state */
extern void* g_pPlayer1Input;      /* 0x800A5764 - Player 1 input */
extern void* g_pPlayer2Input;      /* 0x800A5768 - Player 2 input */
extern void* g_pCurrentInputState; /* 0x800A576C - Active input */

/* Default background color */
extern u8 g_DefaultBGColorR;       /* 0x800A5770 */
extern u8 g_DefaultBGColorG;       /* 0x800A5771 */
extern u8 g_DefaultBGColorB;       /* 0x800A5772 */

/* BLB sector location */
extern u32 g_GameBLBSector;        /* 0x800A59F0 - Starting sector (0x146) */

/* Level context pointers */
extern void* g_pSecondarySpriteBank; /* 0x800A6060 - Secondary sprites (often NULL) */
extern void* g_pLevelDataContext;    /* 0x800A6064 - Current level context */

/* Current game mode (0-6) */
extern u8 g_CurrentGameMode;       /* 0x800A6082 */

/* =============================================================================
 * DEBUG MENU (enabled when g_GameFlags & 0x80)
 * ============================================================================= */

/* Debug menu item name pointers (10 entries)
 * Address: 0x8009DDE0
 * Points to strings at 0x800A60C4: "sub 01", "sub 02", etc.
 */
extern char* g_MenuItemNames[10];

/* =============================================================================
 * SPU/AUDIO BUFFERS
 * ============================================================================= */

/* SPU voice pitch save buffer (24 voices × 2 bytes)
 * Address: 0x8009CC30
 * Used during pause/resume
 */
extern u16 g_SPUVoicePitchBuffer[24];

/* =============================================================================
 * BLB HEADER (loaded at runtime)
 * 
 * Address: 0x800AE3E0
 * Loaded when game boots, contains level/movie metadata
 * ============================================================================= */

extern u8 blbHeaderBufferBase[];  /* 0x800AE3E0 */

/* Key offsets within BLB header:
 *   +0xF31: Level count (u8, value=26)
 *   +0xF32: Movie count (u8, value=13)
 *   +0xF36: Game mode (u8, 3=level, 6=special)
 *   +0xF92: Current level index being loaded
 */

#endif /* GLOBALS_H */
