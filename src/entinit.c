#include "common.h"
#include "functions.h"
extern s32 GetLevelFlags();
#include "Game/game_state.h"
#include "Game/entinit_entities.h"
#include "globals.h"

extern u8 ENTITY_TYPE_REMAP_TABLE[] asm("D_80012120");

extern u8 COLLECTIBLE_CONFIG_DEFAULT[] asm("D_8009D9FC");
typedef Entity EntityList;
typedef u8 EntitySpawnData;

extern Entity *CreateCollectibleWithFlags(Entity *entity, EntitySpawnData *spawnData, u8 *flags, s32 arg3, s32 arg4);

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
/* Type 000/003/004: generic collectible / pickup (config COLLECTIBLE_CONFIG_DEFAULT). */
void EntityType000_003_004_PickupVariant_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = CreateCollectibleWithFlags(entity, spawnData, COLLECTIBLE_CONFIG_DEFAULT, 0, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern u8 COLLECTIBLE_CONFIG_HAZARD[] asm("D_8009DA18");
extern u8 COLLECTIBLE_CONFIG_PARALLAX[] asm("D_8009DA34");
extern u8 PATH_ENEMY_CONFIG_EGG_BEATER[] asm("D_8009DA50");
extern u8 PATH_ENEMY_CONFIG_FLYING[] asm("D_8009DA5C");
extern u8 PATH_ENEMY_CONFIG_FLYING_VARIANT[] asm("D_8009DA68");
extern u8 PATH_ENEMY_CONFIG_STANDARD[] asm("D_8009DA78");
extern u8 PATH_ENEMY_CONFIG_ALT[] asm("D_8009DA84");
extern u8 PATH_ENEMY_CONFIG_PLATFORM[] asm("D_8009DA90");
extern Entity *InitCollectibleVariant(Entity *entity, EntitySpawnData *spawnData);
extern Entity *InitPathFollowingEnemy(Entity *entity, EntitySpawnData *spawnData, u8 *data, s32 arg3, s32 arg4, s32 arg5);
extern Entity *InitSpecialPickupEntity(Entity *entity, EntitySpawnData *spawnData);

/* Types 086/087/088: hazard-style pickup (config COLLECTIBLE_CONFIG_HAZARD). */
void EntityType086_087_088_InvisibleHazard_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = CreateCollectibleWithFlags(entity, spawnData, COLLECTIBLE_CONFIG_HAZARD, 0, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Types 106/107/108: smaller (0x114) collectible variant. */
void EntityType106_107_108_FallingPlatform_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x114, 1, 0);
    entity = InitCollectibleVariant(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Types 112/113/114: animated decor pickup (COLLECTIBLE_CONFIG_DEFAULT, anim flag=1). */
void EntityType112_113_114_AnimatedDecor_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = CreateCollectibleWithFlags(entity, spawnData, COLLECTIBLE_CONFIG_DEFAULT, 1, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Types 115/116/117: parallax-layer decor pickup (COLLECTIBLE_CONFIG_PARALLAX, parallax=1). */
void EntityType115_116_117_ParallaxDecor_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = CreateCollectibleWithFlags(entity, spawnData, COLLECTIBLE_CONFIG_PARALLAX, 0, 1);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* --- Path-following enemy family (alloc 0x130 PathEnemyEntity; each entry
 *     selects a different config blob + behaviour-flag triple). --- */
/* Type 010: 'Egg Beater' patrolling enemy (config PATH_ENEMY_CONFIG_EGG_BEATER). */
void EntityType010_EggBeater_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x130, 1, 0);
    entity = InitPathFollowingEnemy(entity, spawnData, PATH_ENEMY_CONFIG_EGG_BEATER, 0, 0, 1);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Type 026: flying patrolling enemy (config PATH_ENEMY_CONFIG_FLYING). */
void EntityType026_FlyingEnemy_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x130, 1, 0);
    entity = InitPathFollowingEnemy(entity, spawnData, PATH_ENEMY_CONFIG_FLYING, 1, 0, 1);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Type 027: flying-enemy variant (config PATH_ENEMY_CONFIG_FLYING_VARIANT, second-arg flag set). */
void EntityType027_FlyingEnemyVariant_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x130, 1, 0);
    entity = InitPathFollowingEnemy(entity, spawnData, PATH_ENEMY_CONFIG_FLYING_VARIANT, 0, 1, 1);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Type 076: path enemy (config PATH_ENEMY_CONFIG_STANDARD, last-arg=0 -> non-looping path). */
void EntityType076_PathEnemy_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x130, 1, 0);
    entity = InitPathFollowingEnemy(entity, spawnData, PATH_ENEMY_CONFIG_STANDARD, 1, 0, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Types 024/118: special pickup (the "special ammo" naming is a guess). */
void EntityType024_118_SpecialAmmo_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x114, 1, 0);
    entity = InitSpecialPickupEntity(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern Entity *InitLevelStateCollectible(Entity *entity, EntitySpawnData *spawnData);

/* Type 099: collectible whose presence is gated by per-level state flags. */
void EntityType099_LevelStateCollectible_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x114, 1, 0);
    entity = InitLevelStateCollectible(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern Entity *InitAnimatedTimedCollectible(Entity *entity, EntitySpawnData *spawnData);

/* Type 109: animated collectible with a despawn timer. */
void EntityType109_TimedCollectible_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x114, 1, 0);
    entity = InitAnimatedTimedCollectible(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Type 084: another path enemy (config PATH_ENEMY_CONFIG_ALT). */
void EntityType084_PathEnemyAlt_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x130, 1, 0);
    entity = InitPathFollowingEnemy(entity, spawnData, PATH_ENEMY_CONFIG_ALT, 0, 0, 1);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Type 096: path-following platform-style enemy (config PATH_ENEMY_CONFIG_PLATFORM). */
void EntityType096_PathPlatformEnemy_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x130, 1, 0);
    entity = InitPathFollowingEnemy(entity, spawnData, PATH_ENEMY_CONFIG_PLATFORM, 1, 0, 1);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern Entity *InitEntityWithChildSprite(Entity *entity, EntitySpawnData *spawnData);

/* Type 120: parent entity that owns a secondary child sprite. */
void EntityType120_ChildSprite_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x11C, 1, 0);
    entity = InitEntityWithChildSprite(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern Entity *InitGenericSpriteEntity(Entity *entity, EntitySpawnData *spawnData, u32 hash, s32 arg3, s32 arg4);
extern Entity *InitClockPlatformWithTimer(Entity *entity, EntitySpawnData *spawnData, u32 hash, s32 arg3);
extern Entity *InitClayballOnPath(Entity *entity, EntitySpawnData *spawnData, u32 hash, s32 arg3);
extern Entity *InitClayballAtWaypoint(Entity *entity, EntitySpawnData *spawnData, u32 hash, s32 arg3);
extern Entity *InitClayballWithSwitchBlock(Entity *entity, EntitySpawnData *spawnData, u32 hash, s32 arg3);

/* --- InitGenericSpriteEntity family (alloc 0x124, sprite selected by hash). --- */
/* Type 001: 'boss entity' sprite (hash SPR_PLATFORM_BALL_A). Despite the name it shares
 * the generic-sprite path with the moving platforms below. */
void EntityType001_BossEntity_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    entity = InitGenericSpriteEntity(entity, spawnData, SPR_PLATFORM_BALL_A, 0, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Type 005: moving platform variant A (sprite hash SPR_PLATFORM_BALL_B). */
void EntityType005_MovingPlatformA_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    entity = InitGenericSpriteEntity(entity, spawnData, SPR_PLATFORM_BALL_B, 0, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Type 006: moving platform variant B (sprite hash SPR_PLATFORM_BALL_C). */
void EntityType006_MovingPlatformB_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    entity = InitGenericSpriteEntity(entity, spawnData, SPR_PLATFORM_BALL_C, 0, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Type 012: moving platform variant C (hash SPR_PLATFORM_VARIANT_D, behaviour flag=1). */
void EntityType012_MovingPlatformC_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    entity = InitGenericSpriteEntity(entity, spawnData, SPR_PLATFORM_VARIANT_D, 1, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Type 017: enemy-cluster sprite (hash SPR_ENEMY_CLUSTER_A). */
void EntityType017_EnemyCluster_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    entity = InitGenericSpriteEntity(entity, spawnData, SPR_ENEMY_CLUSTER_A, 1, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Type 018: enemy-cluster sibling (hash SPR_ENEMY_CLUSTER_B). */
void EntityType018_EnemyCluster_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    entity = InitGenericSpriteEntity(entity, spawnData, SPR_ENEMY_CLUSTER_B, 1, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* --- Clock platform family (alloc 0x13C; timer-driven appear/disappear). --- */
/* Type 019: clock-platform variant A. */
void EntityType019_ClockPlatformA_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x13C, 1, 0);
    entity = InitClockPlatformWithTimer(entity, spawnData, SPR_CLOCK_PLATFORM_A, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Type 020: clock-platform variant B. */
void EntityType020_ClockPlatformB_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x13C, 1, 0);
    entity = InitClockPlatformWithTimer(entity, spawnData, SPR_CLOCK_PLATFORM_B, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Type 021: clock-platform variant C. */
void EntityType021_ClockPlatformC_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x13C, 1, 0);
    entity = InitClockPlatformWithTimer(entity, spawnData, SPR_CLOCK_PLATFORM_C, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* --- Clayball-on-path family (alloc 0x128). These are clayball-form enemies
 *     driven along a path (each just selects a different sprite hash), NOT the
 *     bosses themselves -- earlier 'Boss*' labels were misleading. --- */
/* Type 049: clayball-on-path sprite A (hash SPR_PLATFORM_BALL_A). */
void EntityType049_ClayballOnPathA_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x128, 1, 0);
    entity = InitClayballOnPath(entity, spawnData, SPR_PLATFORM_BALL_A, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Type 050: clayball-on-path sprite B (hash SPR_PLATFORM_BALL_B). */
void EntityType050_ClayballOnPathB_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x128, 1, 0);
    entity = InitClayballOnPath(entity, spawnData, SPR_PLATFORM_BALL_B, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Type 051: clayball-on-path sprite C (hash SPR_PLATFORM_BALL_C). */
void EntityType051_ClayballOnPathC_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x128, 1, 0);
    entity = InitClayballOnPath(entity, spawnData, SPR_PLATFORM_BALL_C, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* --- Waypoint-clayball family (alloc 0x12C, clayballs stationed at waypoints). --- */
/* Type 057: waypoint clayball A (same sprite hash as type 049). */
void EntityType057_WaypointClayballA_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x12C, 1, 0);
    entity = InitClayballAtWaypoint(entity, spawnData, SPR_PLATFORM_BALL_A, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Type 058: waypoint clayball B. */
void EntityType058_WaypointClayballB_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x12C, 1, 0);
    entity = InitClayballAtWaypoint(entity, spawnData, SPR_PLATFORM_BALL_B, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Type 059: waypoint clayball C. */
void EntityType059_WaypointClayballC_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x12C, 1, 0);
    entity = InitClayballAtWaypoint(entity, spawnData, SPR_PLATFORM_BALL_C, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* --- Generic-sprite clayball variants. These OMIT AddToUpdateQueue --
 *     they only need to render (no per-frame tick logic). --- */
/* Type 062: clayball variant A. */
void EntityType062_ClayballVariantA_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    AddEntityToSortedRenderList(list, InitGenericSpriteEntity(entity, spawnData, SPR_CLAYBALL_DECOR_A, 0, 0));
}

/* Type 063: clayball variant B. */
void EntityType063_ClayballVariantB_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    AddEntityToSortedRenderList(list, InitGenericSpriteEntity(entity, spawnData, SPR_CLAYBALL_DECOR_B, 0, 0));
}

/* Type 064: particle / passive decor sprite. */
void EntityType064_Particle_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    AddEntityToSortedRenderList(list, InitGenericSpriteEntity(entity, spawnData, SPR_PARTICLE_DECOR, 0, 0));
}

/* Type 066: clayball variant C (parallax-layer flag bit set). */
void EntityType066_ClayballVariantC_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    AddEntityToSortedRenderList(list, InitGenericSpriteEntity(entity, spawnData, SPR_CLAYBALL_PARALLAX_A, 0, 1));
}

/* Type 067: clayball variant D (parallax). */
void EntityType067_ClayballVariantD_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    AddEntityToSortedRenderList(list, InitGenericSpriteEntity(entity, spawnData, SPR_CLAYBALL_PARALLAX_B, 0, 1));
}

/* Type 068: clayball variant E (parallax). */
void EntityType068_ClayballVariantE_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    AddEntityToSortedRenderList(list, InitGenericSpriteEntity(entity, spawnData, SPR_CLAYBALL_PARALLAX_C, 0, 1));
}

/* --- Switch-block clayball family (alloc 0x12C, clayball wired to a switch). --- */
/* Type 090: switch-clayball A. */
void EntityType090_SwitchClayballA_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x12C, 1, 0);
    entity = InitClayballWithSwitchBlock(entity, spawnData, SPR_SWITCH_CLAYBALL_A, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Type 091: switch-clayball B. */
void EntityType091_SwitchClayballB_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x12C, 1, 0);
    entity = InitClayballWithSwitchBlock(entity, spawnData, SPR_SWITCH_CLAYBALL_B, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

/* Type 092: switch-clayball C. */
void EntityType092_SwitchClayballC_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x12C, 1, 0);
    entity = InitClayballWithSwitchBlock(entity, spawnData, SPR_SWITCH_CLAYBALL_C, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern Entity *InitBonusClayballEntity(Entity *entity, EntitySpawnData *spawnData, u32 hash, s32 arg3);

/* Type 093: bonus / score clayball (large 0x130 entity). */
void EntityType093_BonusClayball_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x130, 1, 0);
    entity = InitBonusClayballEntity(entity, spawnData, SPR_BONUS_CLAYBALL, 0);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern Entity *InitGreenBulletsCollectible(Entity *entity, EntitySpawnData *spawnData);

/* Type 002: green bullets ammo pickup. */
void EntityType002_GreenBullets_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = InitGreenBulletsCollectible(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern Entity *InitDecorEntityWithHUDIcon(Entity *entity, EntitySpawnData *spawnData);

/* Type 022: decor entity that also shows a HUD icon. Name 'EnemyCluster'
 * is misleading -- inner init is InitDecorEntityWithHUDIcon. */
void EntityType022_EnemyCluster_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x128, 1, 0);
    entity = InitDecorEntityWithHUDIcon(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern Entity *InitClayballWithRandomColor(Entity *entity, EntitySpawnData *spawnData);

/* Type 007: clayball with randomised colour tint. */
void EntityType007_Clayball_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = InitClayballWithRandomColor(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern Entity *InitExtraLifeCollectible(Entity *entity, EntitySpawnData *spawnData);

/* Type 011: extra-life pickup (inner init is the transparent-decor variant). */
void EntityType011_ExtraLife_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = InitExtraLifeCollectible(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern Entity *InitPlatformDecorEntity(Entity *entity, EntitySpawnData *spawnData);
extern void AddToUpdateQueue(EntityList *list, Entity *entity);

/* Type 009: collectible (inner init is labelled 'platform decor' -- ambiguous,
 * one of the two names is likely wrong). */
void EntityType009_Collectible_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    entity = InitPlatformDecorEntity(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern Entity *InitPhoenixHandCollectible(Entity *entity, EntitySpawnData *spawnData);

/* Type 023: Phoenix Hand powerup pickup. */
void EntityType023_PhoenixHand_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = InitPhoenixHandCollectible(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern Entity *InitPhartHeadCollectible(Entity *entity, EntitySpawnData *spawnData);

/* Type 025: PhartHead collectible (bonus head pickup). */
void EntityType025_PhartHeadCollectible_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    entity = InitPhartHeadCollectible(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern Entity *InitSingleFrameDecorEntity(Entity *entity, EntitySpawnData *spawnData);

/* Type 028: GreenHeart pickup (single-frame decor sprite). */
void EntityType028_GreenHeart_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = InitSingleFrameDecorEntity(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern Entity *InitScaleResetCollectible(Entity *entity, EntitySpawnData *spawnData);

/* Type 029: scale-reset pickup -- clears the shrink/grow powerup scaling. */
void EntityType029_ScaleReset_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = InitScaleResetCollectible(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern Entity *InitUniverseEnemaCollectible(Entity *entity, EntitySpawnData *spawnData);

/* Type 061: 'Universe Enema' bonus pickup. */
void EntityType061_UniverseEnema_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = InitUniverseEnemaCollectible(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern Entity *Init1970IconEntity(Entity *entity, EntitySpawnData *spawnData);

/* Type 069: '1970 icon' (era-themed bonus pickup). */
void EntityType069_1970Icon_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = Init1970IconEntity(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern Entity *InitHamsterShieldCollectible(Entity *entity, EntitySpawnData *spawnData);

/* Type 070: Hamster Shield defensive powerup. */
void EntityType070_HamsterShield_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = InitHamsterShieldCollectible(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern Entity *InitSuperWillieCollectible(Entity *entity, EntitySpawnData *spawnData);

/* Type 072: Super Willie powerup. */
void EntityType072_SuperWillie_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = InitSuperWillieCollectible(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern Entity *InitYellowBirdCollectible(Entity *entity, EntitySpawnData *spawnData);

/* Type 075: Yellow Bird collectible. */
void EntityType075_YellowBird_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = InitYellowBirdCollectible(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern Entity *InitEntity_PathDecor2(Entity *entity, EntitySpawnData *spawnData);

/* Type 081: path-following decor sprite (alt init path). */
void EntityType081_PathDecor2_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x124, 1, 0);
    entity = InitEntity_PathDecor2(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern Entity *InitInteractiveDecorEntity(Entity *entity, EntitySpawnData *spawnData);

/* Type 083: interactive decor (responds to player touch/proximity). */
void EntityType083_InteractiveDecor_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = InitInteractiveDecorEntity(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern Entity *InitCheckpointEntity(Entity *entity, EntitySpawnData *spawnData);

/* Type 119: respawn checkpoint marker. */
void EntityType119_Checkpoint_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x10C, 1, 0);
    entity = InitCheckpointEntity(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern Entity *InitCollectibleEntity_Alt(Entity *entity, EntitySpawnData *spawnData);

/* --- Below: variants that skip AddToUpdateQueue (static / one-shot entities). --- */
/* Type 030: small alt collectible (no tick queue). */
void EntityType030_CollectibleAlt_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x104, 1, 0);
    AddEntityToSortedRenderList(list, InitCollectibleEntity_Alt(entity, spawnData));
}

extern u8 SCALED_PLATFORM_CONFIG_MOVING[] asm("D_8009DA9C");
extern u8 SCALED_PLATFORM_CONFIG_TRIGGER[] asm("D_8009DAA8");
extern Entity *InitScaledPlatformEntity(Entity *entity, EntitySpawnData *spawnData, u8 *data);

/* Types 031/032/033: scaled-moving platforms (SCALED_PLATFORM_CONFIG_MOVING config). Render only. */
void EntityType031_032_033_ScaledMovingPlatform_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x110, 1, 0);
    AddEntityToSortedRenderList(list, InitScaledPlatformEntity(entity, spawnData, SCALED_PLATFORM_CONFIG_MOVING));
}

/* Types 034/035/036: trigger-actuated platforms (SCALED_PLATFORM_CONFIG_TRIGGER config). */
void EntityType034_035_036_TriggerPlatform_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x110, 1, 0);
    AddEntityToSortedRenderList(list, InitScaledPlatformEntity(entity, spawnData, SCALED_PLATFORM_CONFIG_TRIGGER));
}

extern Entity *InitDirectionalScaledEntity(Entity *entity, EntitySpawnData *spawnData, s32 arg2);

/* Types 037/038: directional scaled entities (config arg=0). */
void EntityType037_038_DirectionalScaled_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x108, 1, 0);
    AddEntityToSortedRenderList(list, InitDirectionalScaledEntity(entity, spawnData, 0));
}

extern Entity *InitScaledMovingEntity(Entity *entity, EntitySpawnData *spawnData);

/* Type 046: scaled moving object (render only). */
void EntityType046_Object_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x108, 1, 0);
    AddEntityToSortedRenderList(list, InitScaledMovingEntity(entity, spawnData));
}

extern Entity *InitFloatingPlatformEntity(Entity *entity, EntitySpawnData *spawnData);

/* Types 039/052: floating platform-style enemy. */
void EntityType039_052_FloatingEnemy_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x114, 1, 0);
    entity = InitFloatingPlatformEntity(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern Entity *InitPathFollowingEntity_Alt(Entity *entity, EntitySpawnData *spawnData);

/* Type 040: path-following mechanism / contraption. */
void EntityType040_Mechanism_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = InitPathFollowingEntity_Alt(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern Entity *InitWalkingCollectibleEnemy(Entity *entity, EntitySpawnData *spawnData);

/* Type 041: walking enemy that drops a collectible on defeat ('Clay Keeper'). */
void EntityType041_ClayKeeperEnemy_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x11C, 1, 0);
    entity = InitWalkingCollectibleEnemy(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern Entity *InitSnoBloEnemy(Entity *entity, EntitySpawnData *spawnData);

/* Types 042-44 / 53-55 / 60: 'Sno Blo' enemies. Wrapper name reads as a
 * contiguous 42-thru-60 range, but the actual slots are non-contiguous
 * (see ENTITY_TYPE_*_SNO_BLO entries in entity_types.h) -- the dispatch
 * table just maps each of those slots to this same callback. */
void EntityType042_thru_060_SnoBlo_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x114, 1, 0);
    entity = InitSnoBloEnemy(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern Entity *InitSoundEmittingEnemy(Entity *entity, EntitySpawnData *spawnData);

/* Types 047/048: sound-emitting platform/enemy. */
void EntityType047_048_SoundPlatform_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x114, 1, 0);
    entity = InitSoundEmittingEnemy(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern Entity *InitTimerBasedMenuEntity(Entity *entity, EntitySpawnData *spawnData);

/* Type 080: tiny (0x2C, ~menu-base sized) timer-driven manager entity.
 * Uses AddToZOrderList (not AddToUpdateQueue) -- behaves like a UI element. */
void EntityType080_TimerMenu_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x2C, 1, 0);
    AddToZOrderList(list, InitTimerBasedMenuEntity(entity, spawnData));
}

extern Entity *InitBouncableClayEntity(Entity *entity, EntitySpawnData *spawnData);

/* Type 045: bounceable clay block. */
void EntityType045_BounceClay_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x114, 1, 0);
    entity = InitBouncableClayEntity(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern Entity *InitPasswordEntity(Entity *entity, EntitySpawnData *spawnData);

/* Type 065: password-input entity (render only, no per-frame tick here). */
void EntityType065_Password_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    AddEntityToSortedRenderList(list, InitPasswordEntity(entity, spawnData));
}

extern Entity *InitMonkeyMageBoss(Entity *entity, EntitySpawnData *spawnData);

/* --- Boss inits. Each is a single, large entity allocated for one specific
 *     boss encounter (allocSize varies per-boss because each carries its own
 *     state machine workspace). --- */
/* Type 071: Monkey Mage boss (largest entity at 0x148). */
void EntityType071_Boss_MonkeyMage_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x148, 1, 0);
    entity = InitMonkeyMageBoss(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern Entity *InitGlennYntisBoss(Entity *entity, EntitySpawnData *spawnData);

/* Type 100: Glenn Yntis boss. */
void EntityType100_Boss_GlennYntis_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x11C, 1, 0);
    entity = InitGlennYntisBoss(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern Entity *InitShrineyGuardBoss(Entity *entity, EntitySpawnData *spawnData);

/* Type 101: Shriney Guard boss. */
void EntityType101_Boss_ShrineyGuard_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    entity = InitShrineyGuardBoss(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern Entity *InitJoeHeadJoeBoss(Entity *entity, EntitySpawnData *spawnData);

/* Type 102: Joe Head Joe boss. */
void EntityType102_Boss_JoeHeadJoe_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x11C, 1, 0);
    entity = InitJoeHeadJoeBoss(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern Entity *InitKloggBoss(Entity *entity, EntitySpawnData *spawnData);

/* Type 103: Klogg boss. */
void EntityType103_Boss_Klogg_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x118, 1, 0);
    entity = InitKloggBoss(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern Entity *InitEnemySpawnerEntity(Entity *entity, EntitySpawnData *spawnData);

/* Type 079: tiny (0x30) enemy-spawner manager entity (z-order list, not tick queue). */
void EntityType079_EnemySpawner_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x30, 1, 0);
    AddToZOrderList(list, InitEnemySpawnerEntity(entity, spawnData));
}

extern Entity *InitPathFollowingHazard(Entity *entity, EntitySpawnData *spawnData);

/* Type 082: path-following hazard (damages on touch). */
void EntityType082_PathHazard_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x114, 1, 0);
    entity = InitPathFollowingHazard(entity, spawnData);
    AddEntityToSortedRenderList(list, entity);
    AddToUpdateQueue(list, entity);
}

extern Entity *InitTriggerZoneEntity(Entity *entity, EntitySpawnData *spawnData);

/* Types 085/104/105: invisible trigger-zone entity (render-list only). */
void EntityType085_104_105_TriggerZone_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x110, 1, 0);
    AddEntityToSortedRenderList(list, InitTriggerZoneEntity(entity, spawnData));
}

extern Entity *InitGridLineEntity(Entity *entity, EntitySpawnData *spawnData, s32 spriteId, s32 mirrorFlag, s32 flag);

/* Types 089/097/098/110/111: grid/parallax helper. Unique among inits because
 * it inspects spawnData+0x12 (entity subtype) to pick a size + flag triple
 * before allocating the small (0x80) entity header. */
void EntityType089_097_098_110_111_GridHelper_Init(EntityList *list, EntitySpawnData *spawnData) {
    s32 size = 0x4B0;
    s32 useMirror = 0;
    u16 type = ((EntityTypeSpawnDef *)spawnData)->type;
    s32 flag = 0x10000;
    Entity *entity;

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

extern Entity *InitIndexedSpriteEntity(Entity *entity, EntitySpawnData *spawnData);

/* Type 094: indexed sprite entity. Clamps spawnData[3] (sprite index) into
 * the valid range [0xB5, 0xDC] before init; out-of-range gets snapped to 0xB5. */
void EntityType094_IndexedSprite_Init(EntityList *list, EntitySpawnData *spawnData) {
    s32 *ptr = (s32 *)spawnData;
    Entity *entity;
    if ((u32)(ptr[3] - 0xB5) >= 0x28) {
        ptr[3] = 0xB5;
    }
    entity = AllocateFromHeap(g_pBlbHeapBase, 0x104, 1, 0);
    AddEntityToSortedRenderList(list, InitIndexedSpriteEntity(entity, spawnData));
}

extern Entity *InitCameraTrackingEntity(Entity *entity, EntitySpawnData *spawnData);

/* Type 095: camera-tracked sound emitter (tiny 0x28, z-order list). Either the
 * wrapper name ('SoundEmitterPanning') or the inner init name
 * ('InitCameraTrackingEntity') is the wrong half of the same entity --
 * likely it's a positional audio source that pans with the camera. */
void EntityType095_SoundEmitterPanning_Init(EntityList *list, EntitySpawnData *spawnData) {
    Entity *entity = AllocateFromHeap(g_pBlbHeapBase, 0x28, 1, 0);
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

extern u8 *BLB_HEAP_CONTEXT asm("D_800A595C");
extern u8 LEVEL_SPRITE_HASH_TABLE[] asm("D_8009DAB4");
extern void LoadSpriteHashArrayToVRAM(u8 *heap, u8 *data);
extern void LoadSpriteFramesToVRAM(u8 *heap, u32 hash);

/* Uploads the level's sprite-hash array (LEVEL_SPRITE_HASH_TABLE) into VRAM, plus one
 * extra sprite (SPR_LEVEL_FLAG2_EXTRA) when the current level has flag bit 2 set. */
void LoadLevelSpriteAssets(LevelFlagsOwner *arg) {
    LoadSpriteHashArrayToVRAM(BLB_HEAP_CONTEXT, LEVEL_SPRITE_HASH_TABLE);
    if (GetLevelFlags(arg->levelFlagsContext) & 4) {
        LoadSpriteFramesToVRAM(BLB_HEAP_CONTEXT, SPR_LEVEL_FLAG2_EXTRA);
    }
}

extern u8 SLOPE_HEIGHT_TABLE[][16] asm("D_8009D228");

/* Slope-tile height lookup: returns the per-subpixel ground height for the
 * given tile type from the 16-entry-per-row SLOPE_HEIGHT_TABLE table. */
u8 GetSlopeHeightAtSubpixel(Entity *unused, u32 tileType, u32 subpixel) {
    return SLOPE_HEIGHT_TABLE[tileType & 0xFF][subpixel & 0xF];
}

/* Sets the player's current speed (entity+0x140) and propagates it to the
 * attached vehicle (entity+0x2C -> +0x100) and HUD speed slot (entity+0x14C
 * -> +0x1C). The chain of GetLevelFlags() calls is a side-effect dispatch
 * (each call also runs callback hooks on the level-flags object) that gates
 * whether the vehicle copy actually happens for special level modes. */
void SetPlayerVehicleSpeed(PlayerVehicleEntity *entity, s32 speed) {
    PlayerVehicleHud *other;
    u8 *ptr;
    entity->speed = speed;
    if (entity->vehicle) {
        ptr = entity->levelFlagsContext;
        if (GetLevelFlags(ptr) & 0x400) goto store;
        if (GetLevelFlags(ptr) & 0x200) goto store;
        if (GetLevelFlags(ptr) & 0x2000) goto store;
        if (GetLevelFlags(ptr) & 0x100) goto store;
        if (GetLevelFlags(ptr) & 0x10) goto store;
        GetLevelFlags(ptr);
store:
        entity->vehicle->speed = entity->speed;
    }
    other = entity->hud;
    if (other) {
        other->speed = entity->speed;
    }
}

extern Entity *InitAlternateEntity(Entity *entity, EntitySpawnData *spawnData);

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
void ClearAlternateEntitySpawnFlags(AlternateEntityState *state) {
    s16 i;
    AlternateEntitySlot *ptr = state->slots;
    for (i = 0; i < state->count; i++) {
        ptr->spawned = 0;
        ptr++;
    }
}

/* Links a render primitive node into one of 256 depth buckets stored at
 * gameState+0x16C. Bucket index = (node->depth >> 1), clamped to 255.
 * Each bucket is a singly-linked list (node->next at offset 0); newest node
 * inserted at the head. Drained later by FlushDepthBuckets. */
void AddToDepthBucket(DepthBucketGameState *gameState, DepthBucketNode *node) {
    s32 bucket_idx;
    void **buckets;
    u16 bucket;
    u32 raw = (u32)node->depth >> 1;
    bucket = raw;
    if ((s32)raw >= 0x100) {
        bucket = 0xFF;
    }
    buckets = (void **)gameState->depthBuckets;
    if (buckets[bucket] != NULL) {
        node->next = buckets[bucket];
    } else {
        node->next = NULL;
    }
    bucket_idx = bucket;
    gameState->depthBuckets[bucket_idx] = node;
}

extern void SetPolyGT4(DepthBucketNode *prim);

/* Walks the 256 depth buckets (entity+0x16C) front-to-back, calling SetPolyGT4
 * on each primitive and inserting it into the active GPU ordering table
 * (heap+0xA084 -> +0x70). Clears each bucket head as it goes; this is the
 * per-frame drain that turns the bucket queue into GPU draw commands. */
void FlushDepthBuckets(DepthBucketGameState *entity) {
    s16 i;
    DepthBucketNode **buckets;
    DepthBucketNode *node;
    DepthBucketNode *next;
    void *ot;

    i = 0;
    buckets = entity->depthBuckets;
loop_start:
    if ((s16)i >= 256) goto loop_end;
    {
        DepthBucketNode *head = *buckets;
        if (head == NULL) goto advance;
        node = head;
    }
inner:
    if (node == NULL) {
        *buckets = NULL;
        goto advance;
    }
    next = node->next;
    SetPolyGT4(node);
    ot = ((BlbHeapState *)g_pBlbHeapBase)->otContext->orderingTable;
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
void ClearEntityPoolArray(DepthBucketGameState *state) {
    s16 i;
    for (i = 0; i < 256; i++) {
        state->depthBuckets[i] = 0;
    }
}

/* Returns 1 if this is a normal side-scrolling platformer level, 0 if it's
 * any special mode (boss arena, glide/SOAR, auto-scroll, FINN vehicle, etc.).
 * The cascade of GetLevelFlags checks short-circuits to 0 on any special-mode
 * bit; result is the inverted bit-2 ('use parallax / standard layers') test. */
s32 IsNormalPlatformLevel(LevelFlagsOwner *arg) {
    s32 result = 0;
    u8 *ptr = arg->levelFlagsContext;
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

/* .data island 0x8009D9FC..0x8009DAE0 (228B, entinit collectible/enemy config + sprite hash tables) migrated from asm. */
/* Packed 32-bit config/sprite-id sub-lists (null-terminated), carved by the
 * word-aligned +N aliases into per-entity config tables. */
u32 D_8009D9FC[57] asm("D_8009D9FC") = {
    0x400C991D, 0x61981A0C, 0x40189A1D, 0xE398D28D,
    0x75189608, 0x75189608, 0x00000000, 0xF8494814,
    0x3A186940, 0xBA094800, 0x980C6211, 0xB84968B0,
    0x3A186980, 0x00000000, 0x404C5919, 0x61981A0C,
    0x40189A1D, 0xE398D28D, 0x75189608, 0x75189608,
    0x00000000, 0x0428801E, 0x0408C01E, 0x00000000,
    0x004C9138, 0x40489938, 0x00000000, 0x004A981C,
    0x024E981C, 0x425A399C, 0x00000000, 0x040A2110,
    0x040A2110, 0x00000000, 0xA00A0C30, 0xA00A0C30,
    0x00000000, 0xB12C0854, 0xB92C4850, 0x00000000,
    0x84004480, 0xFB260440, 0x00000000, 0x84004480,
    0x9B2204A0, 0x00000000, 0x88A28194, 0xFA700CE7,
    0xB8700CA1, 0xE8628689, 0x80E85EA0, 0xA9240484,
    0x00E2F188, 0x9158A0F6, 0x902C0002, 0x000AC607,
    0x00000000,
};
asm(".globl D_8009DA18\n.set D_8009DA18, D_8009D9FC + 28\n.globl D_8009DA34\n.set D_8009DA34, D_8009D9FC + 56\n.globl D_8009DA50\n.set D_8009DA50, D_8009D9FC + 84\n.globl D_8009DA5C\n.set D_8009DA5C, D_8009D9FC + 96\n.globl D_8009DA68\n.set D_8009DA68, D_8009D9FC + 108\n.globl D_8009DA78\n.set D_8009DA78, D_8009D9FC + 124\n.globl D_8009DA84\n.set D_8009DA84, D_8009D9FC + 136\n.globl D_8009DA90\n.set D_8009DA90, D_8009D9FC + 148\n.globl D_8009DA9C\n.set D_8009DA9C, D_8009D9FC + 160\n.globl D_8009DAA8\n.set D_8009DAA8, D_8009D9FC + 172\n.globl D_8009DAB4\n.set D_8009DAB4, D_8009D9FC + 184\n");
