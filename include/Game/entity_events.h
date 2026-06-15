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

    /* Inter-entity messages (0x10xx block) */
    EVT_DAMAGE            = 0x1000, /* Damage/hit */
    EVT_SET_READY         = 0x1001, /* Set ready flag */
    EVT_TOKEN_QUERY       = 0x1002, /* Token query/sync (returns entity ptr) */
    EVT_RIDER_ATTACH      = 0x1004, /* Rider attach/notify (forwarded to linked entity) */
    EVT_RIDER_DETACH      = 0x1005, /* Rider detach (clears stored rider) */
    EVT_UNBIND            = 0x1006, /* Unbind/detach linked follower (clayball ResetState) */
    EVT_TOKEN_CLAIM       = 0x1008, /* Token claim (stores token if slot free) */
    EVT_TOKEN_RELEASE     = 0x1009, /* Token release (clears token on match) */
    EVT_IDENTITY_QUERY    = 0x100B, /* Identity query (handler returns its own entity ptr) */
    EVT_COLLECTIBLE_PICKUP = 0x100C,/* Collectible pickup (sent to player, collectible as arg) */
    EVT_ATTACH_TO_ENTITY  = 0x100F, /* Attach/ride-to-entity (snap to source position) */
    EVT_RECOLOR_YELLOW    = 0x1011, /* Recolor indicator (yellow variant) */
    EVT_HUD_AWARD         = 0x1013, /* HUD award notification (args 5, 0) */
    EVT_SIGNAL_FIRE       = 0x1014, /* Named-trigger signal (carries signal id; clayball) */
    EVT_SOUND_END_NOTIFY  = 0x1015, /* Sound-emitter end notify (sent to player; clayball) */
    EVT_SET_STATE_FLAG    = 0x1016, /* Set state flag (+0x122) */
    EVT_WORLD_FREEZE      = 0x1018  /* World freeze broadcast (Universe Enema activation) */
} EntityEventId;

#endif /* ENTITY_EVENTS_H */
