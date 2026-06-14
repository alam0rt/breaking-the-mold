#include "common.h"

typedef void *(*CallbackFunc)();
extern CallbackFunc *D_8009F440[];

extern u8 D_8009DF28[];
extern u8 D_8009DF38[];
extern u8 D_8009DF39[];
extern u8 D_8009DF34[];
extern void *D_8009DF18[];
extern void *D_8009DF1C[];
extern void *D_8009DF24[];
extern void *D_8009E230[];

extern u32 D_800AE3D8[];
extern u32 D_800AE3DC[];
extern void StClearRing(void);

void StSetRing(u32 *ring, s32 size) {
    D_800AE3D8[0] = (u32)ring;
    D_800AE3DC[0] = (u32)size;
    StClearRing();
}

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CdInit);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", EVENT_OBJ_84);

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

INCLUDE_ASM("asm/nonmatchings/libs/libcd", printf);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", DeliverEvent);

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

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CdReset);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", SYS_OBJ_98);

extern s32 CD_flush(void);

s32 CdFlush(void) {
    return CD_flush();
}

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CdSetDebug);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CdComstr);

void SYS_OBJ_110(void) {
}

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CdIntstr);

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

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CdSyncCallback);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CdReadyCallback);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CdControl);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CdControlF);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CdControlB);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", SYS_OBJ_538);

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

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CdIntToPos);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CdPosToInt);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", getintr);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", BIOS_OBJ_64);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", BIOS_OBJ_26C);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", BIOS_OBJ_36C);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", BIOS_OBJ_3B8);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", BIOS_OBJ_43C);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", BIOS_OBJ_4C0);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", func_80083E34);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", BIOS_OBJ_570);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CD_sync);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", BIOS_OBJ_6B4);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", BIOS_OBJ_6E4);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", BIOS_OBJ_7DC);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CD_ready);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", BIOS_OBJ_93C);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", BIOS_OBJ_96C);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", BIOS_OBJ_AA4);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CD_cw);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", BIOS_OBJ_DAC);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", BIOS_OBJ_DDC);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", BIOS_OBJ_EC8);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CD_vol);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CD_flush);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CD_initvol);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CD_initintr);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CD_init);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CD_datasync);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", BIOS_OBJ_14A4);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CD_getsector);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CD_getsector2);

extern s32 D_8009E1C4[];

void CD_set_test_parmnum(s32 num) {
    D_8009E1C4[0] = num;
}

INCLUDE_ASM("asm/nonmatchings/libs/libcd", callback);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", BIOS_OBJ_1728);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", func_800850C4);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", BIOS_puts);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CdSearchFile);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", ISO9660_OBJ_F8);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", ISO9660_OBJ_29C);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", ISO9660_OBJ_2A8);

extern s32 strncmp(const char *s1, const char *s2, s32 n);

s32 cmp(const char *s1, const char *s2) {
    return strncmp(s1, s2, 12) == 0;
}

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CD_newmedia);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", ISO9660_OBJ_5A4);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CD_searchdir);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", ISO9660_OBJ_650);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CD_cachefile);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", ISO9660_OBJ_7F4);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", ISO9660_OBJ_810);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", ISO9660_OBJ_8F4);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", cd_read);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", strncmp);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", cb_read);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CDREAD_OBJ_A4);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", func_80085C1C);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CDREAD_OBJ_1A4);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", cb_data);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", cd_read_retry);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CDREAD_OBJ_5C0);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CdReadBreak);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CdRead);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CDREAD_OBJ_6B8);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CdReadSync);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CDREAD_OBJ_810);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CdReadCallback);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CdReadMode);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CdRead2);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CDREAD2_OBJ_50);

extern s32 StCdInterrupt(void);

s32 StCdInterrupt2(void) {
    return StCdInterrupt();
}

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CdReadFile);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CDREADE_OBJ_168);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CdReadExec);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", CDREADE_OBJ_24C);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", strcat);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", strcpy);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", StClearRing);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", StUnSetRing);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", EnterCriticalSection);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", ExitCriticalSection);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", data_ready_callback);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", StGetBackloc);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", C_004_OBJ_DC);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", StSetStream);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", StFreeRing);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", C_007_OBJ_AC);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", init_ring_status);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", StGetNext);

extern u32 D_800AE3D0[];
extern u32 D_800ACB98[];
extern u32 D_800AE3CC[];

void StSetMask(u32 mask, u32 start, u32 end) {
    D_800AE3D0[0] = mask;
    D_800ACB98[0] = start;
    D_800AE3CC[0] = end;
}

INCLUDE_ASM("asm/nonmatchings/libs/libcd", StCdInterrupt);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", C_011_OBJ_270);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", C_011_OBJ_3D0);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", func_80086E70);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", C_011_OBJ_7BC);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", C_011_OBJ_85C);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", func_80087304);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", C_011_OBJ_900);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", C_011_OBJ_960);

void mem2mem(s32 *dst, s32 *src, u32 count) {
    u32 i;
    for (i = 0; i < count; i++) {
        *dst++ = *src++;
    }
}

INCLUDE_ASM("asm/nonmatchings/libs/libcd", dma_execute);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", C_011_OBJ_A30);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", func_800874F0);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", C_011_OBJ_A90);

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

INCLUDE_ASM("asm/nonmatchings/libs/libcd", PadInit);

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

INCLUDE_ASM("asm/nonmatchings/libs/libcd", PAD_dr);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", ChangeClearPAD);

extern s32 D_8009E33C[];

void SetInitPadFlag(s32 flag) {
    D_8009E33C[0] = flag;
}

s32 ReadInitPadFlag(void) {
    return D_8009E33C[0];
}

INCLUDE_ASM("asm/nonmatchings/libs/libcd", PAD_init);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", InitPAD);

extern void StartPAD2(void);
extern void ChangeClearPAD(s32 mode);
extern void EnablePAD(void);

void StartPAD(void) {
    StartPAD2();
    ChangeClearPAD(0);
    EnablePAD();
}

INCLUDE_ASM("asm/nonmatchings/libs/libcd", StopPAD);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", SetPatchPad);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", RemovePatchPad);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", Pad1);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", IsVSync);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", InitPAD2);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", StartPAD2);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", StopPAD2);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", PAD_init2);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", SysEnqIntRP);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", SysDeqIntRP);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", EnablePAD);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", DisablePAD);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", patch_pad);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", FlushCache);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", SendPAD);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", send_pad);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", func_80087BA8);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", SENDPAD_OBJ_B4);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", remove_ChgclrPAD);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", VSync);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", VSYNC_OBJ_84);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", VSYNC_OBJ_130);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", v_wait);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", VSYNC_OBJ_1D4);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", ChangeClearRCnt);

void *ResetCallback(void) {
    CallbackFunc *table = D_8009F440[0];
    return (void *)table[3]();
}

void *InterruptCallback(void) {
    CallbackFunc *table = D_8009F440[0];
    return (void *)table[2]();
}

void *DMACallback(s32 dma, void *func) {
    CallbackFunc *table = D_8009F440[0];
    return table[1](dma, func);
}

void *VSyncCallback(void *func) {
    CallbackFunc *table = D_8009F440[0];
    return table[5](4, func);
}

void *VSyncCallbacks(void) {
    CallbackFunc *table = D_8009F440[0];
    return (void *)table[5]();
}

void *StopCallback(void) {
    CallbackFunc *table = D_8009F440[0];
    return (void *)table[4]();
}

void *RestartCallback(void) {
    CallbackFunc *table = D_8009F440[0];
    return (void *)table[6]();
}

extern u16 D_8009E3BA[];

u16 CheckCallback(void) {
    return D_8009E3BA[0];
}

INCLUDE_ASM("asm/nonmatchings/libs/libcd", GetIntrMask);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", SetIntrMask);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", startIntr);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", trapIntr);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", INTR_OBJ_428);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", setIntr);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", INTR_OBJ_514);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", stopIntr);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", restartIntr);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", INTR_OBJ_6D0);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", memclr_1);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", setjmp);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", func_80088538);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", BIOS_96_remove);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", ReturnFromException);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", ResetEntryInt);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", HookEntryInt);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", startIntrVSync);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", trapIntrVSync);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", setIntrVSync);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", memclr_2);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", startIntrDMA);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", trapIntrDMA);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", setIntrDMA);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", INTR_DMA_OBJ_274);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", memclr_3);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", SetVideoMode);

extern s32 D_8009F4A4[];

s32 GetVideoMode(void) {
    return D_8009F4A4[0];
}

INCLUDE_ASM("asm/nonmatchings/libs/libcd", LoadTPage);

INCLUDE_ASM("asm/nonmatchings/libs/libcd", EXT_OBJ_A8);
