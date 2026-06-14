#include "common.h"
#include "Game/game_state.h"
#include "globals.h"

extern s32 GetLevelFlags();
extern void FreeFromHeap(void *heap, void *ptr, s32 mode, s32 flags);
extern void PlaySoundEffect(u32 soundId, s32 volume, s32 param);
extern void HidePauseMenuHUD(s32 handle);
extern u8 D_80012120[];


extern u8 D_8009D9FC[];
extern void *CreateCollectibleWithFlags(void *entity, void *spawnData, u8 *flags, s32 arg3, s32 arg4);

void EntityType000_003_004_PickupVariant_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = CreateCollectibleWithFlags(entity, spawnData, D_8009D9FC, 0, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern u8 D_8009DA18[];
extern u8 D_8009DA34[];
extern u8 D_8009DA50[];
extern u8 D_8009DA5C[];
extern u8 D_8009DA68[];
extern u8 D_8009DA78[];
extern u8 D_8009DA84[];
extern u8 D_8009DA90[];
extern void *InitCollectibleVariant(void *entity, void *spawnData);
extern void *InitPathFollowingEnemy(void *entity, void *spawnData, u8 *data, s32 arg3, s32 arg4, s32 arg5);
extern void *InitSpecialPickupEntity(void *entity, void *spawnData);

void EntityType086_087_088_InvisibleHazard_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = CreateCollectibleWithFlags(entity, spawnData, D_8009DA18, 0, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType106_107_108_FallingPlatform_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x114, 1, 0);
    entity = InitCollectibleVariant(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType112_113_114_AnimatedDecor_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = CreateCollectibleWithFlags(entity, spawnData, D_8009D9FC, 1, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType115_116_117_ParallaxDecor_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = CreateCollectibleWithFlags(entity, spawnData, D_8009DA34, 0, 1);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType010_EggBeater_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x130, 1, 0);
    entity = InitPathFollowingEnemy(entity, spawnData, D_8009DA50, 0, 0, 1);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType026_FlyingEnemy_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x130, 1, 0);
    entity = InitPathFollowingEnemy(entity, spawnData, D_8009DA5C, 1, 0, 1);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType027_FlyingEnemyVariant_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x130, 1, 0);
    entity = InitPathFollowingEnemy(entity, spawnData, D_8009DA68, 0, 1, 1);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType076_PathEnemy_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x130, 1, 0);
    entity = InitPathFollowingEnemy(entity, spawnData, D_8009DA78, 1, 0, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType024_118_SpecialAmmo_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x114, 1, 0);
    entity = InitSpecialPickupEntity(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitLevelStateCollectible(void *entity, void *spawnData);

void EntityType099_LevelStateCollectible_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x114, 1, 0);
    entity = InitLevelStateCollectible(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitAnimatedTimedCollectible(void *entity, void *spawnData);

void EntityType109_TimedCollectible_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x114, 1, 0);
    entity = InitAnimatedTimedCollectible(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType084_PathEnemyAlt_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x130, 1, 0);
    entity = InitPathFollowingEnemy(entity, spawnData, D_8009DA84, 0, 0, 1);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType096_PathPlatformEnemy_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x130, 1, 0);
    entity = InitPathFollowingEnemy(entity, spawnData, D_8009DA90, 1, 0, 1);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitEntityWithChildSprite(void *entity, void *spawnData);

void EntityType120_ChildSprite_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x11C, 1, 0);
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
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    entity = InitGenericSpriteEntity(entity, spawnData, 0x98F8221E, 0, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType005_MovingPlatformA_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    entity = InitGenericSpriteEntity(entity, spawnData, 0x88783718, 0, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType006_MovingPlatformB_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    entity = InitGenericSpriteEntity(entity, spawnData, 0x8818A018, 0, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType012_MovingPlatformC_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    entity = InitGenericSpriteEntity(entity, spawnData, 0x9299C307, 1, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType017_EnemyCluster_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    entity = InitGenericSpriteEntity(entity, spawnData, 0x93C9A20F, 1, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType018_EnemyCluster_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    entity = InitGenericSpriteEntity(entity, spawnData, 0x9AB9A209, 1, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType019_ClockPlatformA_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x13C, 1, 0);
    entity = InitClockPlatformWithTimer(entity, spawnData, 0x93043811, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType020_ClockPlatformB_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x13C, 1, 0);
    entity = InitClockPlatformWithTimer(entity, spawnData, 0xD2801814, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType021_ClockPlatformC_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x13C, 1, 0);
    entity = InitClockPlatformWithTimer(entity, spawnData, 0x12800031, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType049_BossRelated_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x128, 1, 0);
    entity = InitClayballOnPath(entity, spawnData, 0x98F8221E, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType050_BossMain_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x128, 1, 0);
    entity = InitClayballOnPath(entity, spawnData, 0x88783718, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType051_BossPart_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x128, 1, 0);
    entity = InitClayballOnPath(entity, spawnData, 0x8818A018, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType057_WaypointClayballA_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x12C, 1, 0);
    entity = InitClayballAtWaypoint(entity, spawnData, 0x98F8221E, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType058_WaypointClayballB_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x12C, 1, 0);
    entity = InitClayballAtWaypoint(entity, spawnData, 0x88783718, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType059_WaypointClayballC_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x12C, 1, 0);
    entity = InitClayballAtWaypoint(entity, spawnData, 0x8818A018, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType062_ClayballVariantA_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    AddEntityToSortedRenderList(list, InitGenericSpriteEntity(entity, spawnData, 0x88A16190, 0, 0));
}

void EntityType063_ClayballVariantB_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    AddEntityToSortedRenderList(list, InitGenericSpriteEntity(entity, spawnData, 0xDCB92390, 0, 0));
}

void EntityType064_Particle_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    AddEntityToSortedRenderList(list, InitGenericSpriteEntity(entity, spawnData, 0x80B92212, 0, 0));
}

void EntityType066_ClayballVariantC_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    AddEntityToSortedRenderList(list, InitGenericSpriteEntity(entity, spawnData, 0xB69C8356, 0, 1));
}

void EntityType067_ClayballVariantD_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    AddEntityToSortedRenderList(list, InitGenericSpriteEntity(entity, spawnData, 0xB7CCE25E, 0, 1));
}

void EntityType068_ClayballVariantE_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    AddEntityToSortedRenderList(list, InitGenericSpriteEntity(entity, spawnData, 0xBEBCE258, 0, 1));
}

void EntityType090_SwitchClayballA_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x12C, 1, 0);
    entity = InitClayballWithSwitchBlock(entity, spawnData, 0xD89C319A, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType091_SwitchClayballB_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x12C, 1, 0);
    entity = InitClayballWithSwitchBlock(entity, spawnData, 0xC89E9158, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

void EntityType092_SwitchClayballC_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x12C, 1, 0);
    entity = InitClayballWithSwitchBlock(entity, spawnData, 0xC48C7158, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitBonusClayballEntity(void *entity, void *spawnData, u32 hash, s32 arg3);

void EntityType093_BonusClayball_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x130, 1, 0);
    entity = InitBonusClayballEntity(entity, spawnData, 0x10882010, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitGreenBulletsCollectible(void *entity, void *spawnData);

void EntityType002_GreenBullets_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = InitGreenBulletsCollectible(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitDecorEntityWithHUDIcon(void *entity, void *spawnData);

void EntityType022_EnemyCluster_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x128, 1, 0);
    entity = InitDecorEntityWithHUDIcon(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitClayballWithRandomColor(void *entity, void *spawnData);

void EntityType007_Clayball_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = InitClayballWithRandomColor(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitTransparentDecorEntity(void *entity, void *spawnData);

void EntityType011_ExtraLife_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = InitTransparentDecorEntity(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *AllocateFromHeap(void *heap, s32 size, s32 align, s32 flags);
extern void *InitPlatformDecorEntity(void *entity, void *spawnData);
extern void AddEntityToSortedRenderList(void *list, void *entity);
extern void AddToUpdateQueue(void *list, void *entity);

void EntityType009_Collectible_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    entity = InitPlatformDecorEntity(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitPhoenixHandCollectible(void *entity, void *spawnData);

void EntityType023_PhoenixHand_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = InitPhoenixHandCollectible(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitPhartHeadCollectible(void *entity, void *spawnData);

void EntityType025_PhartHeadCollectible_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    entity = InitPhartHeadCollectible(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitSingleFrameDecorEntity(void *entity, void *spawnData);

void EntityType028_GreenHeart_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = InitSingleFrameDecorEntity(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitScaleResetCollectible(void *entity, void *spawnData);

void EntityType029_ScaleReset_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = InitScaleResetCollectible(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitUniverseEnemaCollectible(void *entity, void *spawnData);

void EntityType061_UniverseEnema_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = InitUniverseEnemaCollectible(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *Init1970IconEntity(void *entity, void *spawnData);

void EntityType069_1970Icon_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = Init1970IconEntity(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitHamsterShieldCollectible(void *entity, void *spawnData);

void EntityType070_HamsterShield_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = InitHamsterShieldCollectible(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitSuperWillieCollectible(void *entity, void *spawnData);

void EntityType072_SuperWillie_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = InitSuperWillieCollectible(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitYellowBirdCollectible(void *entity, void *spawnData);

void EntityType075_YellowBird_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = InitYellowBirdCollectible(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitEntity_PathDecor2(void *entity, void *spawnData);

void EntityType081_PathDecor2_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    entity = InitEntity_PathDecor2(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitInteractiveDecorEntity(void *entity, void *spawnData);

void EntityType083_InteractiveDecor_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = InitInteractiveDecorEntity(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitCheckpointEntity(void *entity, void *spawnData);

void EntityType119_Checkpoint_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x10C, 1, 0);
    entity = InitCheckpointEntity(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitCollectibleEntity_Alt(void *entity, void *spawnData);

void EntityType030_CollectibleAlt_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x104, 1, 0);
    AddEntityToSortedRenderList(list, InitCollectibleEntity_Alt(entity, spawnData));
}

extern u8 D_8009DA9C[];
extern u8 D_8009DAA8[];
extern void *InitScaledPlatformEntity(void *entity, void *spawnData, u8 *data);

void EntityType031_032_033_ScaledMovingPlatform_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x110, 1, 0);
    AddEntityToSortedRenderList(list, InitScaledPlatformEntity(entity, spawnData, D_8009DA9C));
}

void EntityType034_035_036_TriggerPlatform_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x110, 1, 0);
    AddEntityToSortedRenderList(list, InitScaledPlatformEntity(entity, spawnData, D_8009DAA8));
}

extern void *InitDirectionalScaledEntity(void *entity, void *spawnData, s32 arg2);

void EntityType037_038_DirectionalScaled_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x108, 1, 0);
    AddEntityToSortedRenderList(list, InitDirectionalScaledEntity(entity, spawnData, 0));
}

extern void *InitScaledMovingEntity(void *entity, void *spawnData);

void EntityType046_Object_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x108, 1, 0);
    AddEntityToSortedRenderList(list, InitScaledMovingEntity(entity, spawnData));
}

extern void *InitFloatingPlatformEntity(void *entity, void *spawnData);

void EntityType039_052_FloatingEnemy_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x114, 1, 0);
    entity = InitFloatingPlatformEntity(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitPathFollowingEntity_Alt(void *entity, void *spawnData);

void EntityType040_Mechanism_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = InitPathFollowingEntity_Alt(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitWalkingCollectibleEnemy(void *entity, void *spawnData);

void EntityType041_ClayKeeperEnemy_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x11C, 1, 0);
    entity = InitWalkingCollectibleEnemy(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitSnoBloEnemy(void *entity, void *spawnData);

void EntityType042_thru_060_SnoBlo_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x114, 1, 0);
    entity = InitSnoBloEnemy(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitSoundEmittingEnemy(void *entity, void *spawnData);

void EntityType047_048_SoundPlatform_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x114, 1, 0);
    entity = InitSoundEmittingEnemy(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitTimerBasedMenuEntity(void *entity, void *spawnData);

void EntityType080_TimerMenu_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x2C, 1, 0);
    AddToZOrderList(list, InitTimerBasedMenuEntity(entity, spawnData));
}

extern void *InitBouncableClayEntity(void *entity, void *spawnData);

void EntityType045_BounceClay_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x114, 1, 0);
    entity = InitBouncableClayEntity(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitPasswordEntity(void *entity, void *spawnData);

void EntityType065_Password_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    AddEntityToSortedRenderList(list, InitPasswordEntity(entity, spawnData));
}

extern void *InitMonkeyMageBoss(void *entity, void *spawnData);

void EntityType071_Boss_MonkeyMage_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x148, 1, 0);
    entity = InitMonkeyMageBoss(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitGlennYntisBoss(void *entity, void *spawnData);

void EntityType100_Boss_GlennYntis_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x11C, 1, 0);
    entity = InitGlennYntisBoss(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitShrineyGuardBoss(void *entity, void *spawnData);

void EntityType101_Boss_ShrineyGuard_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = InitShrineyGuardBoss(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitJoeHeadJoeBoss(void *entity, void *spawnData);

void EntityType102_Boss_JoeHeadJoe_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x11C, 1, 0);
    entity = InitJoeHeadJoeBoss(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitKloggBoss(void *entity, void *spawnData);

void EntityType103_Boss_Klogg_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x118, 1, 0);
    entity = InitKloggBoss(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitEnemySpawnerEntity(void *entity, void *spawnData);

void EntityType079_EnemySpawner_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x30, 1, 0);
    AddToZOrderList(list, InitEnemySpawnerEntity(entity, spawnData));
}

extern void *InitPathFollowingHazard(void *entity, void *spawnData);

void EntityType082_PathHazard_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x114, 1, 0);
    entity = InitPathFollowingHazard(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitTriggerZoneEntity(void *entity, void *spawnData);

void EntityType085_104_105_TriggerZone_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x110, 1, 0);
    AddEntityToSortedRenderList(list, InitTriggerZoneEntity(entity, spawnData));
}

extern void *InitGridLineEntity(void *entity, void *spawnData, s32 spriteId, s32 mirrorFlag, s32 flag);

void EntityType089_097_098_110_111_GridHelper_Init(void *list, void *spawnData) {
    s32 size = 0x4B0;
    s32 useMirror = 0;
    u16 type = *(u16 *)((u8 *)spawnData + 0x12);
    s32 flag = 0x10000;
    void *entity;

    if (type == 0x62) {
        useMirror = 1;
    } else if (type == 0x61) {
        size = 0x384;
    } else if (type == 0x6E) {
        size = 0x578;
        flag = 0x20000;
    } else if (type == 0x6F) {
        size = 0x640;
        flag = 0x40000;
    }

    entity = AllocateFromHeap(g_pBlbHeapBase, 0x80, 1, 0);
    entity = InitGridLineEntity(entity, spawnData, size, useMirror, flag);
    AddEntityToSortedRenderList(list, entity);
}

extern void *InitIndexedSpriteEntity(void *entity, void *spawnData);

void EntityType094_IndexedSprite_Init(void *list, void *spawnData) {
    s32 *ptr = (s32 *)spawnData;
    void *entity;
    if ((u32)(ptr[3] - 0xB5) >= 0x28) {
        ptr[3] = 0xB5;
    }
    entity = AllocateFromHeap(g_pBlbHeapBase, 0x104, 1, 0);
    AddEntityToSortedRenderList(list, InitIndexedSpriteEntity(entity, spawnData));
}

extern void *InitCameraTrackingEntity(void *entity, void *spawnData);

void EntityType095_SoundEmitterPanning_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x28, 1, 0);
    AddToZOrderList(list, InitCameraTrackingEntity(entity, spawnData));
}

void EntityType008_KloggCatchableBall_Init(void) {
}

INCLUDE_ASM("asm/nonmatchings/entinit", RemapEntityTypesForLevel);

extern void *D_800A595C;
extern u8 D_8009DAB4[];
extern void LoadSpriteHashArrayToVRAM(void *heap, u8 *data);
extern void LoadSpriteFramesToVRAM(void *heap, u32 hash);

void LoadLevelSpriteAssets(void *arg) {
    LoadSpriteHashArrayToVRAM(D_800A595C, D_8009DAB4);
    if (GetLevelFlags((u8 *)arg + 0x84) & 4) {
        LoadSpriteFramesToVRAM(D_800A595C, 0x168254B5);
    }
}

extern u8 D_8009D228[][16];

u8 GetSlopeHeightAtSubpixel(void *unused, u32 tileType, u32 subpixel) {
    return D_8009D228[tileType & 0xFF][subpixel & 0xF];
}

void SetPlayerVehicleSpeed(void *entity, s32 speed) {
    void *other;
    u8 *ptr;
    *(s32 *)((u8 *)entity + 0x140) = speed;
    if (*(void **)((u8 *)entity + 0x2C)) {
        ptr = (u8 *)entity + 0x84;
        if (GetLevelFlags(ptr) & 0x400) goto store;
        if (GetLevelFlags(ptr) & 0x200) goto store;
        if (GetLevelFlags(ptr) & 0x2000) goto store;
        if (GetLevelFlags(ptr) & 0x100) goto store;
        if (GetLevelFlags(ptr) & 0x10) goto store;
        GetLevelFlags(ptr);
store:
        *(s32 *)((u8 *)(*(void **)((u8 *)entity + 0x2C)) + 0x100) = *(s32 *)((u8 *)entity + 0x140);
    }
    other = *(void **)((u8 *)entity + 0x14C);
    if (other) {
        *(s32 *)((u8 *)other + 0x1C) = *(s32 *)((u8 *)entity + 0x140);
    }
}

INCLUDE_ASM("asm/nonmatchings/entinit", SpawnEntitiesAlternateSystem);

void ClearAlternateEntitySpawnFlags(void *state) {
    s16 i;
    u8 *ptr = *(u8 **)((u8 *)state + 0x164);
    for (i = 0; i < *(u16 *)((u8 *)state + 0x168); i++) {
        *(s32 *)(ptr + 0x3C) = 0;
        ptr += 0x40;
    }
}

void AddToDepthBucket(void *gameState, void *node) {
    s32 bucket_idx;
    void **buckets;
    u16 bucket;
    u32 raw = (u32)(*(u16 *)((u8 *)node + 0x26)) >> 1;
    bucket = raw;
    if ((s32)raw >= 0x100) {
        bucket = 0xFF;
    }
    buckets = *(void ***)((u8 *)gameState + 0x16C);
    if (buckets[bucket] != NULL) {
        *(void **)node = buckets[bucket];
    } else {
        *(void **)node = NULL;
    }
    bucket_idx = bucket;
    (*(void ***)((u8 *)gameState + 0x16C))[bucket_idx] = node;
}

extern void SetPolyGT4(void *prim);
extern void AddPrim(void *ot, void *prim);

void FlushDepthBuckets(void *entity) {
    s16 i;
    void **buckets;
    void *node;
    void *next;
    void *ot;

    i = 0;
    buckets = *(void ***)((u8 *)entity + 0x16C);
loop_start:
    if ((s16)i >= 256) goto loop_end;
    {
        void *head = *buckets;
        if (head == NULL) goto advance;
        node = head;
    }
inner:
    if (node == NULL) {
        *buckets = NULL;
        goto advance;
    }
    next = *(void **)node;
    SetPolyGT4(node);
    ot = *(void **)((u8 *)*(void **)((u8 *)g_pBlbHeapBase + 0xA084) + 0x70);
    AddPrim(ot, node);
    node = next;
    goto inner;
advance:
    buckets++;
    i++;
    goto loop_start;
loop_end:
    return;
}

void ClearEntityPoolArray(void *state) {
    s16 i;
    for (i = 0; i < 256; i++) {
        ((s32 *)*(s32 *)((u8 *)state + 0x16C))[i] = 0;
    }
}

s32 IsNormalPlatformLevel(void *arg) {
    s32 result = 0;
    u8 *ptr = (u8 *)arg + 0x84;
    if (GetLevelFlags(ptr) & 0x400) goto end;
    if (GetLevelFlags(ptr) & 0x200) goto end;
    if (GetLevelFlags(ptr) & 0x2000) goto end;
    if (GetLevelFlags(ptr) & 0x100) goto end;
    if (GetLevelFlags(ptr) & 0x10) goto end;
    {
        u32 tmp = GetLevelFlags(ptr) & 0x4;
        result = tmp < 1u;
    }
end:
    return result;
}
