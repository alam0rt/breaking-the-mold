/* =============================================================================
 * player_spawn_data.c  --  PC-port strong backing for player spawn data blobs
 * =============================================================================
 * D_8009C174: the player spawn spriteDef blob (0xE0 bytes, opaque sprite-hash
 * id list; first word = the default player sprite id read by
 * InitEntityWithSprite -> SetEntitySpriteId). Verbatim from
 * asm/data/8C974.data.s.
(D_8009C3A8, the 7-candidate pose table, already has a strong C def in
 * src/player.c and is NOT duplicated here.)
 * ========================================================================== */

unsigned int D_8009C174_data[56] asm("D_8009C174") = {
    0x08208902, 0x48204012, 0x8569A090, 0x0708A4A0,
    0x052AA082, 0x393C80C2, 0x1CF99931, 0x00388110,
    0x1C3AA013, 0x1C395196, 0x3838801A, 0x04084011,
    0x092B8480, 0x0B2084D0, 0x292E8480, 0x282B8491,
    0xD02DC3D5, 0xC8099196, 0x88B9833C, 0x8D01C734,
    0x0A3A4051, 0x1A28C11A, 0x0E0AC230, 0x112AE088,
    0x0628A800, 0x04084011, 0x18298210, 0x1E1000B3,
    0x1D1811B3, 0x392CB014, 0x0AABC010, 0x8524A880,
    0x1E28E0D4, 0x1E089010, 0x4A298690, 0x0A08C490,
    0x5A2082F2, 0x5900C41E, 0x1DF99931, 0x1EF99931,
    0xFA20CD14, 0x6C22083A, 0x987101B9, 0x90B102A1,
    0xD0A5F094, 0x90A19011, 0x1B301085, 0x1C8C2437,
    0x48608484, 0xF936C015, 0x5D2CE434, 0x52A08394,
    0x5A204C30, 0x80E85EA0, 0xD305D045, 0x00000000,
};

