/* =============================================================================
 * sprite_def_lists.c  --  PC-port strong backing for entity sprite-def lists
 * =============================================================================
 * Null-terminated sprite-hash lists passed to InitEntityWithSprite by the
 * entity-type inits (transcribed word-for-word from asm/data/8B944.data.s /
 * 8BCDC.data.s / 8BE34.data.s). The values are opaque 32-bit asset hashes,
 * NOT pointers, so these blobs cannot go through gen_port_data.py's
 * pointer-table heuristic. Weak-zero fallbacks would give every one of these
 * entity types a null sprite.
 * ========================================================================== */

typedef unsigned int u32_;

u32_ D_8009B214[5] asm("D_8009B214") = {   /* type-009 platform decor */
    0x91106183, 0x980861A3, 0x880161A7, 0x09406D8A, 0
};
u32_ D_8009B4DC[4] asm("D_8009B4DC") = {   /* type-024 special ammo */
    0x60B8BC9C, 0x60B98CBD, 0x64BB1CBE, 0
};
u32_ D_8009B54C[4] asm("D_8009B54C") = {   /* types 106/107/108 falling platform */
    0x60181A0C, 0x611C5804, 0x400C9A1D, 0
};
u32_ D_8009B6A4[3] asm("D_8009B6A4") = {   /* types 085/104/105 trigger zone */
    0x9B0AB1C6, 0x9389B8C4, 0
};
u32_ D_8009B6BC[4] asm("D_8009B6BC") = {   /* type-045 bounce clay */
    0xF175320E, 0x657C322C, 0xE7443A6F, 0
};
u32_ D_8009B6CC[3] asm("D_8009B6CC") = {   /* trigger-zone child sprite (0x68) */
    0x80E85EA0, 0x0BBB70AC, 0
};
u32_ D_8009B6D8[3] asm("D_8009B6D8") = {   /* trigger-zone child sprite (0x69) */
    0xA9387D8C, 0xC9387D8C, 0
};
u32_ D_8009B844[4] asm("D_8009B844") = {   /* types 039/052 floating platform */
    0xB4011101, 0xE6830101, 0xE4834384, 0
};
