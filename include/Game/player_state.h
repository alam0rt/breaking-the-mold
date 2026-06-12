#ifndef PLAYER_STATE_H
#define PLAYER_STATE_H

#include "common.h"

/* =============================================================================
 * PlayerState (30 bytes)
 *
 * Clean-room RE note: names are inferred from usage, not original symbols.
 *
 * Persistent player inventory, lives, and collectible counts.
 * Pointed to by g_pPlayerState (0x800A5754) and restored on checkpoint
 * respawn.
 *
 * Field powerup_flags (0x17): restored to player entity +0x18 on respawn
 * (see GameState+0x14B checkpoint_powerup_state).
 *
 * Clayball flags (0x06-0x0F): 10 slots for level-specific clay ball
 * collection state.
 * ============================================================================= */
typedef struct {
    /* 0x00 */ u8  initialized;      /* Non-zero once struct has been set up */
    /* 0x01 */ u8  active;           /* Player active flag */
    /* 0x02 */ u16 results_total_tally; /* Cumulative value displayed on results screens */
    /* 0x04 */ u8  world_index_tally;   /* World-index/progression tally displayed on results screens */
    /* 0x05 */ u8  total_1ups;       /* Total 1-UPs collected */
    /* 0x06 */ u8  clayball_flag_0;  /* Clayball collection state, slot 0 */
    /* 0x07 */ u8  clayball_flag_1;
    /* 0x08 */ u8  clayball_flag_2;
    /* 0x09 */ u8  clayball_flag_3;
    /* 0x0A */ u8  clayball_flag_4;
    /* 0x0B */ u8  clayball_flag_5;
    /* 0x0C */ u8  clayball_flag_6;
    /* 0x0D */ u8  clayball_flag_7;
    /* 0x0E */ u8  clayball_flag_8;
    /* 0x0F */ u8  clayball_flag_9;
    /* 0x10 */ u8  level_complete;   /* Non-zero if current level is completed */
    /* 0x11 */ u8  lives;            /* Remaining lives */
    /* 0x12 */ u8  orb_count;        /* Orbs collected */
    /* 0x13 */ u8  swirly_q_count;   /* Swirly-Qs collected this level */
    /* 0x14 */ u8  phoenix_hands;    /* Phoenix Hands powerup count */
    /* 0x15 */ u8  phart_heads;      /* P-Hart Heads powerup count */
    /* 0x16 */ u8  universe_enemas;  /* Universe Enemas powerup count */
    /* 0x17 */ u8  powerup_flags;    /* Active powerup bitfield (restored on respawn) */
    /* 0x18 */ u8  shrink_mode;      /* Shrink powerup active (0=normal, 1=shrunk) */
    /* 0x19 */ u8  icon_1970_count;  /* "1970" icon collectible count */
    /* 0x1A */ u8  hamster_count;    /* Hamster collectible count */
    /* 0x1B */ u8  total_swirly_qs;  /* Total Swirly-Qs accumulated */
    /* 0x1C */ u8  super_willies;    /* Super Willies powerup count */
    /* 0x1D */ u8  boss_hp;          /* Boss HP (set 3-5 by boss init, decremented on hits, cleared on death) */
} PlayerState;  /* Size: 0x1E (30 bytes) */

#endif /* PLAYER_STATE_H */
