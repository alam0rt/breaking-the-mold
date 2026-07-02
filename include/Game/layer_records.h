#ifndef GAME_LAYER_RECORDS_H
#define GAME_LAYER_RECORDS_H

#include "common.h"
#include "Game/entity.h"

typedef struct {
    Entity *entity;
    u8 _pad04[20];
} LayerRenderSlot;

#endif /* GAME_LAYER_RECORDS_H */
