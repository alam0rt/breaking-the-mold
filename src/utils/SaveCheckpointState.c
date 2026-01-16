#include "common.h"

/**
 * SaveCheckpointState - Save current entity state for checkpoint respawn
 * 
 * Saves the entity tick list from gameState+0x1c to gameState+0x134,
 * stores the current score at +0x138, and sets checkpoint flags.
 * Then adds the player entity to the Z-order list.
 * 
 * This allows the game to respawn entities at their checkpoint positions
 * when the player dies.
 * 
 * @param gameState  Pointer to GameState structure (passed in $a0)
 */
#ifdef MATCHING_HACK_NOP
__asm__(".rept 14 ; nop ; .endr");
#endif
void SaveCheckpointState(void *gameState) {
    s32 temp_a1 = *(s32 *)((u8 *)gameState + 0x1C);
    s32 temp_v1 = *(s32 *)((u8 *)gameState + 0x10C);
    s32 temp_a1_2 = *(s32 *)((u8 *)gameState + 0x2C);
    s32 temp_v0 = 1;
    
    *(s32 *)((u8 *)gameState + 0x134) = temp_a1;
    *(u8 *)((u8 *)gameState + 0x14A) = temp_v0;
    *(u8 *)((u8 *)gameState + 0x63) = temp_v0;
    *(s32 *)((u8 *)gameState + 0x1C) = 0;
    AddToZOrderList(temp_a1_2);
    *(s32 *)((u8 *)gameState + 0x138) = temp_v1;
}
