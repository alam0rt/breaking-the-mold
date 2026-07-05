/* =============================================================================
 * hal_stubs.c  --  Phase-0 aborting bodies for the whole PSY-Q HAL surface
 * =============================================================================
 * Every prototype declared in psyq_pc.h gets a strong definition here that
 * routes through port_stub() (abort, or log+continue when g_port_stub_nonfatal).
 * This lets the port LINK before any real backend exists: booting reaches the
 * first HAL call and stops with the exact symbol name -- the Phase-0 exit gate.
 *
 * In Phase 1 the real backends (gpu_gl.c, gte.c, spu_sdl.c, cd_files.c,
 * pad_sdl.c, bios.c) provide strong definitions for their slices; delete the
 * corresponding stub here (or let the linker's one-definition rule guide the
 * split -- these are strong, so each function must live in exactly one place).
 * ========================================================================== */
#include "psyq_pc.h"
#include "port_runtime.h"

/* Every definition below is WEAK: the moment a real backend (gpu_gl.c, ...) or a
 * matched decomp body (src/libs/libcd.c already defines ResetCallback, PadRead,
 * ... in C) provides a strong definition, it silently overrides the abort stub.
 * This removes any need to hand-maintain an exclusion list as functions land. */
#pragma weak GetTPage
#pragma weak GetClut
#pragma weak DrawSync
#pragma weak VSync
#pragma weak VSyncCallback
#pragma weak ResetGraph
#pragma weak SetDispMask
#pragma weak PutDrawEnv
#pragma weak PutDispEnv
#pragma weak SetDefDrawEnv
#pragma weak SetDefDispEnv
#pragma weak ClearOTag
#pragma weak DrawOTag
#pragma weak AddPrim
#pragma weak LoadImage
#pragma weak StoreImage
#pragma weak MoveImage
#pragma weak SetDrawTPage
#pragma weak LoadTPage
#pragma weak LoadClut
#pragma weak SetVideoMode
#pragma weak GetVideoMode
#pragma weak FntLoad
#pragma weak FntOpen
#pragma weak FntPrint
#pragma weak FntFlush
#pragma weak SetDumpFnt
#pragma weak InitGeom
#pragma weak RotTransPers
#pragma weak ApplyMatrixLV
#pragma weak SetRotMatrix
#pragma weak SetTransMatrix
#pragma weak SpuSetKey
#pragma weak SpuSetVoicePitch
#pragma weak SpuGetVoicePitch
#pragma weak SpuSetVoiceVolume
#pragma weak SpuSetCommonCDVolume
#pragma weak SpuVoiceRegisters
#pragma weak SsUtReverbOn
#pragma weak CdInit
#pragma weak CdControl
#pragma weak CdControlB
#pragma weak CdControlF
#pragma weak CdRead
#pragma weak CdReadSync
#pragma weak CdSync
#pragma weak CdSearchFile
#pragma weak CdGetSector
#pragma weak CdIntToPos
#pragma weak CdPosToInt
#pragma weak CdDataCallback
#pragma weak CdReadCallback
#pragma weak CdReadyCallback
#pragma weak PadInit
#pragma weak InitPAD
#pragma weak StartPAD
#pragma weak StopPAD
#pragma weak ChangeClearPAD
#pragma weak PadRead
#pragma weak DeliverEvent
#pragma weak InterruptCallback
#pragma weak DMACallback
#pragma weak ResetCallback
#pragma weak EnterCriticalSection
#pragma weak ExitCriticalSection

#define STUB(name) port_stub(#name)

/* ---- GPU ----------------------------------------------------------------- */
long   GetTPage(int tp, int abr, int x, int y) { (void)tp;(void)abr;(void)x;(void)y; STUB(GetTPage); return 0; }
u_short GetClut(int x, int y) { (void)x;(void)y; STUB(GetClut); return 0; }
void   DrawSync(int mode) { (void)mode; STUB(DrawSync); }
int    VSync(int mode) { (void)mode; STUB(VSync); return 0; }
void  *VSyncCallback(void (*f)(void)) { (void)f; STUB(VSyncCallback); return 0; }
void   ResetGraph(int mode) { (void)mode; STUB(ResetGraph); }
int    SetDispMask(int mask) { (void)mask; STUB(SetDispMask); return 0; }
void   PutDrawEnv(DRAWENV *env) { (void)env; STUB(PutDrawEnv); }
void   PutDispEnv(DISPENV *env) { (void)env; STUB(PutDispEnv); }
DRAWENV *SetDefDrawEnv(DRAWENV *env, int x, int y, int w, int h) { (void)x;(void)y;(void)w;(void)h; STUB(SetDefDrawEnv); return env; }
DISPENV *SetDefDispEnv(DISPENV *env, int x, int y, int w, int h) { (void)x;(void)y;(void)w;(void)h; STUB(SetDefDispEnv); return env; }
void   ClearOTag(u_int *ot, int n) { (void)ot;(void)n; STUB(ClearOTag); }
void   DrawOTag(u_int *ot) { (void)ot; STUB(DrawOTag); }
void   AddPrim(u_char *ot, u_char *prim) { (void)ot;(void)prim; STUB(AddPrim); }
int    LoadImage(RECT *rect, u_int *data) { (void)rect;(void)data; STUB(LoadImage); return 0; }
int    StoreImage(RECT *rect, u_int *data) { (void)rect;(void)data; STUB(StoreImage); return 0; }
int    MoveImage(RECT *rect, int x, int y) { (void)rect;(void)x;(void)y; STUB(MoveImage); return 0; }
DR_TPAGE *SetDrawTPage(DR_TPAGE *p, int dfe, int dtd, int tpage) { (void)dfe;(void)dtd;(void)tpage; STUB(SetDrawTPage); return p; }
int    LoadTPage(RECT *rect, int tp, int abr, int x, int y, int w, int h) { (void)rect;(void)tp;(void)abr;(void)x;(void)y;(void)w;(void)h; STUB(LoadTPage); return 0; }
int    LoadClut(u_int *clut, int x, int y) { (void)clut;(void)x;(void)y; STUB(LoadClut); return 0; }
int    SetVideoMode(int mode) { (void)mode; STUB(SetVideoMode); return 0; }
int    GetVideoMode(void) { STUB(GetVideoMode); return 0; }

/* ---- Debug font (Fnt) ---------------------------------------------------- */
void   FntLoad(int x, int y) { (void)x;(void)y; STUB(FntLoad); }
int    FntOpen(int x, int y, int w, int h, int isbg, int n) { (void)x;(void)y;(void)w;(void)h;(void)isbg;(void)n; STUB(FntOpen); return 0; }
int    FntPrint(int id, const char *fmt, ...) { (void)id;(void)fmt; STUB(FntPrint); return 0; }
u_int *FntFlush(int id) { (void)id; STUB(FntFlush); return 0; }
void   SetDumpFnt(int id) { (void)id; STUB(SetDumpFnt); }

/* ---- GTE ----------------------------------------------------------------- */
void   InitGeom(void) { STUB(InitGeom); }
void   RotTransPers(SVECTOR *v0, long *sxy, long *p, long *flag) { (void)v0;(void)sxy;(void)p;(void)flag; STUB(RotTransPers); }
void   ApplyMatrixLV(MATRIX *m, VECTOR *v0, VECTOR *v1) { (void)m;(void)v0;(void)v1; STUB(ApplyMatrixLV); }
void   SetRotMatrix(MATRIX *m) { (void)m; STUB(SetRotMatrix); }
void   SetTransMatrix(MATRIX *m) { (void)m; STUB(SetTransMatrix); }

/* ---- SPU ----------------------------------------------------------------- */
void   SpuSetKey(int on_off, u_long voice) { (void)on_off;(void)voice; STUB(SpuSetKey); }
long   SpuSetVoicePitch(int vNum, u_short pitch) { (void)vNum;(void)pitch; STUB(SpuSetVoicePitch); return 0; }
u_short SpuGetVoicePitch(int vNum) { (void)vNum; STUB(SpuGetVoicePitch); return 0; }
short  SpuSetVoiceVolume(int vNum, short l, short r) { (void)vNum;(void)l;(void)r; STUB(SpuSetVoiceVolume); return 0; }
void   SpuSetCommonCDVolume(short l, short r) { (void)l;(void)r; STUB(SpuSetCommonCDVolume); }
long   SpuVoiceRegisters(int rw, u_long voice, u_long attr, void *arg) { (void)rw;(void)voice;(void)attr;(void)arg; STUB(SpuVoiceRegisters); return 0; }

/* ---- libsnd -------------------------------------------------------------- */
void   SsUtReverbOn(void) { STUB(SsUtReverbOn); }

/* ---- CD ------------------------------------------------------------------ */
/* CdlLOC/CdlFILE are not in psyq_pc.h (gamecd.c owns its own typedefs), so the
 * position/file params here are typed as void* -- ABI-identical on 32-bit and
 * enough for the Phase-0 abort stubs. cd_files.c will provide real bodies. */
int    CdInit(void) { STUB(CdInit); return 0; }
int    CdControl(u_char com, u_char *param, u_char *result) { (void)com;(void)param;(void)result; STUB(CdControl); return 0; }
int    CdControlB(u_char com, u_char *param, u_char *result) { (void)com;(void)param;(void)result; STUB(CdControlB); return 0; }
int    CdControlF(u_char com, u_char *param) { (void)com;(void)param; STUB(CdControlF); return 0; }
int    CdRead(int sectors, u_long *buf, int mode) { (void)sectors;(void)buf;(void)mode; STUB(CdRead); return 0; }
int    CdReadSync(int mode, u_char *result) { (void)mode;(void)result; STUB(CdReadSync); return 0; }
int    CdSync(int mode, u_char *result) { (void)mode;(void)result; STUB(CdSync); return 0; }
void  *CdSearchFile(void *fp, char *name) { (void)name; STUB(CdSearchFile); return fp; }
int    CdGetSector(void *madr, int size) { (void)madr;(void)size; STUB(CdGetSector); return 0; }
void  *CdIntToPos(int i, void *p) { (void)i; STUB(CdIntToPos); return p; }
int    CdPosToInt(void *p) { (void)p; STUB(CdPosToInt); return 0; }
void  *CdDataCallback(void (*func)(int, u_char *)) { (void)func; STUB(CdDataCallback); return 0; }
void  *CdReadCallback(void (*func)(int)) { (void)func; STUB(CdReadCallback); return 0; }
void  *CdReadyCallback(void (*func)(int, u_char *)) { (void)func; STUB(CdReadyCallback); return 0; }

/* ---- PAD ----------------------------------------------------------------- */
void   PadInit(int mode) { (void)mode; STUB(PadInit); }
void   InitPAD(char *bufa, int lena, char *bufb, int lenb) { (void)bufa;(void)lena;(void)bufb;(void)lenb; STUB(InitPAD); }
void   StartPAD(void) { STUB(StartPAD); }
void   StopPAD(void) { STUB(StopPAD); }
void   ChangeClearPAD(int val) { (void)val; STUB(ChangeClearPAD); }
u_long PadRead(int id) { (void)id; STUB(PadRead); return 0; }

/* ---- BIOS / libapi / libetc ---------------------------------------------- */
long   DeliverEvent(u_long ev_class, u_long spec) { (void)ev_class;(void)spec; STUB(DeliverEvent); return 0; }
void  *InterruptCallback(int irq, void (*f)(void)) { (void)irq;(void)f; STUB(InterruptCallback); return 0; }
void  *DMACallback(int dma, void (*f)(void)) { (void)dma;(void)f; STUB(DMACallback); return 0; }
void  *ResetCallback(void) { STUB(ResetCallback); return 0; }
int    EnterCriticalSection(void) { STUB(EnterCriticalSection); return 0; }
void   ExitCriticalSection(void) { STUB(ExitCriticalSection); }
