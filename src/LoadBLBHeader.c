/**
 * LoadBLBHeader.c - BLB Archive Header Initialization
 * 
 * This module initializes the BLB (game archive) system by reading the
 * first 2 sectors (4096 bytes) of GAME.BLB into memory.
 * 
 * The BLB header contains the asset/level index table with entries
 * for locating data within the archive.
 * 
 * NON-MATCHING: Uses GP-relative store for D_800A5960 which GCC 2.7.2
 * cannot generate for extern variables.
 */

#include "common.h"

extern void func_8007BFB8(void);
extern void func_8007A1BC(void *, s32, void *);
extern void func_8007BB00(s32, void *);
extern void func_80020848(void);

extern s32 D_800A5954;
extern void *D_800A5960;
extern s32 D_801FC400;

typedef struct GameState {
    u8 pad00[0x28];
    s32 unk28;          // 0x28
    u8 pad2C[0x10];
    s32 unk3C;          // 0x3C - secondary buffer (header + 0x1000)
    s32 unk40;          // 0x40 - BLB header buffer
    u8 pad44[0x20];
    s16 unk64;          // 0x64
    s16 unk66;          // 0x66
    u8 pad68[0x1C];
    u8 unk84[0x80];     // 0x84 - callback struct
    s16 unk104;         // 0x104
    u8 pad106[0x2];
    s32 unk108;         // 0x108
    u8 pad10C[0x4];
    s32 unk110;         // 0x110
    u8 unk114;          // 0x114
    u8 pad115[0x7];
    s32 unk11C;         // 0x11C - buffer size
    u8 pad120[0xC];
    s32 unk12C;         // 0x12C - magic value
    u8 unk130;          // 0x130
} GameState;

#ifdef NON_MATCHING
// This code compiles and is validated by the IDE, but doesn't byte-match
// due to GP-relative addressing differences. Use -DNON_MATCHING to build.
void LoadBLBHeader(GameState *state) {
    s32 blbHeaderBuffer;

    func_8007BFB8();
    
    D_800A5960 = state;  // GP-relative store - can't match
    
    state->unk114 = 0;
    state->unk130 = 0;
    state->unk11C = 0x10000;
    state->unk28 = 0;
    state->unk104 = 0;
    state->unk108 = 0;
    state->unk110 = 0;
    state->unk64 = 0;
    state->unk66 = 0;
    
    state->unk12C = 0x01234567;
    blbHeaderBuffer = *(s32 *)((u8 *)D_800A5954 + 0xA650);
    D_801FC400 = 0x01234567;
    state->unk40 = blbHeaderBuffer;
    state->unk3C = blbHeaderBuffer + 0x1000;
    
    CdBLB_ReadSectors(0, 2, (void *)state->unk40);
    
    func_8007A1BC((void *)state->unk84, state->unk40, &func_80020848);
    func_8007BB00(0, (void *)state->unk84);
}
#else
INCLUDE_ASM("asm/pal/nonmatchings/LoadBLBHeader", LoadBLBHeader);
#endif
