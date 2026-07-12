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
 * pickups, bosses) would dereference NULL. On PS1 the .sdata initializer
 * points it at the .bss struct @ 0x8009B1D8; the port now seeds the SAME
 * address inside the PSX-mirror arena (port_psx2host), so the struct's live
 * contents are dumpable and pointers to it are bit-identical to PS1. */
#define PSX_PLAYER_STATE 0x8009B1D8u

/* Storage for the friendly-named player-state pointer from include/globals.h
 * (g_pPlayerState, PSX .sdata @ 0x800A597C). The port never defined it, so
 * code that relies on the header (e.g. CreateMenuEntities' HUD, reading
 * g_pPlayerState->orb_count) would fail to link. Define it once here and seed
 * it to the same PlayerState struct as the D_800A597C asm-alias users below --
 * both names then observe identical player data. */
PlayerState *g_pPlayerState;

/* PC-port backing for the two controller InputState structs.
 * On PSX, D_800A5964 (g_pInputStateA) and D_800A5968 (g_pInputStateB) are .sdata
 * pointers to .bss InputState structs (0x14 bytes each). Weak-zero backing
 * leaves them NULL, so the very first frame's UpdateInputState(g_pInputStateA,
 * ...) dereferences NULL. On PS1 the .sdata initializers point them at the
 * .bss structs @ 0x8009B14C / 0x8009B160 (0x14 bytes each); the port seeds
 * the SAME arena addresses so the input-state contents live in RAM dumps and
 * the GameState's input_state_ptr (+0x140) is bit-identical to PS1. */
#define PSX_INPUT_STATE_A 0x8009B14Cu
#define PSX_INPUT_STATE_B 0x8009B160u

/* PC-port seeding for the layer-render-slot / sprite-frame-cache table.
 * On PSX, D_800A595C is an .sdata POINTER whose init value is &D_8009AE58 -- a
 * zero-filled 0x1E0-byte .data table of 20 x 0x18-byte slots. It is used both as
 * the sprite-frame cache (LoadSpriteFramesToVRAM/LoadSpriteHashArrayToVRAM index
 * it by *0x18) and the layer-render-slot table (FindLayerSlotByEntityPointer).
 * The port's weak-zero backing leaves the pointer NULL, so seed it at the
 * (weak-backed) D_8009AE58 storage before the first level load. */
extern u8    D_8009AE58[] asm("D_8009AE58");           /* render-slot table  */
extern void *g_LayerRenderSlots asm("D_800A595C");    /* -> D_8009AE58       */

/* PC-port link backing for the gfx.c sdata-island pointer initializers.
 * When the 0x800A5954 engine-globals island was migrated into src/gfx.c as
 * natural C, its pointer members took the ROM initializers &D_800907EC
 * (g_pBlbHeapBase), &D_8009B14C (g_pInputStateA) and &D_8009B160
 * (g_pInputStateB) -- PSX .bss/.data addresses that don't exist on PC. These
 * asm-aliased externs make gen_port_stubs.py emit weak zero backing so the
 * native link resolves; the live pointers are re-seeded at boot below
 * (input states -> s_inputStateA/s_inputStateB; heap base via InitGraphicsSystem). */
extern u8    D_800907EC[] asm("D_800907EC");           /* g_pBlbHeapBase init */
extern u8    D_8009B14C[] asm("D_8009B14C");           /* g_pInputStateA init */
extern u8    D_8009B160[] asm("D_8009B160");           /* g_pInputStateB init */

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
extern void ConstructStaticGameState(void);
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

/* demo-playback path (PORT_DEMO): matched C (blbacc.c) + port conversions
 * (decomp/blb/EnableDemoPlaybackMode.c) + PSX-LCG srand (spec/bios.c). */
extern u8  *GetDemoDataPtr(void *ctx);
extern u8   GetCurrentLevelAssetIndex(void *ctx);
extern void InitEntityDataPointers(void *input, void *dataBase);
extern void EnableDemoPlaybackMode(void *input, u8 enable);
extern void srand(unsigned int seed);
/* demo banner spawn (SetupAndStartLevel demo branch parity) */
extern u8  *AllocateFromHeap(void *heap, u32 size, u32 count, u8 flag);
extern Entity *InitEntitySprite(Entity *entity, u32 spriteId, s32 z, s16 x,
                                s16 y, s32 flags);
extern long GetTPage(int tp, int abr, int x, int y);
extern void AddEntityToSortedRenderList(void *gs, void *ent);
extern void GetWorldPositionX();
extern void GetWorldPositionY();
extern char *getenv(const char *name);
extern long strtol(const char *s, char **end, int base);

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
    /* PSX static-ctor equivalent: installs the GameState vtable (D_80012100)
     * at gs+0x18. Its +0x1C slot is EntityRemoval, the frame loop's
     * post-render pass that flushes dirty entity textures/CLUTs to VRAM --
     * without this, entity sprites never upload and render invisible. */
    ConstructStaticGameState();
    PadInit(0);
    InitGeom();
    SetDispMask(1);
    FntLoad(0x3C0, 0x100);
    SetDumpFnt(FntOpen(0x10, 0x20, 0x120, 0xC8, 0, 0x200));
    PLAYER_STATE_DATA = port_psx2host(PSX_PLAYER_STATE);  /* D_800A597C target */
    g_pPlayerState = (PlayerState *)PLAYER_STATE_DATA;    /* friendly-name alias */
    g_pInputStateA = port_psx2host(PSX_INPUT_STATE_A);    /* D_800A5964 target */
    g_pInputStateB = port_psx2host(PSX_INPUT_STATE_B);    /* D_800A5968 target */
    g_LayerRenderSlots = D_8009AE58;            /* give D_800A595C a real target */
    initPlayerState(PLAYER_STATE_DATA);

    /* PORT_LEVEL / PORT_STAGE: override the boot level before InitGameState.
     * The boot-time InitializeAndLoadLevel path seeds its sequence-seek from
     * RESPAWN_PLAYER_STATE[0] (level index) and [1] (stage/mode). Needed e.g.
     * to boot straight into one of the demo-bearing levels (asset 700). */
    {
        const char *lv = getenv("PORT_LEVEL");
        const char *st = getenv("PORT_STAGE");
        if (lv && *lv) ((u8 *)PLAYER_STATE_DATA)[0] = (u8)strtol(lv, NULL, 0);
        if (st && *st) ((u8 *)PLAYER_STATE_DATA)[1] = (u8)strtol(st, NULL, 0);
    }

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

    /* Log which level the boot sequence actually landed on (observable check
     * for PORT_LEVEL -- without it the sequence stays at its start, the MENU). */
    {
        u8 lvlIdx = GetCurrentLevelAssetIndex(ctx);
        port_log("boot: loaded level %u '%s'", lvlIdx,
                 (lvlIdx < D_800A60BC && g_LevelNameTable[lvlIdx] != NULL)
                     ? (const char *)g_LevelNameTable[lvlIdx] : "?");
    }

}

/* PORT_DEMO=1: replicate SetupAndStartLevel's attract-demo branch
 * (gs->demo_return_flag path, asm @0x8007DB44..0x8007DC58) for the port's
 * boot-time level load, which bypasses SetupAndStartLevel. Called by
 * InitGameState BETWEEN the level load and SpawnPlayerAndEntities -- the
 * exact PS1 slot. Position is load-bearing twice over: the banner's 0x100
 * heap allocation precedes the player's on PS1 (running it after boot-init
 * left every later heap object 0x390 below its PS1 address), and
 * SpawnPlayerAndEntities draws rand() (entity baseRGB), so srand(1) must
 * precede it for RNG-stream parity. With bios.c's PSX-LCG rand this makes a
 * demo run deterministic and directly diffable against a PS1 reference trace
 * (make port-trace / diffdb --offset). Requires a level that carries asset
 * 700 (replay data): BOIL, BRG1, CAVE, FOOD, GLID, MENU, SCIE, TMPL, WEED --
 * combine with PORT_LEVEL. */
void port_demo_prestart(void *arg0) {
    u8 *gs = (u8 *)arg0;
    void *ctx = (void *)(gs + 0x84);
    u8 *demoData;

    if (!getenv("PORT_DEMO")) {
        return;
    }
    demoData = GetDemoDataPtr(ctx);
    if (demoData == NULL) {
        port_log("demo: no asset-700 replay data in this level; PORT_DEMO ignored");
        return;
    }

    srand(1);
    InitEntityDataPointers(g_pInputStateA, demoData);
    EnableDemoPlaybackMode(g_pInputStateA, 1);

    /* Frame-phase alignment: on PS1, SetupAndStartLevel enables playback
     * MID-frame -- after that frame's UpdateInputState already ran -- so the
     * first consuming UpdateInputState is one frame later than the port's
     * (boot-init runs before frame 1). Extend the first RLE entry by one
     * frame so every input flip lands on the same frame as the PS1 trace
     * (measured: without this, the demo's first directional input -- the
     * walk after the spawn landing -- fired one frame early, a constant
     * 1-frame/3px lead for the whole run). */
    ((InputState *)g_pInputStateA)->playback_timer += 1;

    /* demo_return_flag: set before SetupAndStartLevel on PS1 (attract
     * sequence); InitGameState just cleared it, so restore for parity. */
    gs[0x152] = 1;

    /* the "demo" overlay banner (sprite 0x28C080DF, z 0x7530, at 160/32) */
    {
        u8 *e = AllocateFromHeap(g_pBlbHeapBase, 0x100, 1, 0);
        u8 *prim;
        InitEntitySprite((Entity *)e, 0x28C080DF, 0x7530, 0xA0, 0x20, 0);
        *(u32 *)(e + 0x24) = 0xFFFF0000u;
        *(void **)(e + 0x28) = (void *)GetWorldPositionX;
        *(u32 *)(e + 0x2C) = 0xFFFF0000u;
        *(void **)(e + 0x30) = (void *)GetWorldPositionY;
        prim = *(u8 **)(e + 0x34);
        *(s16 *)(prim + 0x24) =
            (s16)GetTPage(prim[0x32], 1,
                          *(s16 *)(prim + 0x10) & ~0x3F,
                          *(s16 *)(prim + 0x12) & ~0xFF);
        AddEntityToSortedRenderList(arg0, e);
    }
    port_log("demo: playback on (%u RLE entries), banner spawned",
             *(u16 *)demoData);
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

    /* PORT_TRACE_GS=1: log GameState transition-flag changes (debug). */
    if (getenv("PORT_TRACE_GS")) {
        static u8 prev[8];
        u8 cur[8];
        cur[0] = *((u8 *)gs + 0x146); cur[1] = *((u8 *)gs + 0x147);
        cur[2] = *((u8 *)gs + 0x148); cur[3] = *((u8 *)gs + 0x150);
        cur[4] = *((u8 *)gs + 0x151); cur[5] = *((u8 *)gs + 0x149);
        cur[6] = *((u8 *)gs + 0x14A); cur[7] = *((u8 *)gs + 0x14B);
        if (memcmp(prev, cur, 8) != 0) {
            port_log("gs: adv=%d next=%d direct=%d menu=%d fade=%d chk=%d/%d/%d",
                     cur[0], cur[1], cur[2], cur[3], cur[4], cur[5], cur[6], cur[7]);
            memcpy(prev, cur, 8);
        }
    }
    /* PORT_TRACE_PLAYER=1: per-frame player-entity dump (debug). */
    if (getenv("PORT_TRACE_PLAYER")) {
        static unsigned dbg_frame;
        u8 *pl = *(u8 **)((u8 *)gs + 0x2C);
        dbg_frame++;
        if (pl != NULL) {
            u8 *in = *(u8 **)(pl + 0x100);
            port_log("plr f=%u x=%d y=%d face=%d/%d vx=%d spd=%d conv=%d held=%04x prs=%04x tick=%p phys=%p",
                     dbg_frame, *(s16 *)(pl + 0x68), *(s16 *)(pl + 0x6A),
                     pl[0x74], pl[0x75], *(s32 *)(pl + 0xB4), *(s32 *)(pl + 0x124),
                     *(s16 *)(pl + 0x160),
                     in ? *(u16 *)(in + 0) : 0, in ? *(u16 *)(in + 2) : 0,
                     *(void **)(pl + 0x04), *(void **)(pl + 0x20));
        }
    }
    EntityTickLoop(gs);
    WaitForVBlankIfNeeded(g_pBlbHeapBase);
    RenderEntities(gs);
    DrawSync(0);

    /* postRenderCallbackContext dispatch: (**(ctx+0x1C))(gs + *(s16*)(ctx+0x18)).
     * On PSX gs->postRenderCallbackContext is always installed during level
     * setup; the port has not yet converted that setter, so it can still be NULL
     * here. Guard the dispatch (the callback is a render-finalize step, not
     * required for the frame to composite) until the setter is ported. */
    {
        void *postCtx = *(void **)((u8 *)gs + 0x18);   /* GameState.postRenderCallbackContext */
        if (postCtx != NULL) {
            void (*fn)(void *) = *(void (**)(void *))((u8 *)postCtx + 0x1C);
            fn((u8 *)gs + *(s16 *)((u8 *)postCtx + 0x18));
        }
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
