/* =============================================================================
 * FreeAndCoalesceVRAMSlot.c  --  functional-C body for vram.c
 * FreeAndCoalesceVRAMSlot (TARGET_PC)
 * =============================================================================
 * Transcribed from asm/nonmatchings/vram/FreeAndCoalesceVRAMSlot.s (0x80015134,
 * 0x2F0). Frees `rows` rows starting at absolute row `row` from the VRAM slot
 * chain and merges fully-free neighbours. First reached when an entity that
 * owns VRAM (the spawn teleporter) is removed after its eject animation.
 *
 * Layout (matches src/vram.c FindFreeVRAMSlotEntry/GetMaxVRAMSlotSize):
 * stride-6 entries at base: rowMask u16 @+0xA08C (bit=row in use), maxRun u8
 * @+0xA08E, next/status u8 @+0xA08F (0xFF terminator, 0xFE free entry), row
 * count u8 @+0xA090; chain head u8 @ base+0xA29C, rows-per-slot u8 @+0xA29D,
 * total free rows u8 @+0xA29E.
 *
 * Walk the chain accumulating row counts until the slot containing `row`;
 * clear the freed rows' bits in its mask; recompute the longest free run
 * (maxRun); if the mask empties, credit the freed rows and coalesce with the
 * previous entry (when it is empty too, absorbing this one, marking it 0xFE)
 * and then with the next entry likewise.
 * ========================================================================== */
#include "common.h"

void FreeAndCoalesceVRAMSlot(void *arg0, s16 row, s32 rows) {
    u8 *base = (u8 *)arg0;
    u8 prev = 0xFF;
    s32 rowBase = 0;
    u32 n = (u32)(rows & 0xFF);
    u8 idx = *(u8 *)(base + 0xA29C);

    while ((idx & 0xFF) != 0xFF) {
        u8 *ent = base + idx * 6;
        if ((rowBase & 0xFF) != row) {
            prev = idx;
            rowBase += *(u8 *)(ent + 0xA090);
            idx = *(u8 *)(ent + 0xA08F);
            continue;
        }

        {
            u16 mask = *(u16 *)(ent + 0xA08C);
            s32 bit = 1 << row;
            u32 i;
            u8 run = 0, best = 0;
            s32 probe = 1;
            u8 perSlot = *(u8 *)(base + 0xA29D);

            for (i = 0; i < n; i++) {
                mask ^= bit;
                bit <<= 1;
            }
            *(u16 *)(ent + 0xA08C) = mask;

            for (i = 0; i < perSlot; i++) {
                run++;
                if (probe & mask & 0xFFFF) {
                    run = 0;
                } else if (run >= best) {
                    best = run;
                }
                probe <<= 1;
            }
            *(u8 *)(ent + 0xA08E) = best;

            if ((mask & 0xFFFF) == 0) {
                u8 cur = idx;
                *(u8 *)(base + 0xA29E) += *(u8 *)(ent + 0xA090);
                if (prev != 0xFF) {
                    u8 *pent = base + prev * 6;
                    if (*(u16 *)(pent + 0xA08C) == 0) {
                        /* absorb this entry into the empty previous one */
                        *(u8 *)(pent + 0xA08F) = *(u8 *)(ent + 0xA08F);
                        *(u8 *)(pent + 0xA090) += *(u8 *)(ent + 0xA090);
                        *(u8 *)(ent + 0xA08F) = 0xFE;
                        *(u8 *)(ent + 0xA090) = 0xFE;
                        cur = prev;
                    }
                }
                {
                    u8 *cent = base + cur * 6;
                    u8 nxt = *(u8 *)(cent + 0xA08F);
                    if (nxt != 0xFF) {
                        u8 *nent = base + nxt * 6;
                        if (*(u16 *)(nent + 0xA08C) == 0) {
                            *(u8 *)(cent + 0xA08F) = *(u8 *)(nent + 0xA08F);
                            *(u8 *)(cent + 0xA090) += *(u8 *)(nent + 0xA090);
                            *(u8 *)(nent + 0xA08F) = 0xFE;
                            *(u8 *)(nent + 0xA090) = 0xFE;
                        }
                    }
                }
            }
        }
        return;
    }
}
