/* =============================================================================
 * SetupSpriteFromFrame.c  --  PC-port sprite render-primitive frame setup
 * =============================================================================
 * Faithful transcription of export/SLES_010.90.c SetupSpriteFromFrame (0x800195A8
 * region, INCLUDE_ASM in src/layer.c). Copies a static sprite frame's texture
 * parameters into the render primitive: indexes the frame table (0x14-byte
 * entries at *spriteContext + 4) by frameIndex, then writes the texture page
 * (preserving the primitive's existing semi-transparency bits 5-6), the U/V
 * texture coordinates, the CLUT id (from the sprite asset +0x12), and raises the
 * primitive's dirty flag.
 *
 * spriteContext: pointer to a 4-byte slot holding the sprite-asset base pointer.
 * renderState: the BasicEntity render primitive. Frame entry: tpage u16 @+0x0C,
 * U u8 @+0x0E, V u8 @+0x0F. renderState: tpage @+0x24, CLUT @+0x26, dirty @+0x2E,
 * U @+0x30, V @+0x31.
 * ---------------------------------------------------------------------------*/
#include "common.h"
#include <stdint.h>

void SetupSpriteFromFrame(void *spriteContext, int renderState, u32 frameIndex) {
    u8 *asset = *(u8 **)spriteContext;
    u8 *frame = *(u8 **)(asset + 4) + (frameIndex & 0xFFFF) * 0x14;
    u8 *rs = (u8 *)(intptr_t)renderState;
    u16 tpage = *(u16 *)(frame + 0xC);
    u8  u = *(u8 *)(frame + 0xE);
    u8  v = *(u8 *)(frame + 0xF);

    *(u8 *)(rs + 0x2E) = 1;
    *(u16 *)(rs + 0x24) = (tpage & 0xFF9F) | (*(u16 *)(rs + 0x24) & 0x60);
    *(u8 *)(rs + 0x30) = u;
    *(u8 *)(rs + 0x31) = v;
    *(u16 *)(rs + 0x26) = *(u16 *)(asset + 0x12);
}
