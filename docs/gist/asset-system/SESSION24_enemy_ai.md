# Session 24 — Enemy AI system

## Yes, there's real (modest) enemy AI — factory + state-machine + per-frame tick
~30 of 121 entity types are distinct enemy behaviors. Each: an Init factory, a tick callback,
and a small state set. Verified against the decomp (not just the doc).

## VERIFIED in decomp
- **EnemyAIUpdateWithRandomization (FUN_8004fb58)** — the core randomized tick, confirmed:
  - decrement ai_timer [+0x101]; when 0 -> ai_random_value[+0x100] = rand()&7, reset timer to
    (rand()&0x1F)+0x20 = **32-63 frames**. So enemies pick a new random behavior every ~0.5-1s.
  - EntityUpdateCallback (base move/anim), off-screen cleanup (SendMessage state,3),
    CollisionCheckWrapper(.,4,**0x1002**,.) = projectile-hit detection.
- **Boss HP = g_pPlayerState[0x1D], init 5** (line 15315), decremented and checked ==0 for boss
  death (15540/15589). Player-projectile hit (msg 0x1002) does the decrement; ~4-5 hits to kill.

## Entity AI fields (verified offsets)
+0x68/6A world pos (physics) · +0x94/96 render pos · +0x74 facing_left · +0x100 ai_random
(rand()&7) · +0x101 ai_timer · +0x108 collision_target · +0x10C hit_counter(max4) · +0x116
sound_cooldown(180f) · +0x12 collision mask (0x0002=enemy) · +0x16 damage-react (0x8000).

## AI functions
InitGroundPatrolEnemy 0x8002ea3c (Clay Keeper, Loud Mouth) · InitEnemyEntityWithAI 0x8004f8dc ·
EnemyAIUpdateWithRandomization 0x8004fb58 · EnemyHitMessageHandler 0x8004ddf0 ·
InitPathFollowingEnemy 0x8003c5b8 · InitWalkingEnemyEntity 0x800514a0 · Type25 factory 0x800805c8.

## States (0-7): IDLE/PATROL/CHASE/ATTACK/HURT/DEATH/STUNNED/SPECIAL
## Behavior patterns
- Patrol: constant-speed walk, turn on wall OR ledge, player gravity. (core verified; turn
  logic is doc pseudo-code.)
- Flying: horizontal + sine-table vertical oscillation, no gravity, optional player-Y track.
- Shooter: timer -> fire if player in ~200-300px range.
- Chase: idle->chase->attack by player proximity.
- Jump/hop: periodic fixed-velocity hops.

## Hit sounds (ties to our audio work)
EnemyHitMessageHandler plays RANDOM sounds from tables @0x8009bb84/90/9c on hit messages
0x24825800/0x308b4800, gated by sound_cooldown[+0x116]=180 frames (3s). This is the
random-variant mechanism (Session 15's & 0xf00 flag) applied at the AI layer.

## Net
- Enemy AI characterized: randomized-timer behavior switching + 8-state machine + per-type
  tick. Classic 2D-platformer AI — real but not complex/scripted.
- Boss HP field + death logic verified (g_pPlayerState[0x1D]=5).
- New player-state field: [0x1D] = boss HP (joins [0x11]=lives, inventory map).

## Files
- `enemy_ai.json`
