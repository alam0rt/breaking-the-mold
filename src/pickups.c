#include "common.h"

extern void *g_pBlbHeapBase;
extern void FreeFromHeap(void *heap, void *ptr, s32 a2, s32 a3);

INCLUDE_ASM("asm/nonmatchings/pickups", InitGreenBulletsCollectible);

INCLUDE_ASM("asm/nonmatchings/pickups", DecorEntity_CollectWithSwirlyEffect);

INCLUDE_ASM("asm/nonmatchings/pickups", InitDecorEntityWithHUDIcon);

INCLUDE_ASM("asm/nonmatchings/pickups", EntityDestructor_FreeRenderListAt120);

INCLUDE_ASM("asm/nonmatchings/pickups", CollectibleOrbTickCallback);

INCLUDE_ASM("asm/nonmatchings/pickups", EntityRenderWithScaledPosition);

INCLUDE_ASM("asm/nonmatchings/pickups", func_8002D978);

INCLUDE_ASM("asm/nonmatchings/pickups", TriggerFadeOut);

INCLUDE_ASM("asm/nonmatchings/pickups", InitYellowBirdCollectible);

INCLUDE_ASM("asm/nonmatchings/pickups", CollectibleYellowBirdTickCallback);

INCLUDE_ASM("asm/nonmatchings/pickups", InitClayballWithRandomColor);

INCLUDE_ASM("asm/nonmatchings/pickups", CollectibleClaySingleTickCallback);

INCLUDE_ASM("asm/nonmatchings/pickups", InitPlatformDecorEntity);

INCLUDE_ASM("asm/nonmatchings/pickups", CheckpointSwampTickCallback);

INCLUDE_ASM("asm/nonmatchings/pickups", DecorSetSpriteIdle);

INCLUDE_ASM("asm/nonmatchings/pickups", DecorSetSpriteActive);

INCLUDE_ASM("asm/nonmatchings/pickups", DecorStartAnimation);

INCLUDE_ASM("asm/nonmatchings/pickups", DecorStartAnimationAlt);

INCLUDE_ASM("asm/nonmatchings/pickups", InitTransparentDecorEntity);

INCLUDE_ASM("asm/nonmatchings/pickups", CollectibleExtraLifeTickCallback);

INCLUDE_ASM("asm/nonmatchings/pickups", InitPhoenixHandCollectible);

INCLUDE_ASM("asm/nonmatchings/pickups", CollectiblePhoenixHandTickCallback);

INCLUDE_ASM("asm/nonmatchings/pickups", InitPhartHeadCollectible);

INCLUDE_ASM("asm/nonmatchings/pickups", CollectiblePhartHeadTickCallback);

INCLUDE_ASM("asm/nonmatchings/pickups", InitUniverseEnemaCollectible);

INCLUDE_ASM("asm/nonmatchings/pickups", CollectibleUniverseEnemaTickCallback);

INCLUDE_ASM("asm/nonmatchings/pickups", InitSingleFrameDecorEntity);

INCLUDE_ASM("asm/nonmatchings/pickups", TriggerCollectible100CTickCallback);

INCLUDE_ASM("asm/nonmatchings/pickups", InitScaleResetCollectible);

INCLUDE_ASM("asm/nonmatchings/pickups", ScaleResetCollectibleTickCallback);

INCLUDE_ASM("asm/nonmatchings/pickups", Init1970IconEntity);

INCLUDE_ASM("asm/nonmatchings/pickups", Collectible1970IconTickCallback);

INCLUDE_ASM("asm/nonmatchings/pickups", InitHamsterShieldCollectible);

INCLUDE_ASM("asm/nonmatchings/pickups", CollectibleHamsterShieldTickCallback);

INCLUDE_ASM("asm/nonmatchings/pickups", InitSuperWillieCollectible);

INCLUDE_ASM("asm/nonmatchings/pickups", CollectibleSuperWillieTickCallback);

INCLUDE_ASM("asm/nonmatchings/pickups", InitEntity_PathDecor2);

INCLUDE_ASM("asm/nonmatchings/pickups", DecorCollisionTickWithSpawnAndSound);

INCLUDE_ASM("asm/nonmatchings/pickups", DecorSetRandomTimer);

INCLUDE_ASM("asm/nonmatchings/pickups", DecorStartWithRandomTimer);

INCLUDE_ASM("asm/nonmatchings/pickups", InitInteractiveDecorEntity);

INCLUDE_ASM("asm/nonmatchings/pickups", DecorEntityTickWithCollision);

INCLUDE_ASM("asm/nonmatchings/pickups", DecorEntityCollisionHandler);

INCLUDE_ASM("asm/nonmatchings/pickups", DecorEntityCollisionHandlerExt);

INCLUDE_ASM("asm/nonmatchings/pickups", DecorPlaySoundAndAnimate);

INCLUDE_ASM("asm/nonmatchings/pickups", DecorEntityDestroyWithParticles);

INCLUDE_ASM("asm/nonmatchings/pickups", SpawnEntityOrTriggerZone);

INCLUDE_ASM("asm/nonmatchings/pickups", InitEntityType0x3DE);

INCLUDE_ASM("asm/nonmatchings/pickups", DecorEntitySpawnExplosionPieces);

INCLUDE_ASM("asm/nonmatchings/pickups", EntityDestructor_Vtable0x80010870_A);

INCLUDE_ASM("asm/nonmatchings/pickups", EntityDestructor_VtableAndHeapFree);

INCLUDE_ASM("asm/nonmatchings/pickups", EntityDestructor_Vtable0x80010870_B);

INCLUDE_ASM("asm/nonmatchings/pickups", EntityDestructor_Vtable0x80010870_C);

INCLUDE_ASM("asm/nonmatchings/pickups", EntityDestructor_Vtable0x80010870_D);

INCLUDE_ASM("asm/nonmatchings/pickups", EntityDestructor_Vtable0x80010870_E);

INCLUDE_ASM("asm/nonmatchings/pickups", EntityDestructor_Vtable0x80010870_F);

INCLUDE_ASM("asm/nonmatchings/pickups", EntityDestructor_Vtable0x80010870_G);

INCLUDE_ASM("asm/nonmatchings/pickups", EntityDestructor_Vtable0x80010870_H);

INCLUDE_ASM("asm/nonmatchings/pickups", EntityDestructor_Vtable0x80010870_I);

INCLUDE_ASM("asm/nonmatchings/pickups", EntityDestructor_Vtable0x80010870_J);

INCLUDE_ASM("asm/nonmatchings/pickups", EntityDestructor_Vtable0x80010870_K);

INCLUDE_ASM("asm/nonmatchings/pickups", EntityDestructor_Vtable0x80010870_L);

INCLUDE_ASM("asm/nonmatchings/pickups", EntityDestructor_Vtable0x80010870_M);

INCLUDE_ASM("asm/nonmatchings/pickups", EntityDestructor_Vtable0x80010870_N);

INCLUDE_ASM("asm/nonmatchings/pickups", EntityDestructor_Vtable0x80010870_O);

INCLUDE_ASM("asm/nonmatchings/pickups", EntityDestructor_Vtable0x80010870_P);

void func_80030C98(void) {
}

void func_80030CA0(void) {
}

INCLUDE_ASM("asm/nonmatchings/pickups", EntityDestructor_Vtable0x80010870_Q);

void FreeEntityNoTeardown_80030cdc(void *e) {
    FreeFromHeap(g_pBlbHeapBase, e, 0, 0);
}

