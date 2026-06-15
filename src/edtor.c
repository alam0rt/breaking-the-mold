#include "common.h"

INCLUDE_ASM("asm/nonmatchings/edtor", EntityDestructor_DestroyAllChildEntities);

void func_8002C528(void *e) {
    u8 *p = *(u8 **)((u8 *)e + 0x1C);
    *(u8 *)((u8 *)e + 0x20) = 1;
    p[0xA] = 0;
}

INCLUDE_ASM("asm/nonmatchings/edtor", EntityDestructor_Type0);

INCLUDE_ASM("asm/nonmatchings/edtor", EntityDestructor_Type1);

INCLUDE_ASM("asm/nonmatchings/edtor", EntityDestructor_Type2);

INCLUDE_ASM("asm/nonmatchings/edtor", EntityDestructor_Type3);

INCLUDE_ASM("asm/nonmatchings/edtor", EntityDestructor_Type4);

INCLUDE_ASM("asm/nonmatchings/edtor", EntityDestructor_Type5);

void func_8002C794(void) {
}

void func_8002C79C(void) {
}

INCLUDE_ASM("asm/nonmatchings/edtor", EntityDestructor_Type6_Simple);

