#include "common.h"
#include "Game/game_state.h"

extern u16 GetLevelFlags(LevelDataContext *levelCtx);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", InitializeAndLoadLevel);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", SetupAndStartLevel);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", DisplayTransitionSprite);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", SpawnPlayerAndEntities);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", DestroyEntityAndFreeResources);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", GameModeCallback);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", SaveCheckpointState);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", RestoreCheckpointEntities);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", RemoveCheckpointEntityById);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", PauseGameAndShowMenu);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", PauseGameWithFadeOut);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", SetNextLevelFlagWithFade);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", UnpauseGameAndRestoreEntities);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", ClearEntitiesAndFadeToBlack);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", HandleGenericTriggerZone);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", CleanupRespawnEntities);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType000_003_004_PickupVariant_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType086_087_088_InvisibleHazard_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType106_107_108_FallingPlatform_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType112_113_114_AnimatedDecor_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType115_116_117_ParallaxDecor_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType010_EggBeater_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType026_FlyingEnemy_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType027_FlyingEnemyVariant_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType076_PathEnemy_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType024_118_SpecialAmmo_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType099_LevelStateCollectible_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType109_TimedCollectible_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType084_PathEnemyAlt_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType096_PathPlatformEnemy_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType120_ChildSprite_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType001_BossEntity_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType005_MovingPlatformA_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType006_MovingPlatformB_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType012_MovingPlatformC_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType017_EnemyCluster_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType018_EnemyCluster_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType019_ClockPlatformA_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType020_ClockPlatformB_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType021_ClockPlatformC_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType049_BossRelated_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType050_BossMain_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType051_BossPart_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType057_WaypointClayballA_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType058_WaypointClayballB_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType059_WaypointClayballC_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType062_ClayballVariantA_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType063_ClayballVariantB_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType064_Particle_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType066_ClayballVariantC_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType067_ClayballVariantD_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType068_ClayballVariantE_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType090_SwitchClayballA_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType091_SwitchClayballB_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType092_SwitchClayballC_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType093_BonusClayball_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType002_GreenBullets_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType022_EnemyCluster_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType007_Clayball_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", func_80080420);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", SpawnRandomColorDecorEntity);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType011_ExtraLife_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType009_Collectible_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType023_PhoenixHand_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType025_PhartHeadCollectible_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType028_GreenHeart_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType029_ScaleReset_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType061_UniverseEnema_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType069_1970Icon_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType070_HamsterShield_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", func_80080810);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", SpawnPathDecor1Entity);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType072_SuperWillie_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType075_YellowBird_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType081_PathDecor2_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", func_80080960);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", SpawnPathDecor2Entity);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType083_InteractiveDecor_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType119_Checkpoint_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType030_CollectibleAlt_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType031_032_033_ScaledMovingPlatform_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType034_035_036_TriggerPlatform_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType037_038_DirectionalScaled_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType046_Object_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType039_052_FloatingEnemy_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType040_Mechanism_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType041_ClayKeeperEnemy_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType042_thru_060_SnoBlo_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType047_048_SoundPlatform_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType080_TimerMenu_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType045_BounceClay_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType065_Password_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType071_Boss_MonkeyMage_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType100_Boss_GlennYntis_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType101_Boss_ShrineyGuard_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType102_Boss_JoeHeadJoe_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType103_Boss_Klogg_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType079_EnemySpawner_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType082_PathHazard_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType085_104_105_TriggerZone_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType089_097_098_110_111_GridHelper_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType094_IndexedSprite_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType095_SoundEmitterPanning_Init);

void EntityType008_KloggCatchableBall_Init(void) {
}

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", RemapEntityTypesForLevel);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", LoadLevelSpriteAssets);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", GetSlopeHeightAtSubpixel);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", SetPlayerVehicleSpeed);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", SpawnEntitiesAlternateSystem);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", ClearAlternateEntitySpawnFlags);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", AddToDepthBucket);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", FlushDepthBuckets);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", ClearEntityPoolArray);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", IsNormalPlatformLevel);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", CheckCheatCodeInput);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", func_800826C0);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", func_800826E4);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", func_800826F4);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", func_80082700);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", FindSaveSlotForCurrentLevel);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", func_80082730);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", func_8008273C);

u8 GetLevelDebugFlag(GameState *gameState) {
    return GetLevelFlags(&gameState->level_context) >> 0xf;
}

u8 GetLevelShowHUDFlag(GameState *gameState) {
    return (GetLevelFlags(&gameState->level_context) >> 0xe) & 1;
}

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", func_80082790);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", func_8008279C);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", func_800827A4);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", func_800827AC);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", func_800827B8);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", func_800827C0);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", func_800827CC);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", func_800827D8);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", func_800827E4);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", func_800827EC);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", func_800827F4);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", func_80082800);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", func_80082808);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", func_80082810);

u8 GetLevelAutoScrollFlag(GameState *gameState) {
    return (GetLevelFlags(&gameState->level_context) >> 0xb) & 1;
}

void func_8008283C(void) {
}

void func_80082844(void) {
}

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", SpecialEntityDestroyCallback_2120);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", FreeSpecialEntity2120Memory);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", main);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", func_80082BE8);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", ProcessDebugMenuInput);
