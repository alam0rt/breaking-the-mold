/* =============================================================================
 * render_core.c  --  functional-C render core for the PC port (TARGET_PC)
 * =============================================================================
 * PC replacements for the gfx.c render-architecture functions that the boot +
 * frame loop depend on. On the byte-match build these are `INCLUDE_ASM` in
 * src/gfx.c; here they are strong C definitions that override the generated weak
 * stubs. Behaviour is transcribed from the disassembly
 * (asm/nonmatchings/gfx/{InitGraphicsSystem,SwapBuffersAndClearOT,
 * ClearOrderingTables}.s) with the double-buffer + ordering-table layout mapped
 * onto the OpenGL backend (port/spec/gpu_gl.c).
 *
 * ---- HEAP / GPU-CONTEXT LAYOUT (verified vs prim_records.h / spracc_records.h)
 *   base+0x0     s16  screen width  (0x140 = 320)
 *   base+0x2     s16  screen height (0x100 = 256)
 *   base+0x4     GpuContext  buffer 0  (draw env @ +0x0, disp env @ +0x5C,
 *                            ot pointer @ +0x70, prim pools @ +0x74..)
 *   base+0x5044  GpuContext  buffer 1  (stride 0x5040)
 *   base+0xA084  GpuContext* gpu        (current back buffer)
 *   base+0xA088  u8          frameIdx   (0/1)
 *   base+0xA308  s32         last VSync value
 *
 * ---- ORDERING TABLE (PC-native): gpu->ot points to a single-word list head
 * (one per buffer, s_ot_head[]). The game's AddPrim links primitives into that
 * head (via gpu->ot, reached as otContext->orderingTable); SwapBuffersAndClearOT
 * walks it with DrawOTag (the "present"). Both agree on the 32-bit-tag scheme in
 * gpu_gl.c. See docs/plans/pc-port.md CP-2.1 render-core note.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include "port_runtime.h"
#include "port_hal.h"

#include <stdint.h>

/* Scratch buffer returned by the GetFrameReadyFlag *function entry* (see below).
 * InitGraphicsSystem aligns it up and stores it at base+0xA650 as the BLB header
 * read destination (LoadBLBHeader reads 0x1000 bytes into it, and the level
 * loader carves a ~1 MB sub-heap starting just after it -- see the InitHeapConfig
 * call in InitializeAndLoadLevel: `size = 0x801FC000 - (buf+off)` clamps to
 * 0xFFFF0). On PSX this is a fixed RAM address inside the flat 2 MB map; on PC it
 * must be a real region large enough to hold the staged level data + that
 * sub-heap, so it is sized 4 MB (not a few KB). */
u8 D_800AE3E0[4 * 1024 * 1024] asm("D_800AE3E0");

/* PSY-Q surface (implemented in port/spec/gpu_gl.c + bios.c). */
extern void ResetGraph(int mode);
extern void PutDrawEnv(void *env);
extern void PutDispEnv(void *env);
extern void *SetDefDrawEnv(void *env, int x, int y, int w, int h);
extern void *SetDefDispEnv(void *env, int x, int y, int w, int h);
extern void DrawOTag(u_int *ot);
extern void ClearOTag(u_int *ot, int n);
extern int  VSync(int mode);
extern void *VSyncCallback(void (*f)(void));
extern void InitVRAMSlotTable(void *base, int a1, int a2);

/* Heap / GPU-context offsets. */
#define GPU_CTX0_OFF     0x0004
#define GPU_CTX1_OFF     0x5044
#define GPU_OT_OFF       0x0070    /* GpuContext.ot                          */
#define GPU_DISPENV_OFF  0x005C    /* GpuContext disp env                    */
#define GG_GPU_OFF       0xA084    /* GameGlobals.gpu (current context ptr)  */
#define GG_FRAMEIDX_OFF  0xA088    /* GameGlobals.frameIdx                   */
#define GG_VSYNC_OFF     0xA308    /* last VSync value                       */
#define GG_BLBBUF_OFF    0xA650    /* aligned BLB-header read buffer pointer  */
/* Prim-pool count offsets within a GpuContext (from prim_records.h). */
static const u32 k_pool_count_off[7] = {
    0x1014, 0x1658, 0x25FC, 0x2DD0, 0x3734, 0x4228, 0x503C
};

/* One ordering-table list head per buffer. gpu->ot points here; AddPrim links
 * primitives in, DrawOTag walks them, ClearOTag empties them. */
static u_int s_ot_head[2];

static u8 *ctx_ot(void *base, u32 ctx_off) {
    /* Return the pointer stored at (base + ctx_off + 0x70) == GpuContext.ot. */
    return *(u8 **)((u8 *)base + ctx_off + GPU_OT_OFF);
}

/* GetFrameReadyFlag @ 0x800131F0 is a DUAL-ENTRY function:
 *   - entry (0x800131F0): returns &D_800AE3E0 -- a scratch BUFFER POINTER
 *     (`lui/addiu` of the symbol). This is what InitGraphicsSystem calls.
 *   - entry+0xC (0x800131FC): `lbu g_FrameReady` -- returns the frame-ready
 *     byte; this is the VSync-ISR getter (served on PC by TriggerBufferSwapIfReady
 *     reading g_FrameReady directly).
 * The gfx.c plate comment describes only the +0xC behaviour; the function ENTRY
 * returns the buffer pointer, so that is what this C definition returns. */
u32 GetFrameReadyFlag(void) {
    return (u32)(uintptr_t)&D_800AE3E0[0];
}

/* SetGraphDebug: PSY-Q debug-level toggle -- no-op on PC. */
void SetGraphDebug(int level) {
    (void)level;
}

/* TriggerBufferSwapIfReady @ 0x80013200: the VSync callback body. If a frame is
 * ready and no swap is in flight, latch the swap and perform it. Fired once per
 * host frame by port_bios_fire_vsync(). */
void SwapBuffersAndClearOT(void *base);   /* fwd */

void TriggerBufferSwapIfReady(void) {
    if (g_FrameReady && !g_SwapInFlight) {
        g_SwapInFlight = 1;
        g_FrameReady = 0;
        SwapBuffersAndClearOT(g_pBlbHeapBase);
    }
}

/* SwapBuffersAndClearOT @ 0x80013554: present the finished buffer (DrawOTag),
 * flip to the other buffer, clear its OT + reset its prim-pool bump counters. */
void SwapBuffersAndClearOT(void *base) {
    u8 *b = (u8 *)base;
    u8 *cur = *(u8 **)(b + GG_GPU_OFF);
    s32 vsync;
    u8 nextIdx;
    u8 *next;
    int i;

    g_SwapInFlight = 1;
    vsync = VSync(-1);

    PutDispEnv(cur + GPU_DISPENV_OFF);
    PutDrawEnv(cur);
    DrawOTag((u_int *)*(u8 **)(cur + GPU_OT_OFF));   /* present the OT */

    nextIdx = (u8)((*(u8 *)(b + GG_FRAMEIDX_OFF) + 1) & 1);
    *(u8 *)(b + GG_FRAMEIDX_OFF) = nextIdx;
    next = b + (nextIdx ? GPU_CTX1_OFF : GPU_CTX0_OFF);
    *(u8 **)(b + GG_GPU_OFF) = next;

    ClearOTag((u_int *)*(u8 **)(next + GPU_OT_OFF), 1);
    for (i = 0; i < 7; i++) {
        *(s16 *)(next + k_pool_count_off[i]) = 0;
    }
    *(s32 *)(b + GG_VSYNC_OFF) = vsync;
}

/* ClearOrderingTables @ 0x800134B8: reset both buffers' ordering tables (used at
 * level (re)load via gstate.c). */
void ClearOrderingTables(void *base) {
    g_FrameReady = 0;
    g_SwapInFlight = 1;
    ClearOTag((u_int *)ctx_ot(base, GPU_CTX0_OFF), 1);
    ClearOTag((u_int *)ctx_ot(base, GPU_CTX1_OFF), 1);
}

/* InitGraphicsSystem @ 0x80013268: bring up the double-buffered GPU context,
 * install the swap-on-vsync ISR, init VRAM slots, prime both buffers. */
void InitGraphicsSystem(void *base) {
    u8 *b = (u8 *)base;
    u8 *ctx0 = b + GPU_CTX0_OFF;
    u8 *ctx1 = b + GPU_CTX1_OFF;

    /* BLB-header read buffer: align the GetFrameReadyFlag scratch pointer up to
     * 16 and stash it at base+0xA650 (LoadBLBHeader reads it via gs+0x40). */
    *(s32 *)(b + GG_BLBBUF_OFF) = (GetFrameReadyFlag() + 0xF) & ~0xF;

    ResetGraph(0);
    SetGraphDebug(0);
    VSyncCallback(TriggerBufferSwapIfReady);

    /* Screen dimensions (PAL 320x256 framebuffer). */
    *(s16 *)(b + 0x0) = 0x140;
    *(s16 *)(b + 0x2) = 0x100;

    /* Point each buffer's ot field at its list head. */
    *(u8 **)(ctx0 + GPU_OT_OFF) = (u8 *)&s_ot_head[0];
    *(u8 **)(ctx1 + GPU_OT_OFF) = (u8 *)&s_ot_head[1];

    /* Double-buffered draw/display environments. */
    SetDefDrawEnv(ctx0, 0, 0, 0x140, 0x100);
    SetDefDrawEnv(ctx1, 0, 0, 0x140, 0x100);
    SetDefDispEnv(ctx0 + GPU_DISPENV_OFF, 0, 0, 0x140, 0x100);
    SetDefDispEnv(ctx1 + GPU_DISPENV_OFF, 0, 0x100, 0x140, 0x100);

    /* Current back buffer = buffer 0. */
    *(u8 **)(b + GG_GPU_OFF) = ctx0;
    *(u8 *)(b + GG_FRAMEIDX_OFF) = 0;

    ClearOTag((u_int *)&s_ot_head[0], 1);
    ClearOTag((u_int *)&s_ot_head[1], 1);

    InitVRAMSlotTable(base, 0, 1);

    SwapBuffersAndClearOT(base);
    SwapBuffersAndClearOT(base);
}
