#ifndef BLB_RECORDS_H
#define BLB_RECORDS_H

#include "common.h"
#include "Game/entity.h"
#include "Game/game_state.h"

/* =============================================================================
 * BLB-ALLOCATED ENTITY / HEAP OVERLAY LAYOUTS
 *
 * Clean-room RE note: all struct/field names are inferred working labels.
 * These overlay BLB-heap-allocated entity instances, the heap header, and the
 * BLB entity vtable as touched by the tick loop / removal code. Padding runs
 * cover offsets not yet traced. Used exclusively by src/blb.c.
 *
 * NOTE: the "BLB entity" passed to DestroyEntity / RemoveFromUpdateQueue /
 * EntityTickLoop / RemoveFromZOrderList / RenderEntities is actually the
 * central GameState scene object (see Game/game_state.h) -- every field those
 * functions touch (postRenderCallbackContext @0x18, previous_spawn_list @0x3C,
 * tick_list_head @0x1C, tile_render_state_count/ptr @0x104/0x108,
 * palette_group_ptrs/count @0x110/0x114, bg_color_change_flag @0x130) is a
 * documented GameState member. Those functions use GameState directly; no
 * dedicated overlay struct is needed here.
 * ============================================================================= */

typedef struct BlbEntityWithSpriteSubobject {
    /* 0x00 */ Entity base;
    /* 0x80 */ u8 pad80[4];
    /* 0x84 */ u8 spriteSubobject;
} BlbEntityWithSpriteSubobject;

/* BLB heap header. The queue-busy latch at +0xA08A is the single field
 * read/written here; the rest of the heap holds the bump allocator state
 * and entity instances. */
typedef struct {
    /* 0x0000 */ u8  _pad0000[0xA08A];
    /* 0xA08A */ s16 queueBusyLatch;
} BlbHeapHeader;

/* BLB entity vtable subset. Only the two fields used by EntityTickLoop are
 * named; the full table has more slots but they aren't read in this file. */
typedef struct {
    /* 0x00 */ u8   _pad00[0x10];
    /* 0x10 */ s16  argOffset;            /* added to entity ptr before calling tickFn */
    /* 0x12 */ u8   _pad12[2];
    /* 0x14 */ void (*tickFn)(void *);
} BlbEntityVtable;

#endif /* BLB_RECORDS_H */
