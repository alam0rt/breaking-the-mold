#include "common.h"

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", GetFrameReadyFlag);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", TriggerBufferSwapIfReady);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", SetVideoModePAL);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", SsUtReverbOn);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80013268);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80013490);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_800134B8);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80013500);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_8001352C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80013554);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_800136C8);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80013718);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80013760);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_800137B0);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80013800);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80013850);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_800138A0);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_800138F0);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80013AB0);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80013B1C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80013D10);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80013F50);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80014134);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80014278);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_800143A4);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_800143F0);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_800145A4);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80014854);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80014928);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80014968);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_800149E8);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80014A9C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80014CF8);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80015074);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_800150C4);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80015134);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80015424);

/* Empty function. NOTE: cannot live here as decompiled C - gcc 2.7.2 at -O2
 * defers tiny inline-candidate functions to end-of-file, which breaks the
 * address-order layout this file must preserve (verified: the 8-byte body
 * relocated to 0x8001A0B8 and shifted everything after 0x80015434). */
INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80015434);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_8001543C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_8001546C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80015614);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_800156A4);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80015764);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80015E0C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80015EB4);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_8001601C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80016470);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_8001652C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_800165A4);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80016AC8);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80016FE8);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_8001713C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80017540);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80017A3C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80017AF8);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80017D0C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018110);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_800181FC);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_8001889C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_800188E0);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018BD8);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018BE0);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018BE8);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018BF4);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018C0C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018C18);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018C24);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018C30);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018C38);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018C40);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018C4C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018C58);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018C60);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018C6C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018C78);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018C80);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018C88);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018C94);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018C9C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018CC0);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018CD0);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018CD8);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018D00);

/* Empty function. See func_80015434 note re: -O2 deferred output. */
INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018D4C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018D54);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018D94);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80018DDC);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_800191DC);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80019238);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80019364);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_800193D4);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80019474);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_800194F4);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_8001954C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80019558);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_800195B0);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_8001960C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80019618);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_8001963C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80019650);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80019678);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_800196D8);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80019700);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80019748);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80019790);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80019864);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_8001991C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80019A14);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80019CF8);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80019D74);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80019F2C);

INCLUDE_ASM("asm/nonmatchings/Game/RENDER", func_80019F88);
