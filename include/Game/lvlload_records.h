#ifndef GAME_LVLLOAD_RECORDS_H
#define GAME_LVLLOAD_RECORDS_H

#include "common.h"

typedef struct CheckpointEntityNode {
    /* 0x00 */ struct CheckpointEntityNode *next;
    /* 0x04 */ s32 entity_id;
} CheckpointEntityNode;

#endif /* GAME_LVLLOAD_RECORDS_H */
