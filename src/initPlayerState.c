#include "common.h"
#include "game.h"

/* =============================================================================
 * initPlayerState - Initialize default player state values
 * @ 0x8001F3B4
 * =============================================================================
 * Initializes a PlayerState structure with default values for a new game.
 * 
 * Note: To match the original binary, we must use pointer arithmetic instead
 * of direct struct member access. This preserves the exact instruction order
 * and register allocation from the original code.
 * =============================================================================
 */
void initPlayerState(PlayerState *state) {
    s16 i;
    u8 *ptr = (u8 *)state;  // Use byte pointer for exact matching

    ptr[0x00] = 1;    // lives
    ptr[0x01] = 1;    // continues
    ptr[0x11] = 5;    // health
    ptr[0x12] = 0;
    ptr[0x14] = 0;
    ptr[0x15] = 0;
    ptr[0x16] = 0;
    ptr[0x1C] = 0;
    ptr[0x13] = 0;
    ptr[0x19] = 0;
    ptr[0x1A] = 0;
    ptr[0x1B] = 0;
    ptr[0x17] = 0;
    ptr[0x18] = 0;
    ptr[0x1D] = 0;
    ptr[0x10] = 1;
    *(s16 *)&ptr[0x02] = 0;  // unknown02
    ptr[0x04] = 0;
    ptr[0x05] = 0;

    // Clear level unlock flags (10 bytes)
    for (i = 0; i < 10; i++) {
        ptr[i + 0x06] = 0;
    }
}
