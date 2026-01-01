#include "common.h"

// Player state structure - initializes default values
void initPlayerState(u8 *state) {
    s16 i;

    state[0x00] = 1;    // lives?
    state[0x01] = 1;    // continues?
    state[0x11] = 5;    // some counter (health?)
    state[0x12] = 0;
    state[0x14] = 0;
    state[0x15] = 0;
    state[0x16] = 0;
    state[0x1C] = 0;
    state[0x13] = 0;
    state[0x19] = 0;
    state[0x1A] = 0;
    state[0x1B] = 0;
    state[0x17] = 0;
    state[0x18] = 0;
    state[0x1D] = 0;
    state[0x10] = 1;
    *(s16 *)&state[0x02] = 0;  // 16-bit value
    state[0x04] = 0;
    state[0x05] = 0;

    // Clear 10 bytes starting at offset 6 (level unlock flags?)
    for (i = 0; i < 10; i++) {
        *(state + i + 0x06) = 0;
    }
}
