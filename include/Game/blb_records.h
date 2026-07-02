#ifndef BLB_RECORDS_H
#define BLB_RECORDS_H

#include "common.h"
#include "Game/entity.h"

/* =============================================================================
 * BLB-ALLOCATED ENTITY / HEAP OVERLAY LAYOUTS
 *
 * Clean-room RE note: all struct/field names are inferred working labels.
 * These overlay BLB-heap-allocated entity instances, the heap header, and the
 * BLB entity vtable as touched by the tick loop / removal code. Padding runs
 * cover offsets not yet traced. Used exclusively by src/blb.c.
 * ============================================================================= */

typedef struct BlbEntityWithSpriteSubobject {
    /* 0x00 */ Entity base;
    /* 0x80 */ u8 pad80[4];
    /* 0x84 */ u8 spriteSubobject;
} BlbEntityWithSpriteSubobject;

/* BLB-allocated entity layout (only fields this file touches are named).
 * NOTE: distinct from the engine's Entity at include/Game/entity.h despite
 * the partial overlap -- +0x3C in particular is a per-instance helper buffer
 * pointer here, where Entity stores renderWidth (s16). The destructor at
 * DestroyEntity, the tick-queue free in RemoveFromUpdateQueue, and the
 * Z-order child array touched by RemoveFromZOrderList confirm the layout. */
typedef struct {
    /* 0x000 */ u8    _pad000[0x18];
    /* 0x018 */ void *vtable;            /* same slot as Entity.collisionVtable */
    /* 0x01C */ u8    _pad01C[0x3C - 0x1C];
    /* 0x03C */ void *helperBuffer;      /* arbitrary per-instance buffer freed via builtin_delete */
    /* 0x040 */ u8    _pad040[0x104 - 0x40];
    /* 0x104 */ s16   tickQueueCount;
    /* 0x106 */ u8    _pad106[2];
    /* 0x108 */ void *tickQueue;
} BlbEntity;

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

/* EntityListNode comes from Game/entity.h (via functions.h). */
typedef struct EntityListHead {
    u8 pad[0x1C];
    EntityListNode *head;        /* 0x1C */
} EntityListHead;

#endif /* BLB_RECORDS_H */
