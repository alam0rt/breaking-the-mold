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
extern void *D_800A5960; // Points at the currently active GameState buffer
extern s32 D_801FC400; // Marker that the BLB header buffer is initialised

typedef struct GameState {
    u8 pad00[0x28];
    s32 unk28;          // 0x28
    u8 pad2C[0x10];
    s32 headerMirror;   // 0x3C - 0x1000 bytes after headerBuffer
    s32 headerBuffer;   // 0x40 - BLB header buffer (first 2 sectors)
    u8 pad44[0x20];
    s16 unk64;          // 0x64
    s16 unk66;          // 0x66
    u8 pad68[0x1C];
    u8 cdRequest[0x80]; // 0x84 - CD read callback / BLB header window
    s16 unk104;         // 0x104
    u8 pad106[0x2];
    s32 unk108;         // 0x108
    u8 pad10C[0x4];
    s32 unk110;         // 0x110
    u8 unk114;          // 0x114
    u8 pad115[0x7];
    s32 headerBufferSize;   // 0x11C - BLB header allocation size (0x10000)
    u8 pad120[0xC];
    s32 headerMagic;    // 0x12C - init sentinel (0x01234567)
    u8 unk130;          // 0x130
} GameState;

typedef struct EngineContext {
    u8 pad0000[0xA650];
    void *blbHeaderBufferBase; // 0xA650 - permanent BLB header allocation pointer
} EngineContext;

#ifdef NON_MATCHING
// This code compiles and is validated by the IDE, but doesn't byte-match
// due to GP-relative addressing differences. Use -DNON_MATCHING to build.
void LoadBLBHeader(GameState *state) {
    s32 blbHeaderBuffer;
    EngineContext *engine;

    func_8007BFB8();
    
    D_800A5960 = state;  // GP-relative store - can't match
    
    state->unk114 = 0;
    state->unk130 = 0;
    state->headerBufferSize = 0x10000;
    state->unk28 = 0;
    state->unk104 = 0;
    state->unk108 = 0;
    state->unk110 = 0;
    state->unk64 = 0;
    state->unk66 = 0;
    
    state->headerMagic = 0x01234567;
    engine = (EngineContext *)D_800A5954;
    blbHeaderBuffer = (s32)engine->blbHeaderBufferBase;
    D_801FC400 = 0x01234567;
    state->headerBuffer = blbHeaderBuffer;
    state->headerMirror = blbHeaderBuffer + 0x1000;
    
    CdBLB_ReadSectors(0, 2, (void *)state->headerBuffer);
    
    func_8007A1BC((void *)state->cdRequest, state->headerBuffer, &func_80020848);
    func_8007BB00(0, (void *)state->cdRequest);
}
#else
INCLUDE_ASM("asm/pal/nonmatchings/LoadBLBHeader", LoadBLBHeader);
#endif
