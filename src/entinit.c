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

/* ===========================================================================
 * EntityType<NNN>_..._Init: callbacks installed in g_EntityTypeCallbackTable
 * [121] @ 0x8009D5F8 and dispatched at spawn time (see RemapEntityTypesForLevel
 * for the type-remap pass that feeds this table). Standard wrapper shape:
 *   1) AllocateFromHeap(g_pBlbHeapBase, allocSize, 1, 0)  -- entity struct
 *   2) per-type Init...()  -- sets up sprite/FSM/physics, returns entity
 *   3) AddEntityToSortedRenderList(list, entity)  -- z-sorted draw list
 *   4) AddToUpdateQueue(list, entity)  -- per-frame tick queue
 * The D_8009DAxx pointer args are per-type config blobs; trailing ints toggle
 * sub-variants. Variants that omit step 4 are static (no per-frame logic);
 * variants that use AddToZOrderList instead are tiny manager entities.
 * =========================================================================== */
/* Type 000/003/004: generic collectible / pickup (config D_8009D9FC). */
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

/* Types 086/087/088: hazard-style pickup (config D_8009DA18). */
void EntityType086_087_088_InvisibleHazard_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = CreateCollectibleWithFlags(entity, spawnData, D_8009DA18, 0, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Types 106/107/108: smaller (0x114) collectible variant. */
void EntityType106_107_108_FallingPlatform_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x114, 1, 0);
    entity = InitCollectibleVariant(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Types 112/113/114: animated decor pickup (D_8009D9FC, anim flag=1). */
void EntityType112_113_114_AnimatedDecor_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = CreateCollectibleWithFlags(entity, spawnData, D_8009D9FC, 1, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Types 115/116/117: parallax-layer decor pickup (D_8009DA34, parallax=1). */
void EntityType115_116_117_ParallaxDecor_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = CreateCollectibleWithFlags(entity, spawnData, D_8009DA34, 0, 1);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* --- Path-following enemy family (alloc 0x130 PathEnemyEntity; each entry
 *     selects a different config blob + behaviour-flag triple). --- */
/* Type 010: 'Egg Beater' patrolling enemy (config D_8009DA50). */
void EntityType010_EggBeater_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x130, 1, 0);
    entity = InitPathFollowingEnemy(entity, spawnData, D_8009DA50, 0, 0, 1);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Type 026: flying patrolling enemy (config D_8009DA5C). */
void EntityType026_FlyingEnemy_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x130, 1, 0);
    entity = InitPathFollowingEnemy(entity, spawnData, D_8009DA5C, 1, 0, 1);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Type 027: flying-enemy variant (config D_8009DA68, second-arg flag set). */
void EntityType027_FlyingEnemyVariant_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x130, 1, 0);
    entity = InitPathFollowingEnemy(entity, spawnData, D_8009DA68, 0, 1, 1);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Type 076: path enemy (config D_8009DA78, last-arg=0 -> non-looping path). */
void EntityType076_PathEnemy_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x130, 1, 0);
    entity = InitPathFollowingEnemy(entity, spawnData, D_8009DA78, 1, 0, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Types 024/118: special pickup (the "special ammo" naming is a guess). */
void EntityType024_118_SpecialAmmo_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x114, 1, 0);
    entity = InitSpecialPickupEntity(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitLevelStateCollectible(void *entity, void *spawnData);

/* Type 099: collectible whose presence is gated by per-level state flags. */
void EntityType099_LevelStateCollectible_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x114, 1, 0);
    entity = InitLevelStateCollectible(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitAnimatedTimedCollectible(void *entity, void *spawnData);

/* Type 109: animated collectible with a despawn timer. */
void EntityType109_TimedCollectible_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x114, 1, 0);
    entity = InitAnimatedTimedCollectible(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Type 084: another path enemy (config D_8009DA84). */
void EntityType084_PathEnemyAlt_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x130, 1, 0);
    entity = InitPathFollowingEnemy(entity, spawnData, D_8009DA84, 0, 0, 1);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Type 096: path-following platform-style enemy (config D_8009DA90). */
void EntityType096_PathPlatformEnemy_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x130, 1, 0);
    entity = InitPathFollowingEnemy(entity, spawnData, D_8009DA90, 1, 0, 1);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitEntityWithChildSprite(void *entity, void *spawnData);

/* Type 120: parent entity that owns a secondary child sprite. */
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

/* --- InitGenericSpriteEntity family (alloc 0x124, sprite selected by hash). --- */
/* Type 001: 'boss entity' sprite (hash 0x98F8221E). Despite the name it shares
 * the generic-sprite path with the moving platforms below. */
void EntityType001_BossEntity_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    entity = InitGenericSpriteEntity(entity, spawnData, 0x98F8221E, 0, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Type 005: moving platform variant A (sprite hash 0x88783718). */
void EntityType005_MovingPlatformA_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    entity = InitGenericSpriteEntity(entity, spawnData, 0x88783718, 0, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Type 006: moving platform variant B (sprite hash 0x8818A018). */
void EntityType006_MovingPlatformB_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    entity = InitGenericSpriteEntity(entity, spawnData, 0x8818A018, 0, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Type 012: moving platform variant C (hash 0x9299C307, behaviour flag=1). */
void EntityType012_MovingPlatformC_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    entity = InitGenericSpriteEntity(entity, spawnData, 0x9299C307, 1, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Type 017: enemy-cluster sprite (hash 0x93C9A20F). */
void EntityType017_EnemyCluster_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    entity = InitGenericSpriteEntity(entity, spawnData, 0x93C9A20F, 1, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Type 018: enemy-cluster sibling (hash 0x9AB9A209). */
void EntityType018_EnemyCluster_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    entity = InitGenericSpriteEntity(entity, spawnData, 0x9AB9A209, 1, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* --- Clock platform family (alloc 0x13C; timer-driven appear/disappear). --- */
/* Type 019: clock-platform variant A. */
void EntityType019_ClockPlatformA_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x13C, 1, 0);
    entity = InitClockPlatformWithTimer(entity, spawnData, 0x93043811, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Type 020: clock-platform variant B. */
void EntityType020_ClockPlatformB_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x13C, 1, 0);
    entity = InitClockPlatformWithTimer(entity, spawnData, 0xD2801814, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Type 021: clock-platform variant C. */
void EntityType021_ClockPlatformC_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x13C, 1, 0);
    entity = InitClockPlatformWithTimer(entity, spawnData, 0x12800031, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* --- Clayball-on-path family (alloc 0x128). Despite the 'Boss*' labels these
 *     are clayball-form enemies driven along a path, NOT the bosses themselves
 *     (each just selects a different sprite hash). Names likely should be
 *     ClayballOnPath_{A,B,C}. --- */
/* Type 049: clayball-on-path sprite A (hash 0x98F8221E). */
void EntityType049_BossRelated_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x128, 1, 0);
    entity = InitClayballOnPath(entity, spawnData, 0x98F8221E, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Type 050: clayball-on-path sprite B (hash 0x88783718). 'BossMain' is misleading. */
void EntityType050_BossMain_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x128, 1, 0);
    entity = InitClayballOnPath(entity, spawnData, 0x88783718, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Type 051: clayball-on-path sprite C (hash 0x8818A018). 'BossPart' is misleading. */
void EntityType051_BossPart_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x128, 1, 0);
    entity = InitClayballOnPath(entity, spawnData, 0x8818A018, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* --- Waypoint-clayball family (alloc 0x12C, clayballs stationed at waypoints). --- */
/* Type 057: waypoint clayball A (same sprite hash as type 049). */
void EntityType057_WaypointClayballA_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x12C, 1, 0);
    entity = InitClayballAtWaypoint(entity, spawnData, 0x98F8221E, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Type 058: waypoint clayball B. */
void EntityType058_WaypointClayballB_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x12C, 1, 0);
    entity = InitClayballAtWaypoint(entity, spawnData, 0x88783718, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Type 059: waypoint clayball C. */
void EntityType059_WaypointClayballC_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x12C, 1, 0);
    entity = InitClayballAtWaypoint(entity, spawnData, 0x8818A018, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* --- Generic-sprite clayball variants. These OMIT AddToUpdateQueue --
 *     they only need to render (no per-frame tick logic). --- */
/* Type 062: clayball variant A. */
void EntityType062_ClayballVariantA_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    AddEntityToSortedRenderList(list, InitGenericSpriteEntity(entity, spawnData, 0x88A16190, 0, 0));
}

/* Type 063: clayball variant B. */
void EntityType063_ClayballVariantB_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    AddEntityToSortedRenderList(list, InitGenericSpriteEntity(entity, spawnData, 0xDCB92390, 0, 0));
}

/* Type 064: particle / passive decor sprite. */
void EntityType064_Particle_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    AddEntityToSortedRenderList(list, InitGenericSpriteEntity(entity, spawnData, 0x80B92212, 0, 0));
}

/* Type 066: clayball variant C (parallax-layer flag bit set). */
void EntityType066_ClayballVariantC_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    AddEntityToSortedRenderList(list, InitGenericSpriteEntity(entity, spawnData, 0xB69C8356, 0, 1));
}

/* Type 067: clayball variant D (parallax). */
void EntityType067_ClayballVariantD_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    AddEntityToSortedRenderList(list, InitGenericSpriteEntity(entity, spawnData, 0xB7CCE25E, 0, 1));
}

/* Type 068: clayball variant E (parallax). */
void EntityType068_ClayballVariantE_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    AddEntityToSortedRenderList(list, InitGenericSpriteEntity(entity, spawnData, 0xBEBCE258, 0, 1));
}

/* --- Switch-block clayball family (alloc 0x12C, clayball wired to a switch). --- */
/* Type 090: switch-clayball A. */
void EntityType090_SwitchClayballA_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x12C, 1, 0);
    entity = InitClayballWithSwitchBlock(entity, spawnData, 0xD89C319A, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Type 091: switch-clayball B. */
void EntityType091_SwitchClayballB_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x12C, 1, 0);
    entity = InitClayballWithSwitchBlock(entity, spawnData, 0xC89E9158, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Type 092: switch-clayball C. */
void EntityType092_SwitchClayballC_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x12C, 1, 0);
    entity = InitClayballWithSwitchBlock(entity, spawnData, 0xC48C7158, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitBonusClayballEntity(void *entity, void *spawnData, u32 hash, s32 arg3);

/* Type 093: bonus / score clayball (large 0x130 entity). */
void EntityType093_BonusClayball_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x130, 1, 0);
    entity = InitBonusClayballEntity(entity, spawnData, 0x10882010, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitGreenBulletsCollectible(void *entity, void *spawnData);

/* Type 002: green bullets ammo pickup. */
void EntityType002_GreenBullets_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = InitGreenBulletsCollectible(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitDecorEntityWithHUDIcon(void *entity, void *spawnData);

/* Type 022: decor entity that also shows a HUD icon. Name 'EnemyCluster'
 * is misleading -- inner init is InitDecorEntityWithHUDIcon. */
void EntityType022_EnemyCluster_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x128, 1, 0);
    entity = InitDecorEntityWithHUDIcon(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitClayballWithRandomColor(void *entity, void *spawnData);

/* Type 007: clayball with randomised colour tint. */
void EntityType007_Clayball_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = InitClayballWithRandomColor(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitTransparentDecorEntity(void *entity, void *spawnData);

/* Type 011: extra-life pickup (inner init is the transparent-decor variant). */
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

/* Type 009: collectible (inner init is labelled 'platform decor' -- ambiguous,
 * one of the two names is likely wrong). */
void EntityType009_Collectible_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    entity = InitPlatformDecorEntity(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitPhoenixHandCollectible(void *entity, void *spawnData);

/* Type 023: Phoenix Hand powerup pickup. */
void EntityType023_PhoenixHand_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = InitPhoenixHandCollectible(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitPhartHeadCollectible(void *entity, void *spawnData);

/* Type 025: PhartHead collectible (bonus head pickup). */
void EntityType025_PhartHeadCollectible_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    entity = InitPhartHeadCollectible(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitSingleFrameDecorEntity(void *entity, void *spawnData);

/* Type 028: GreenHeart pickup (single-frame decor sprite). */
void EntityType028_GreenHeart_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = InitSingleFrameDecorEntity(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitScaleResetCollectible(void *entity, void *spawnData);

/* Type 029: scale-reset pickup -- clears the shrink/grow powerup scaling. */
void EntityType029_ScaleReset_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = InitScaleResetCollectible(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitUniverseEnemaCollectible(void *entity, void *spawnData);

/* Type 061: 'Universe Enema' bonus pickup. */
void EntityType061_UniverseEnema_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = InitUniverseEnemaCollectible(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *Init1970IconEntity(void *entity, void *spawnData);

/* Type 069: '1970 icon' (era-themed bonus pickup). */
void EntityType069_1970Icon_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = Init1970IconEntity(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitHamsterShieldCollectible(void *entity, void *spawnData);

/* Type 070: Hamster Shield defensive powerup. */
void EntityType070_HamsterShield_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = InitHamsterShieldCollectible(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitSuperWillieCollectible(void *entity, void *spawnData);

/* Type 072: Super Willie powerup. */
void EntityType072_SuperWillie_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = InitSuperWillieCollectible(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitYellowBirdCollectible(void *entity, void *spawnData);

/* Type 075: Yellow Bird collectible. */
void EntityType075_YellowBird_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = InitYellowBirdCollectible(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitEntity_PathDecor2(void *entity, void *spawnData);

/* Type 081: path-following decor sprite (alt init path). */
void EntityType081_PathDecor2_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    entity = InitEntity_PathDecor2(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitInteractiveDecorEntity(void *entity, void *spawnData);

/* Type 083: interactive decor (responds to player touch/proximity). */
void EntityType083_InteractiveDecor_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = InitInteractiveDecorEntity(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitCheckpointEntity(void *entity, void *spawnData);

/* Type 119: respawn checkpoint marker. */
void EntityType119_Checkpoint_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x10C, 1, 0);
    entity = InitCheckpointEntity(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitCollectibleEntity_Alt(void *entity, void *spawnData);

/* --- Below: variants that skip AddToUpdateQueue (static / one-shot entities). --- */
/* Type 030: small alt collectible (no tick queue). */
void EntityType030_CollectibleAlt_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x104, 1, 0);
    AddEntityToSortedRenderList(list, InitCollectibleEntity_Alt(entity, spawnData));
}

extern u8 D_8009DA9C[];
extern u8 D_8009DAA8[];
extern void *InitScaledPlatformEntity(void *entity, void *spawnData, u8 *data);

/* Types 031/032/033: scaled-moving platforms (D_8009DA9C config). Render only. */
void EntityType031_032_033_ScaledMovingPlatform_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x110, 1, 0);
    AddEntityToSortedRenderList(list, InitScaledPlatformEntity(entity, spawnData, D_8009DA9C));
}

/* Types 034/035/036: trigger-actuated platforms (D_8009DAA8 config). */
void EntityType034_035_036_TriggerPlatform_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x110, 1, 0);
    AddEntityToSortedRenderList(list, InitScaledPlatformEntity(entity, spawnData, D_8009DAA8));
}

extern void *InitDirectionalScaledEntity(void *entity, void *spawnData, s32 arg2);

/* Types 037/038: directional scaled entities (config arg=0). */
void EntityType037_038_DirectionalScaled_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x108, 1, 0);
    AddEntityToSortedRenderList(list, InitDirectionalScaledEntity(entity, spawnData, 0));
}

extern void *InitScaledMovingEntity(void *entity, void *spawnData);

/* Type 046: scaled moving object (render only). */
void EntityType046_Object_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x108, 1, 0);
    AddEntityToSortedRenderList(list, InitScaledMovingEntity(entity, spawnData));
}

extern void *InitFloatingPlatformEntity(void *entity, void *spawnData);

/* Types 039/052: floating platform-style enemy. */
void EntityType039_052_FloatingEnemy_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x114, 1, 0);
    entity = InitFloatingPlatformEntity(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitPathFollowingEntity_Alt(void *entity, void *spawnData);

/* Type 040: path-following mechanism / contraption. */
void EntityType040_Mechanism_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = InitPathFollowingEntity_Alt(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitWalkingCollectibleEnemy(void *entity, void *spawnData);

/* Type 041: walking enemy that drops a collectible on defeat ('Clay Keeper'). */
void EntityType041_ClayKeeperEnemy_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x11C, 1, 0);
    entity = InitWalkingCollectibleEnemy(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitSnoBloEnemy(void *entity, void *spawnData);

/* Types 042-44 / 53-55 / 60: 'Sno Blo' enemies. Wrapper name reads as a
 * contiguous 42-thru-60 range, but the actual slots are non-contiguous
 * (see ENTITY_TYPE_*_SNO_BLO entries in entity_types.h) -- the dispatch
 * table just maps each of those slots to this same callback. */
void EntityType042_thru_060_SnoBlo_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x114, 1, 0);
    entity = InitSnoBloEnemy(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitSoundEmittingEnemy(void *entity, void *spawnData);

/* Types 047/048: sound-emitting platform/enemy. */
void EntityType047_048_SoundPlatform_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x114, 1, 0);
    entity = InitSoundEmittingEnemy(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitTimerBasedMenuEntity(void *entity, void *spawnData);

/* Type 080: tiny (0x2C, ~menu-base sized) timer-driven manager entity.
 * Uses AddToZOrderList (not AddToUpdateQueue) -- behaves like a UI element. */
void EntityType080_TimerMenu_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x2C, 1, 0);
    AddToZOrderList(list, InitTimerBasedMenuEntity(entity, spawnData));
}

extern void *InitBouncableClayEntity(void *entity, void *spawnData);

/* Type 045: bounceable clay block. */
void EntityType045_BounceClay_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x114, 1, 0);
    entity = InitBouncableClayEntity(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitPasswordEntity(void *entity, void *spawnData);

/* Type 065: password-input entity (render only, no per-frame tick here). */
void EntityType065_Password_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    AddEntityToSortedRenderList(list, InitPasswordEntity(entity, spawnData));
}

extern void *InitMonkeyMageBoss(void *entity, void *spawnData);

/* --- Boss inits. Each is a single, large entity allocated for one specific
 *     boss encounter (allocSize varies per-boss because each carries its own
 *     state machine workspace). --- */
/* Type 071: Monkey Mage boss (largest entity at 0x148). */
void EntityType071_Boss_MonkeyMage_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x148, 1, 0);
    entity = InitMonkeyMageBoss(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitGlennYntisBoss(void *entity, void *spawnData);

/* Type 100: Glenn Yntis boss. */
void EntityType100_Boss_GlennYntis_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x11C, 1, 0);
    entity = InitGlennYntisBoss(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitShrineyGuardBoss(void *entity, void *spawnData);

/* Type 101: Shriney Guard boss. */
void EntityType101_Boss_ShrineyGuard_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = InitShrineyGuardBoss(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitJoeHeadJoeBoss(void *entity, void *spawnData);

/* Type 102: Joe Head Joe boss. */
void EntityType102_Boss_JoeHeadJoe_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x11C, 1, 0);
    entity = InitJoeHeadJoeBoss(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitKloggBoss(void *entity, void *spawnData);

/* Type 103: Klogg boss. */
void EntityType103_Boss_Klogg_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x118, 1, 0);
    entity = InitKloggBoss(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitEnemySpawnerEntity(void *entity, void *spawnData);

/* Type 079: tiny (0x30) enemy-spawner manager entity (z-order list, not tick queue). */
void EntityType079_EnemySpawner_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x30, 1, 0);
    AddToZOrderList(list, InitEnemySpawnerEntity(entity, spawnData));
}

extern void *InitPathFollowingHazard(void *entity, void *spawnData);

/* Type 082: path-following hazard (damages on touch). */
void EntityType082_PathHazard_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x114, 1, 0);
    entity = InitPathFollowingHazard(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern void *InitTriggerZoneEntity(void *entity, void *spawnData);

/* Types 085/104/105: invisible trigger-zone entity (render-list only). */
void EntityType085_104_105_TriggerZone_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x110, 1, 0);
    AddEntityToSortedRenderList(list, InitTriggerZoneEntity(entity, spawnData));
}

extern void *InitGridLineEntity(void *entity, void *spawnData, s32 spriteId, s32 mirrorFlag, s32 flag);

/* Types 089/097/098/110/111: grid/parallax helper. Unique among inits because
 * it inspects spawnData+0x12 (entity subtype) to pick a size + flag triple
 * before allocating the small (0x80) entity header. */
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

/* Type 094: indexed sprite entity. Clamps spawnData[3] (sprite index) into
 * the valid range [0xB5, 0xDC] before init; out-of-range gets snapped to 0xB5. */
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

/* Type 095: camera-tracked sound emitter (tiny 0x28, z-order list). Either the
 * wrapper name ('SoundEmitterPanning') or the inner init name
 * ('InitCameraTrackingEntity') is the wrong half of the same entity --
 * likely it's a positional audio source that pans with the camera. */
void EntityType095_SoundEmitterPanning_Init(void *list, void *spawnData) {
    void *entity = AllocateFromHeap(g_pBlbHeapBase, 0x28, 1, 0);
    AddToZOrderList(list, InitCameraTrackingEntity(entity, spawnData));
}

/* Type 008: intentionally empty stub. The Klogg-fight catchable ball is
 * spawned/managed directly by the Klogg boss script rather than via the
 * generic spawn dispatch table -- the slot exists only so the type index
 * remains valid in the BLB entity defs. (Signature also differs: no args.) */
void EntityType008_KloggCatchableBall_Init(void) {
}

/* Pre-spawn pass over the level's EntityDefinition[] (GameState+0x28): rewrites
 * each .entityType field per the current level so that BLB's logical type ids
 * map to the right index in g_EntityTypeCallbackTable. Implemented as a big
 * jump table (jtbl_80012158) keyed on the raw BLB type byte; same logical
 * type can resolve to a different concrete callback in e.g. KLOG vs SOAR. */
INCLUDE_ASM("asm/nonmatchings/entinit", RemapEntityTypesForLevel);

extern void *D_800A595C;
extern u8 D_8009DAB4[];
extern void LoadSpriteHashArrayToVRAM(void *heap, u8 *data);
extern void LoadSpriteFramesToVRAM(void *heap, u32 hash);

/* Uploads the level's sprite-hash array (D_8009DAB4) into VRAM, plus one
 * extra sprite (hash 0x168254B5) when the current level has flag bit 2 set. */
void LoadLevelSpriteAssets(void *arg) {
    LoadSpriteHashArrayToVRAM(D_800A595C, D_8009DAB4);
    if (GetLevelFlags((u8 *)arg + 0x84) & 4) {
        LoadSpriteFramesToVRAM(D_800A595C, 0x168254B5);
    }
}

extern u8 D_8009D228[][16];

/* Slope-tile height lookup: returns the per-subpixel ground height for the
 * given tile type from the 16-entry-per-row D_8009D228 table. */
u8 GetSlopeHeightAtSubpixel(void *unused, u32 tileType, u32 subpixel) {
    return D_8009D228[tileType & 0xFF][subpixel & 0xF];
}

/* Sets the player's current speed (entity+0x140) and propagates it to the
 * attached vehicle (entity+0x2C -> +0x100) and HUD speed slot (entity+0x14C
 * -> +0x1C). The chain of GetLevelFlags() calls is a side-effect dispatch
 * (each call also runs callback hooks on the level-flags object) that gates
 * whether the vehicle copy actually happens for special level modes. */
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

extern void *InitAlternateEntity(void *entity, void *spawnData);

/* Alternate-spawn iterator. Walks alternate_entity_data[] (gameState+0x164,
 * 64-byte stride, alternate_entity_count entries), tests each slot's AABB
 * against the player's scaled visible bounds (+/-0x10 margin), and for any
 * in-range slot whose +0x3C 'spawned' flag is clear: allocates a 0x10C-byte
 * entity, runs InitAlternateEntity, links it into the sorted render list,
 * and marks the slot consumed. Drives the streamed spawn flow used by
 * auto-scroller / boss arenas. */
INCLUDE_ASM("asm/nonmatchings/entinit", SpawnEntitiesAlternateSystem);

/* Resets the per-slot 'already spawned' flag (+0x3C) on every entry in the
 * alternate-spawn table so SpawnEntitiesAlternateSystem can re-spawn them
 * (used on checkpoint restore / level restart). */
void ClearAlternateEntitySpawnFlags(void *state) {
    s16 i;
    u8 *ptr = *(u8 **)((u8 *)state + 0x164);
    for (i = 0; i < *(u16 *)((u8 *)state + 0x168); i++) {
        *(s32 *)(ptr + 0x3C) = 0;
        ptr += 0x40;
    }
}

/* Links a render primitive node into one of 256 depth buckets stored at
 * gameState+0x16C. Bucket index = (node->depth >> 1), clamped to 255.
 * Each bucket is a singly-linked list (node->next at offset 0); newest node
 * inserted at the head. Drained later by FlushDepthBuckets. */
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

/* Walks the 256 depth buckets (entity+0x16C) front-to-back, calling SetPolyGT4
 * on each primitive and inserting it into the active GPU ordering table
 * (heap+0xA084 -> +0x70). Clears each bucket head as it goes; this is the
 * per-frame drain that turns the bucket queue into GPU draw commands. */
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

/* Zeroes all 256 entries of the depth-bucket head array (state+0x16C).
 * Called at frame start to reset the bucket queue before any AddToDepthBucket
 * calls accumulate the next frame's primitives. */
void ClearEntityPoolArray(void *state) {
    s16 i;
    for (i = 0; i < 256; i++) {
        ((s32 *)*(s32 *)((u8 *)state + 0x16C))[i] = 0;
    }
}

/* Returns 1 if this is a normal side-scrolling platformer level, 0 if it's
 * any special mode (boss arena, glide/SOAR, auto-scroll, FINN vehicle, etc.).
 * The cascade of GetLevelFlags checks short-circuits to 0 on any special-mode
 * bit; result is the inverted bit-2 ('use parallax / standard layers') test. */
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
