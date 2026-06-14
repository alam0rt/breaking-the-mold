#include "common.h"
#include "Game/entity.h"

extern void EntityProcessCallbackQueue(Entity *entity);
extern s32 PlatformEntityOnCollision(Entity *entity, u32 event, u32 arg2, u32 arg3);

INCLUDE_ASM("asm/nonmatchings/Game/PLAYER_STATES/player_states", CalcEntityYFromTileCollision);

INCLUDE_ASM("asm/nonmatchings/Game/PLAYER_STATES/player_states", CheckEntityNearTileY);

INCLUDE_ASM("asm/nonmatchings/Game/PLAYER_STATES/player_states", PlatformTick_MainWithSoundAndTimers);

INCLUDE_ASM("asm/nonmatchings/Game/PLAYER_STATES/player_states", PlatformEntityOnCollision);

s32 PlatformEvent_CollisionAndQueue(Entity *entity, u32 event, u32 arg2, u32 arg3) {
    s32 result;
    u32 maskedEvent = event & 0xFFFF;
    result = PlatformEntityOnCollision(entity, maskedEvent, arg2, arg3);
    if (maskedEvent == 2) {
        EntityProcessCallbackQueue(entity);
        return result;
    }
    return result;
}

s32 PlatformEvent_QueueOnAnimReady(Entity *entity, u32 event) {
    if ((event & 0xFFFF) == 2) {
        EntityProcessCallbackQueue(entity);
    }
    return 0;
}

void func_80071778(Entity *entity) {
    s32 pos = ((s32)entity->worldX << 16) + (u16)entity->velocityX + *(s32 *)((u8 *)entity + 0x104);
    entity->worldX = pos >> 16;
    entity->velocityX = (s16)pos;
}

INCLUDE_ASM("asm/nonmatchings/Game/PLAYER_STATES/player_states", PlatformEntityTickMoving);

INCLUDE_ASM("asm/nonmatchings/Game/PLAYER_STATES/player_states", PlatformSpeedRampTick);

INCLUDE_ASM("asm/nonmatchings/Game/PLAYER_STATES/player_states", RunnRender_PhysicsAndTileCollision);

INCLUDE_ASM("asm/nonmatchings/Game/PLAYER_STATES/player_states", RunnState_DieAndAdvanceLevel);

INCLUDE_ASM("asm/nonmatchings/Game/PLAYER_STATES/player_states", func_80071D28);

INCLUDE_ASM("asm/nonmatchings/Game/PLAYER_STATES/player_states", RunnLand_GroundContactAndWallCheck);

INCLUDE_ASM("asm/nonmatchings/Game/PLAYER_STATES/player_states", RunnLand_HazardDamageTransition);
