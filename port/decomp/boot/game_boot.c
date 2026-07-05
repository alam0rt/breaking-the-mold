/* =============================================================================
 * game_boot.c  --  PC-port game boot spine (functional C, TARGET_PC)
 * =============================================================================
 * Functional-C reconstruction of the PSX `main` boot sequence + frame body
 * (asm/nonmatchings/main/main.s @ 0x800828B0), based on the working draft in
 * docs/analysis/decomp-drafts/main.c. This is CP-2.1 of the PC port: the boot
 * spine that host_main -> port_game_main drives.
 *
 * Split into two entry points so the host frame loop (port_boot.c) can weave in
 * SDL present / input / pacing between iterations:
 *   port_game_boot_init()   -- the one-time boot prologue
 *   port_game_boot_frame()  -- one iteration of the main frame loop
 *
 * Boot callees resolve as: HAL replacements (port/spec/*.c) for the PSY-Q
 * surface + small gfx helpers; matched C for the BLB accessors (blbacc.c) and
 * list drivers (blb.c); and -- for functions still only in asm -- the generated
 * weak stubs, which abort with the exact name when first reached. That abort is
 * the work queue: convert the named function, rebuild, get one step further.
 *
 * See docs/plans/pc-port.md CP-2.1..CP-2.6.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include "port_runtime.h"
#include "port_hal.h"

/* ---- boot globals not in globals.h (raw absolute-address aliases) --------- */
extern u8   g_GameStateBase[] asm("g_GameStateBase");     /* 0x1A0 GameState    */
extern void *PLAYER_STATE_DATA asm("D_800A597C");         /* -> player state    */
extern void *g_pInputStateA asm("D_800A5964");            /* pad-1 input state  */
extern void *g_pInputStateB asm("D_800A5968");            /* pad-2 input state  */
extern s16   D_800A596C asm("D_800A596C");
extern s16   D_800A596E asm("D_800A596E");
extern s16   D_800A5970 asm("D_800A5970");
extern void *g_LevelNameTable[] asm("D_8009DE08");        /* level-name ptr tbl */
extern u16   D_800A60BC asm("D_800A60BC");                /* level count mirror */
extern u16   D_800A60BE asm("D_800A60BE");                /* asset count mirror */

/* PC-port backing for the respawn/player-state struct.
 * On PSX, D_800A597C is an .sdata pointer to a .bss PlayerState struct that the
 * boot's initPlayerState() stamps with default {level=1, stage=1, ...}. The
 * port's weak-zero global backing leaves the pointer NULL, so initPlayerState
 * early-returns (it guards `if (!p) return;`) and every later PlayerState access
 * (InitGameState's RESPAWN_PLAYER_STATE[0]=... writes, SpawnPlayerAndEntities,
 * pickups, bosses) would dereference NULL. Seed the pointer at a zeroed static
 * struct before the first use so the real init + all downstream reads work. */
static u8 s_respawnPlayerState[0x400];

/* PC-port seeding for the layer-render-slot / sprite-frame-cache table.
 * On PSX, D_800A595C is an .sdata POINTER whose init value is &D_8009AE58 -- a
 * zero-filled 0x1E0-byte .data table of 20 x 0x18-byte slots. It is used both as
 * the sprite-frame cache (LoadSpriteFramesToVRAM/LoadSpriteHashArrayToVRAM index
 * it by *0x18) and the layer-render-slot table (FindLayerSlotByEntityPointer).
 * The port's weak-zero backing leaves the pointer NULL, so seed it at the
 * (weak-backed) D_8009AE58 storage before the first level load. */
extern u8    D_8009AE58[] asm("D_8009AE58");           /* render-slot table  */
extern void *g_LayerRenderSlots asm("D_800A595C");    /* -> D_8009AE58       */

/* ---- boot/frame callees (HAL, matched C, or weak-stub-until-converted) ---- */
extern void SsUtReverbOn(void);
extern void ResetCallback(void);
extern void LoadGameAssetLocations(void);
extern void InitGraphicsSystem(void *base);
extern void PadInit(int mode);
extern void InitGeom(void);
extern int  SetDispMask(int on);
extern void FntLoad(int tx, int ty);
extern int  FntOpen(int x, int y, int w, int h, int isbg, int n);
extern void SetDumpFnt(int id);
extern void initPlayerState(void *state);
extern void InitGameState(void *gs, void *input);
extern u8   GetLevelCount(void *ctx);
extern void *getLevelName(void *ctx, u8 index);
extern u8   GetAssetCount(void *ctx);
extern void *GetMovieEntryByIndex(void *ctx, u8 index);

extern void TickCDStreamBuffer(void);
extern u_long PadRead(int id);
extern void UpdateInputState(void *input, u16 raw);
extern void EntityTickLoop(void *gs);
extern void WaitForVBlankIfNeeded(void *base);
extern void RenderEntities(void *gs);
extern void DrawSync(int mode);
extern int  VSync(int mode);
extern void ProcessDebugMenuInput(void);
extern void FlushDebugFontAndEndFrame(void *base);

/* GameState is accessed here mostly by raw offset (matching the draft) to avoid
 * coupling to the full struct header while the layout is still being confirmed.
 *   +0x00 s16 event_marker      +0x02 s16 event_table_offset
 *   +0x04 event_callback / table pointer
 *   +0x84 LevelDataContext (embedded)
 *   postRenderCallbackContext pointer (see game_state.h) */
#define GS_LEVEL_CTX(gs) ((void *)((u8 *)(gs) + 0x84))

void port_game_boot_init(void) {
    u8 i;
    void *gs;
    void *ctx;

    SsUtReverbOn();
    ResetCallback();
    LoadGameAssetLocations();
    InitGraphicsSystem(g_pBlbHeapBase);

    g_pGameState = (GameState *)g_GameStateBase;
    PadInit(0);
    InitGeom();
    SetDispMask(1);
    FntLoad(0x3C0, 0x100);
    SetDumpFnt(FntOpen(0x10, 0x20, 0x120, 0xC8, 0, 0x200));
    PLAYER_STATE_DATA = s_respawnPlayerState;   /* give D_800A597C a real target */
    g_LayerRenderSlots = D_8009AE58;            /* give D_800A595C a real target */
    initPlayerState(PLAYER_STATE_DATA);
    InitGameState(g_GameStateBase, g_pInputStateA);

    /* Build the debug/level-select name tables from the loaded BLB TOC. */
    gs = g_pGameState;
    ctx = GS_LEVEL_CTX(gs);
    for (i = 0; (u8)i < GetLevelCount(ctx); i++) {
        g_LevelNameTable[(u8)i] = getLevelName(ctx, (u8)i);
        g_TotalMenuItems++;
        D_800A60BC++;
    }
    for (i = 0; (u8)i < GetAssetCount(ctx); i++) {
        g_DebugMenuItemNames[g_TotalMenuItems] =
            (char *)GetMovieEntryByIndex(ctx, (u8)i);
        g_TotalMenuItems++;
        D_800A60BE++;
    }

    D_800A596C = 0x40;
    D_800A596E = 0x20;
    D_800A5970 = 0x80;
    g_pCurrentInputState = (InputState *)g_pInputStateA;
}

void port_game_boot_frame(void) {
    void *gs = g_pGameState;
    s16 modeId;

    TickCDStreamBuffer();

    {
        u_long padBits = PadRead(1);
        UpdateInputState(g_pInputStateA, (u16)padBits);
        UpdateInputState(g_pInputStateB, (u16)(padBits >> 16));
    }

    /* GameMode FSM dispatch (event_marker/event_callback @ +0x00/+0x04). */
    modeId = ((s16 *)gs)[1];
    if (modeId != 0) {
        s32 argOff = 0;
        void (*cb)(void *);
        s32 baseOff;
        if (modeId > 0) {
            s16 tblOff = ((s16 *)gs)[2];
            s32 *table = *(s32 **)((u8 *)gs + tblOff);
            argOff = table[(modeId - 1) * 2];
            cb = (void (*)(void *))table[(modeId - 1) * 2 + 1];
        } else {
            cb = (void (*)(void *))((s32 *)gs)[1];
        }
        baseOff = ((s16 *)gs)[0];
        if (modeId > 0) {
            baseOff += (s16)argOff;
        }
        cb((u8 *)gs + baseOff);
    }

    EntityTickLoop(gs);
    WaitForVBlankIfNeeded(g_pBlbHeapBase);
    RenderEntities(gs);
    DrawSync(0);

    /* postRenderCallbackContext dispatch: (**(ctx+0x1C))(gs + *(s16*)(ctx+0x18)) */
    {
        void *postCtx = *(void **)((u8 *)gs + 0x0C);
        void (*fn)(void *) = *(void (**)(void *))((u8 *)postCtx + 0x1C);
        fn((u8 *)gs + *(s16 *)((u8 *)postCtx + 0x18));
    }
    DrawSync(0);

    if (g_GameFlags & 0x6) {
        s32 target = VSync(-1) + 2;
        while ((u32)VSync(-1) < (u32)target) {
        }
    }

    ProcessDebugMenuInput();
    FlushDebugFontAndEndFrame(g_pBlbHeapBase);
}
