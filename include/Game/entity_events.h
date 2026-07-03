#ifndef ENTITY_EVENTS_H
#define ENTITY_EVENTS_H

/* =============================================================================
 * Entity Event IDs
 *
 * Event ids passed to the entity event-FSM slots (eventMarker/eventCallback
 * @ +0x08/+0x0C, and the extended slot @ +0x104/+0x108). Dispatch uses the
 * tagged-union marker encoding documented in Game/entity.h. Handlers receive
 * (target, eventId, arg, source); arg counts vary by handler.
 *
 * CLEAN-ROOM: ids are behavioral observations; names are ours. The canonical
 * table lives in docs/reference/entity-event-ids.md - keep the two in sync.
 *
 * The 1-3 range is engine lifecycle; the 0x10xx block is inter-entity
 * messaging. Values are used as plain ints at call sites, so substituting
 * these constants for the original hex literals is codegen-identical.
 * ============================================================================= */
typedef enum {
    /* Engine lifecycle (sent by the core entity loop) */
    EVT_SPAWN             = 1,      /* Spawn/init (create linked child) */
    EVT_TICK              = 2,      /* Per-frame tick / post-tick wakeup */
    EVT_GAME_NOTIFY       = 3,      /* Game-progression notify (sent to GameState slot) */

    /* Inter-entity messages (0x10xx block). Some ids are context-overloaded:
     * the handler that receives them decides the meaning, so the comment gives
     * the dominant meaning and (where relevant) the overload. Kept in sync with
     * docs/reference/entity-event-ids.md. */
    EVT_DAMAGE            = 0x1000, /* Damage/hit (generic: collision, hazard, projectile) */
    EVT_SET_READY         = 0x1001, /* Hit / set-ready flag (+0x106 = 1) */
    EVT_TOKEN_QUERY       = 0x1002, /* Dominant: projectile/kill hit; overload: token query/sync (returns entity ptr) */
    EVT_RIDER_ATTACH      = 0x1004, /* Rider attach/notify (forwarded to linked entity) */
    EVT_RIDER_DETACH      = 0x1005, /* Rider detach (clears stored rider) */
    EVT_UNBIND            = 0x1006, /* Child-destroyed / path-complete notify to parent */
    EVT_ENEMY_HIT_BOUNDS  = 0x1007, /* Enemy hit with bounds check (CheckPlayerHitByEnemy) */
    EVT_TOKEN_CLAIM       = 0x1008, /* Token claim (stores token if slot free) */
    EVT_TOKEN_RELEASE     = 0x1009, /* Dominant: destroy/death notify; overload: token release (clears on match) */
    EVT_PLATFORM_BOUNCE   = 0x100A, /* Player platform-bounce notify (1 site, low confidence) */
    EVT_IDENTITY_QUERY    = 0x100B, /* Identity query (handler returns its own entity ptr) */
    EVT_ENTER_SHRINK_ZONE = 0x100C, /* Enter shrink/scale zone -> PlayerEnterShrinkZone. NOTE: was
                                     * mislabeled EVT_COLLECTIBLE_PICKUP; the sender is a scale-trigger
                                     * "collectible", not a normal pickup (docs/reference/entity-event-ids.md). */
    EVT_EXIT_SHRINK_ZONE  = 0x100D, /* Exit shrink zone / reset scale -> PlayerExitShrinkZone */
    EVT_ATTACH_TO_ENTITY  = 0x100F, /* Checkpoint teleport / attach-ride-to-entity (snap to source position) */
    EVT_LEVEL_EXIT_TELEPORT = 0x1010, /* Level-exit teleporter (player); overload: multipart hide / try-idle query */
    EVT_RECOLOR_YELLOW    = 0x1011, /* Recolor indicator (yellow variant) */
    EVT_HUD_AWARD         = 0x1013, /* HUD award notification (args 5, 0) */
    EVT_SIGNAL_FIRE       = 0x1014, /* Named-trigger signal (carries signal id; clayball) */
    EVT_SOUND_END_NOTIFY  = 0x1015, /* Sound-emitter end notify (sent to player; clayball) */
    EVT_SET_STATE_FLAG    = 0x1016, /* Special activation / set state flag (+0x122) */
    EVT_CAN_INTERACT_QUERY = 0x1017, /* Query "can interact/teleport?" (returns true if enabled) */
    EVT_WORLD_FREEZE      = 0x1018  /* World freeze broadcast (Universe Enema activation) */
} EntityEventId;

#endif /* ENTITY_EVENTS_H */
