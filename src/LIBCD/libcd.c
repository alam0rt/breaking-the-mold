#include "common.h"

extern u8 D_8009DF28[];
extern u8 D_8009DF38[];
extern u8 D_8009DF39[];
extern u8 D_8009DF34[];
extern void *D_8009DF18[];
extern void *D_8009DF1C[];
extern void *D_8009DF24[];
extern void *D_8009E230[];

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", StSetRing);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CdInit);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", EVENT_OBJ_84);

extern void DeliverEvent(u32 event, u32 spec);

void def_cbsync(void) {
    DeliverEvent(0xF0000003, 0x0020);
}

void def_cbready(void) {
    DeliverEvent(0xF0000003, 0x0040);
}

void def_cbread(void) {
    DeliverEvent(0xF0000003, 0x0040);
}

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", printf);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", DeliverEvent);

u8 CdStatus(void) {
    return D_8009DF28[0];
}

u8 CdMode(void) {
    return D_8009DF38[0];
}

u8 CdLastCom(void) {
    return D_8009DF39[0];
}

u8 *CdLastPos(void) {
    return D_8009DF34;
}

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CdReset);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", SYS_OBJ_98);

extern s32 CD_flush(void);

s32 CdFlush(void) {
    return CD_flush();
}

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CdSetDebug);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CdComstr);

void SYS_OBJ_110(void) {
}

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CdIntstr);

void SYS_OBJ_144(void) {
}

extern s32 CD_sync(s32 mode, u8 *result);
extern s32 CD_ready(s32 mode, u8 *result);
extern s32 CD_vol(void *vol);
extern s32 CD_getsector(void *madr, s32 size);
extern s32 CD_getsector2(void *madr, s32 size);

s32 CdSync(s32 mode, u8 *result) {
    return CD_sync(mode, result);
}

s32 CdReady(s32 mode, u8 *result) {
    return CD_ready(mode, result);
}

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CdSyncCallback);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CdReadyCallback);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CdControl);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CdControlF);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CdControlB);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", SYS_OBJ_538);

s32 CdMix(void *vol) {
    CD_vol(vol);
    return 1;
}

s32 CdGetSector(void *madr, s32 size) {
    return CD_getsector(madr, size) == 0;
}

s32 CdGetSector2(void *madr, s32 size) {
    return CD_getsector2(madr, size) == 0;
}

extern void *DMACallback(s32 dma, void *func);

void *CdDataCallback(void *func) {
    return DMACallback(3, func);
}

extern s32 CD_datasync(s32 mode, u8 *result);

s32 CdDataSync(s32 mode, u8 *result) {
    return CD_datasync(mode, result);
}

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CdIntToPos);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CdPosToInt);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", getintr);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", BIOS_OBJ_64);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", BIOS_OBJ_26C);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", BIOS_OBJ_36C);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", BIOS_OBJ_3B8);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", BIOS_OBJ_43C);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", BIOS_OBJ_4C0);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", func_80083E34);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", BIOS_OBJ_570);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CD_sync);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", BIOS_OBJ_6B4);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", BIOS_OBJ_6E4);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", BIOS_OBJ_7DC);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CD_ready);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", BIOS_OBJ_93C);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", BIOS_OBJ_96C);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", BIOS_OBJ_AA4);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CD_cw);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", BIOS_OBJ_DAC);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", BIOS_OBJ_DDC);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", BIOS_OBJ_EC8);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CD_vol);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CD_flush);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CD_initvol);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CD_initintr);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CD_init);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CD_datasync);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", BIOS_OBJ_14A4);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CD_getsector);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CD_getsector2);

extern s32 D_8009E1C4[];

void CD_set_test_parmnum(s32 num) {
    D_8009E1C4[0] = num;
}

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", callback);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", BIOS_OBJ_1728);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", func_800850C4);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", BIOS_puts);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CdSearchFile);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", ISO9660_OBJ_F8);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", ISO9660_OBJ_29C);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", ISO9660_OBJ_2A8);

extern s32 strncmp(const char *s1, const char *s2, s32 n);

s32 cmp(const char *s1, const char *s2) {
    return strncmp(s1, s2, 12) == 0;
}

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CD_newmedia);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", ISO9660_OBJ_5A4);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CD_searchdir);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", ISO9660_OBJ_650);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CD_cachefile);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", ISO9660_OBJ_7F4);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", ISO9660_OBJ_810);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", ISO9660_OBJ_8F4);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", cd_read);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", strncmp);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", cb_read);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CDREAD_OBJ_A4);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", func_80085C1C);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CDREAD_OBJ_1A4);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", cb_data);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", cd_read_retry);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CDREAD_OBJ_5C0);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CdReadBreak);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CdRead);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CDREAD_OBJ_6B8);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CdReadSync);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CDREAD_OBJ_810);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CdReadCallback);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CdReadMode);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CdRead2);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CDREAD2_OBJ_50);

extern s32 StCdInterrupt(void);

s32 StCdInterrupt2(void) {
    return StCdInterrupt();
}

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CdReadFile);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CDREADE_OBJ_168);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CdReadExec);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", CDREADE_OBJ_24C);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", strcat);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", strcpy);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", StClearRing);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", StUnSetRing);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", EnterCriticalSection);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", ExitCriticalSection);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", data_ready_callback);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", StGetBackloc);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", C_004_OBJ_DC);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", StSetStream);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", StFreeRing);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", C_007_OBJ_AC);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", init_ring_status);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", StGetNext);

extern u32 D_800AE3D0[];
extern u32 D_800ACB98[];
extern u32 D_800AE3CC[];

void StSetMask(u32 mask, u32 start, u32 end) {
    D_800AE3D0[0] = mask;
    D_800ACB98[0] = start;
    D_800AE3CC[0] = end;
}

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", StCdInterrupt);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", C_011_OBJ_270);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", C_011_OBJ_3D0);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", func_80086E70);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", C_011_OBJ_7BC);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", C_011_OBJ_85C);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", func_80087304);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", C_011_OBJ_900);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", C_011_OBJ_960);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", mem2mem);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", dma_execute);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", C_011_OBJ_A30);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", func_800874F0);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", C_011_OBJ_A90);

extern void *D_800ACBA8[];

void *DsSyncCallback(void *func) {
    void *old = D_800ACBA8[0];
    D_800ACBA8[0] = func;
    return old;
}

extern void *D_800ACBAC[];

void *DsReadyCallback(void *func) {
    void *old = D_800ACBAC[0];
    D_800ACBAC[0] = func;
    return old;
}

void *DsDataCallback(void *func) {
    return DMACallback(3, func);
}

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", PadInit);

extern void PAD_dr(void);
extern u32 D_800A85F8[];

u32 PadRead(s32 pad) {
    PAD_dr();
    return ~D_800A85F8[0];
}

extern void StopPAD(void);

void PadStop(void) {
    StopPAD();
}

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", PAD_dr);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", ChangeClearPAD);

extern s32 D_8009E33C[];

void SetInitPadFlag(s32 flag) {
    D_8009E33C[0] = flag;
}

s32 ReadInitPadFlag(void) {
    return D_8009E33C[0];
}

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", PAD_init);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", InitPAD);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", StartPAD);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", StopPAD);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", SetPatchPad);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", RemovePatchPad);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", Pad1);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", IsVSync);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", InitPAD2);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", StartPAD2);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", StopPAD2);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", PAD_init2);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", SysEnqIntRP);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", SysDeqIntRP);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", EnablePAD);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", DisablePAD);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", patch_pad);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", FlushCache);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", SendPAD);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", send_pad);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", func_80087BA8);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", SENDPAD_OBJ_B4);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", remove_ChgclrPAD);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", VSync);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", VSYNC_OBJ_84);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", VSYNC_OBJ_130);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", v_wait);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", VSYNC_OBJ_1D4);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", ChangeClearRCnt);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", ResetCallback);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", InterruptCallback);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", DMACallback);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", VSyncCallback);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", VSyncCallbacks);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", StopCallback);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", RestartCallback);

extern u16 D_8009E3BA[];

u16 CheckCallback(void) {
    return D_8009E3BA[0];
}

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", GetIntrMask);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", SetIntrMask);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", startIntr);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", trapIntr);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", INTR_OBJ_428);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", setIntr);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", INTR_OBJ_514);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", stopIntr);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", restartIntr);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", INTR_OBJ_6D0);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", memclr_1);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", setjmp);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", func_80088538);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", BIOS_96_remove);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", ReturnFromException);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", ResetEntryInt);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", HookEntryInt);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", startIntrVSync);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", trapIntrVSync);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", setIntrVSync);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", memclr_2);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", startIntrDMA);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", trapIntrDMA);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", setIntrDMA);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", INTR_DMA_OBJ_274);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", memclr_3);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", SetVideoMode);

extern s32 D_8009F4A4[];

s32 GetVideoMode(void) {
    return D_8009F4A4[0];
}

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", LoadTPage);

INCLUDE_ASM("asm/nonmatchings/LIBCD/libcd", EXT_OBJ_A8);
