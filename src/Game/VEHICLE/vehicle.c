#include "common.h"
#include "Game/game_state.h"

extern u16 GetLevelFlags(LevelDataContext *levelCtx);
extern void *D_800A5954;
extern void FreeFromHeap(void *heap, void *ptr, s32 mode, s32 flags);
extern void PlaySoundEffect(u32 soundId, s32 volume, s32 param);
extern void HidePauseMenuHUD(s32 handle);
extern u8 D_80012120[];

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", InitializeAndLoadLevel);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", SetupAndStartLevel);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", DisplayTransitionSprite);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", SpawnPlayerAndEntities);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", DestroyEntityAndFreeResources);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", GameModeCallback);

extern void AddToZOrderList(u8 *obj, s32 zOrder);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", SaveCheckpointState);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", RestoreCheckpointEntities);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", RemoveCheckpointEntityById);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", PauseGameAndShowMenu);

void PauseGameWithFadeOut(u8 *obj) {
    PlaySoundEffect(0x4C60F249, 0xA0, 1);
    obj[0x151] = 1;
    HidePauseMenuHUD(*(s32 *)&obj[0x14C]);
    obj[0x160] = 0x16;
}

void SetNextLevelFlagWithFade(u8 *obj) {
    obj[0x151] = 1;
    obj[0x160] = 0x16;
    obj[0x147] = 1;
}

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", UnpauseGameAndRestoreEntities);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", ClearEntitiesAndFadeToBlack);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", HandleGenericTriggerZone);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", CleanupRespawnEntities);

extern u8 D_8009D9FC[];
extern void *CreateCollectibleWithFlags(void *entity, void *spawnData, u8 *flags, s32 arg3, s32 arg4);

void EntityType000_003_004_PickupVariant_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x120, 1, 0);
    entity = CreateCollectibleWithFlags(entity, spawnData, D_8009D9FC, 0, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType086_087_088_InvisibleHazard_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType106_107_108_FallingPlatform_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType112_113_114_AnimatedDecor_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType115_116_117_ParallaxDecor_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType010_EggBeater_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType026_FlyingEnemy_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType027_FlyingEnemyVariant_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType076_PathEnemy_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType024_118_SpecialAmmo_Init);

extern void *InitLevelStateCollectible(void *entity, void *spawnData);

void EntityType099_LevelStateCollectible_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x114, 1, 0);
    entity = InitLevelStateCollectible(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitAnimatedTimedCollectible(void *entity, void *spawnData);

void EntityType109_TimedCollectible_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x114, 1, 0);
    entity = InitAnimatedTimedCollectible(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType084_PathEnemyAlt_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType096_PathPlatformEnemy_Init);

extern void *InitEntityWithChildSprite(void *entity, void *spawnData);

void EntityType120_ChildSprite_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x11C, 1, 0);
    entity = InitEntityWithChildSprite(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitGenericSpriteEntity(void *entity, void *spawnData, u32 hash, s32 arg3, s32 arg4);
extern void *InitClockPlatformWithTimer(void *entity, void *spawnData, u32 hash, s32 arg3);
extern void *InitClayballOnPath(void *entity, void *spawnData, u32 hash, s32 arg3);
extern void *InitClayballAtWaypoint(void *entity, void *spawnData, u32 hash, s32 arg3);
extern void *InitClayballWithSwitchBlock(void *entity, void *spawnData, u32 hash, s32 arg3);

void EntityType001_BossEntity_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x124, 1, 0);
    entity = InitGenericSpriteEntity(entity, spawnData, 0x98F8221E, 0, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType005_MovingPlatformA_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x124, 1, 0);
    entity = InitGenericSpriteEntity(entity, spawnData, 0x88783718, 0, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType006_MovingPlatformB_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x124, 1, 0);
    entity = InitGenericSpriteEntity(entity, spawnData, 0x8818A018, 0, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType012_MovingPlatformC_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x124, 1, 0);
    entity = InitGenericSpriteEntity(entity, spawnData, 0x9299C307, 1, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType017_EnemyCluster_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x124, 1, 0);
    entity = InitGenericSpriteEntity(entity, spawnData, 0x93C9A20F, 1, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType018_EnemyCluster_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x124, 1, 0);
    entity = InitGenericSpriteEntity(entity, spawnData, 0x9AB9A209, 1, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType019_ClockPlatformA_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x13C, 1, 0);
    entity = InitClockPlatformWithTimer(entity, spawnData, 0x93043811, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType020_ClockPlatformB_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x13C, 1, 0);
    entity = InitClockPlatformWithTimer(entity, spawnData, 0xD2801814, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType021_ClockPlatformC_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x13C, 1, 0);
    entity = InitClockPlatformWithTimer(entity, spawnData, 0x12800031, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType049_BossRelated_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x128, 1, 0);
    entity = InitClayballOnPath(entity, spawnData, 0x98F8221E, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType050_BossMain_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x128, 1, 0);
    entity = InitClayballOnPath(entity, spawnData, 0x88783718, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType051_BossPart_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x128, 1, 0);
    entity = InitClayballOnPath(entity, spawnData, 0x8818A018, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType057_WaypointClayballA_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x12C, 1, 0);
    entity = InitClayballAtWaypoint(entity, spawnData, 0x98F8221E, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType058_WaypointClayballB_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x12C, 1, 0);
    entity = InitClayballAtWaypoint(entity, spawnData, 0x88783718, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType059_WaypointClayballC_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x12C, 1, 0);
    entity = InitClayballAtWaypoint(entity, spawnData, 0x8818A018, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType062_ClayballVariantA_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x124, 1, 0);
    AddEntityToSortedRenderList(list, InitGenericSpriteEntity(entity, spawnData, 0x88A16190, 0, 0));
}

void EntityType063_ClayballVariantB_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x124, 1, 0);
    AddEntityToSortedRenderList(list, InitGenericSpriteEntity(entity, spawnData, 0xDCB92390, 0, 0));
}

void EntityType064_Particle_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x124, 1, 0);
    AddEntityToSortedRenderList(list, InitGenericSpriteEntity(entity, spawnData, 0x80B92212, 0, 0));
}

void EntityType066_ClayballVariantC_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x124, 1, 0);
    AddEntityToSortedRenderList(list, InitGenericSpriteEntity(entity, spawnData, 0xB69C8356, 0, 1));
}

void EntityType067_ClayballVariantD_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x124, 1, 0);
    AddEntityToSortedRenderList(list, InitGenericSpriteEntity(entity, spawnData, 0xB7CCE25E, 0, 1));
}

void EntityType068_ClayballVariantE_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x124, 1, 0);
    AddEntityToSortedRenderList(list, InitGenericSpriteEntity(entity, spawnData, 0xBEBCE258, 0, 1));
}

void EntityType090_SwitchClayballA_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x12C, 1, 0);
    entity = InitClayballWithSwitchBlock(entity, spawnData, 0xD89C319A, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType091_SwitchClayballB_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x12C, 1, 0);
    entity = InitClayballWithSwitchBlock(entity, spawnData, 0xC89E9158, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType092_SwitchClayballC_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x12C, 1, 0);
    entity = InitClayballWithSwitchBlock(entity, spawnData, 0xC48C7158, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType093_BonusClayball_Init);

extern void *InitGreenBulletsCollectible(void *entity, void *spawnData);

void EntityType002_GreenBullets_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x120, 1, 0);
    entity = InitGreenBulletsCollectible(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitDecorEntityWithHUDIcon(void *entity, void *spawnData);

void EntityType022_EnemyCluster_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x128, 1, 0);
    entity = InitDecorEntityWithHUDIcon(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType007_Clayball_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", func_80080420);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", SpawnRandomColorDecorEntity);

extern void *InitTransparentDecorEntity(void *entity, void *spawnData);

void EntityType011_ExtraLife_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x120, 1, 0);
    entity = InitTransparentDecorEntity(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *AllocateFromHeap(void *heap, s32 size, s32 align, s32 flags);
extern void *InitPlatformDecorEntity(void *entity, void *spawnData);
extern void AddEntityToSortedRenderList(void *list, void *entity);
extern void AddToUpdateQueue(void *list, void *entity);

void EntityType009_Collectible_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x124, 1, 0);
    entity = InitPlatformDecorEntity(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitPhoenixHandCollectible(void *entity, void *spawnData);

void EntityType023_PhoenixHand_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x120, 1, 0);
    entity = InitPhoenixHandCollectible(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitPhartHeadCollectible(void *entity, void *spawnData);

void EntityType025_PhartHeadCollectible_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x124, 1, 0);
    entity = InitPhartHeadCollectible(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitSingleFrameDecorEntity(void *entity, void *spawnData);

void EntityType028_GreenHeart_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x120, 1, 0);
    entity = InitSingleFrameDecorEntity(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitScaleResetCollectible(void *entity, void *spawnData);

void EntityType029_ScaleReset_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x120, 1, 0);
    entity = InitScaleResetCollectible(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitUniverseEnemaCollectible(void *entity, void *spawnData);

void EntityType061_UniverseEnema_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x120, 1, 0);
    entity = InitUniverseEnemaCollectible(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *Init1970IconEntity(void *entity, void *spawnData);

void EntityType069_1970Icon_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x120, 1, 0);
    entity = Init1970IconEntity(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType070_HamsterShield_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", func_80080810);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", SpawnPathDecor1Entity);

extern void *InitSuperWillieCollectible(void *entity, void *spawnData);

void EntityType072_SuperWillie_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x120, 1, 0);
    entity = InitSuperWillieCollectible(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitYellowBirdCollectible(void *entity, void *spawnData);

void EntityType075_YellowBird_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x120, 1, 0);
    entity = InitYellowBirdCollectible(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType081_PathDecor2_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", func_80080960);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", SpawnPathDecor2Entity);

extern void *InitInteractiveDecorEntity(void *entity, void *spawnData);

void EntityType083_InteractiveDecor_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x120, 1, 0);
    entity = InitInteractiveDecorEntity(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitCheckpointEntity(void *entity, void *spawnData);

void EntityType119_Checkpoint_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x10C, 1, 0);
    entity = InitCheckpointEntity(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType030_CollectibleAlt_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType031_032_033_ScaledMovingPlatform_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType034_035_036_TriggerPlatform_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType037_038_DirectionalScaled_Init);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType046_Object_Init);

extern void *InitFloatingPlatformEntity(void *entity, void *spawnData);

void EntityType039_052_FloatingEnemy_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x114, 1, 0);
    entity = InitFloatingPlatformEntity(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitPathFollowingEntity_Alt(void *entity, void *spawnData);

void EntityType040_Mechanism_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x120, 1, 0);
    entity = InitPathFollowingEntity_Alt(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitWalkingCollectibleEnemy(void *entity, void *spawnData);

void EntityType041_ClayKeeperEnemy_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x11C, 1, 0);
    entity = InitWalkingCollectibleEnemy(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitSnoBloEnemy(void *entity, void *spawnData);

void EntityType042_thru_060_SnoBlo_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x114, 1, 0);
    entity = InitSnoBloEnemy(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitSoundEmittingEnemy(void *entity, void *spawnData);

void EntityType047_048_SoundPlatform_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x114, 1, 0);
    entity = InitSoundEmittingEnemy(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType080_TimerMenu_Init);

extern void *InitBouncableClayEntity(void *entity, void *spawnData);

void EntityType045_BounceClay_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x114, 1, 0);
    entity = InitBouncableClayEntity(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType065_Password_Init);

extern void *InitMonkeyMageBoss(void *entity, void *spawnData);

void EntityType071_Boss_MonkeyMage_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x148, 1, 0);
    entity = InitMonkeyMageBoss(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitGlennYntisBoss(void *entity, void *spawnData);

void EntityType100_Boss_GlennYntis_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x11C, 1, 0);
    entity = InitGlennYntisBoss(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitShrineyGuardBoss(void *entity, void *spawnData);

void EntityType101_Boss_ShrineyGuard_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x120, 1, 0);
    entity = InitShrineyGuardBoss(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitJoeHeadJoeBoss(void *entity, void *spawnData);

void EntityType102_Boss_JoeHeadJoe_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x11C, 1, 0);
    entity = InitJoeHeadJoeBoss(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitKloggBoss(void *entity, void *spawnData);

void EntityType103_Boss_Klogg_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x118, 1, 0);
    entity = InitKloggBoss(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", EntityType079_EnemySpawner_Init);

extern void *InitPathFollowingHazard(void *entity, void *spawnData);

void EntityType082_PathHazard_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(D_800A5954, 0x114, 1, 0);
    entity = InitPathFollowingHazard(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

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

void func_800826C0(u8 *obj, u8 *outA, u8 *outB, u8 *outC) {
    *outA = obj[0x199];
    *outB = obj[0x19A];
    *outC = obj[0x19B];
}

void func_800826E4(u8 *obj, u8 a, u8 b, u8 c) {
    obj[0x199] = a;
    obj[0x19A] = b;
    obj[0x19B] = c;
}

u8 func_800826F4(u8 *obj) {
    return obj[0x198];
}

void func_80082700(u8 *obj, u8 val) {
    obj[0x198] = val;
}

extern u8 D_800A608C[];
extern s32 FindSaveSlotByName(u8 *name, u8 *slots);

s32 FindSaveSlotForCurrentLevel(u8 *obj) {
    return FindSaveSlotByName(&obj[0x84], D_800A608C);
}

void func_80082730(u8 *obj, u8 a, u8 b) {
    obj[0x19C] = a;
    obj[0x19D] = b;
}

u8 func_8008273C(u8 *obj) {
    return obj[0x19C];
}

u8 GetLevelDebugFlag(GameState *gameState) {
    return GetLevelFlags(&gameState->level_context) >> 0xf;
}

u8 GetLevelShowHUDFlag(GameState *gameState) {
    return (GetLevelFlags(&gameState->level_context) >> 0xe) & 1;
}

u8 func_80082790(u8 *obj) {
    return obj[0x14A];
}

void func_8008279C(u8 *obj, u8 val) {
    obj[0x152] = val;
}

void func_800827A4(u8 *obj, u8 val) {
    obj[0x170] = val;
}

u8 func_800827AC(u8 *obj) {
    return obj[0x161];
}

void func_800827B8(u8 *obj, u8 val) {
    obj[0x161] = val;
}

void func_800827C0(u8 *obj, s32 val32, s16 val16) {
    *(s32 *)&obj[0x164] = val32;
    *(s16 *)&obj[0x168] = val16;
}

s32 func_800827CC(u8 *obj) {
    return *(s32 *)&obj[0x14C];
}

u8 func_800827D8(u8 *obj) {
    return obj[0x14B];
}

void func_800827E4(u8 *obj, u8 val) {
    obj[0x14B] = val;
}

void func_800827EC(u8 *obj, u8 val) {
    obj[0x144] = val;
}

u8 func_800827F4(u8 *obj) {
    return obj[0x149];
}

void func_80082800(u8 *obj, u8 val) {
    obj[0x149] = val;
}

void func_80082808(u8 *obj, u8 val) {
    obj[0x148] = val;
}

void func_80082810(u8 *obj, u8 val) {
    obj[0x146] = val;
}

u8 GetLevelAutoScrollFlag(GameState *gameState) {
    return (GetLevelFlags(&gameState->level_context) >> 0xb) & 1;
}

void func_8008283C(void) {
}

void func_80082844(void) {
}

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", SpecialEntityDestroyCallback_2120);

void FreeSpecialEntity2120Memory(void *ptr) {
    FreeFromHeap(D_800A5954, ptr, 0, 0);
}

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", main);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", func_80082BE8);

INCLUDE_ASM("asm/nonmatchings/Game/VEHICLE/vehicle", ProcessDebugMenuInput);
