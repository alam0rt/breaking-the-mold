---
title: "Making Things Tick"
category: book
tags: [book, tick, frame, entities, fsm, ai, game-loop]
---

# Making Things Tick

> *A note on names.* As in the last chapter, this is a clean-room reconstruction:
> every function and field name — `EntityTickLoop`, `tickMarker`, `ai_timer` — is
> inferred from behaviour and may be wrong. The control flow and the byte offsets
> are read off the running machine and are the parts to trust.

In the last chapter we took a world apart and found it wasn't a place at all —
just a recipe and a box of parts, reassembled into a picture sixty-ish times a
second. But a picture that only ever *redraws* is a screensaver. What makes
Skullmonkeys a game is that between one picture and the next, **things change**:
Klaymen drifts a pixel left, a monkey decides to throw a fireball, a timer
ticks toward zero, a pickup notices it's been touched and vanishes.

This chapter is about that gap between two frames — the **tick**. A tick is one
turn of the engine's crank: read the controller, let everything that's alive take
its turn, move the camera, and only *then* draw. Everything dynamic in the game is
something that happens during a tick, and almost all of it happens the same way,
through one small idiom repeated a few thousand times. Four questions, four
sections:

1. **What is a tick?** — the heartbeat, and the order of work inside it.
2. **Who gets a turn?** — how the engine knows what to update, via three lists.
3. **What is an entity's "turn"?** — tick callbacks, and why *state is a function
   pointer*.
4. **How does an enemy decide anything?** — AI as timers, dice, and messages.

---

## Section 1 — The heartbeat

Open the engine's front door — `main` at `0x800828b0` — and past the start-up
ceremony you find an infinite loop. That loop *is* the game. One trip around it is
one tick, and it always does the same things in the same order:

```c
while (true) {
    TickCDStreamBuffer();          // keep streaming the CD
    padData = PadRead(1);          // sample both controllers
    UpdateInputState(p1, padData);   // edge-detect: held vs. just-pressed
    UpdateInputState(p2, padData >> 16);

    GameModeCallback(...);         // (1) the mode's per-frame logic
    EntityTickLoop(gameState);     // (2) give every live entity its turn
    WaitForVBlankIfNeeded(...);    // (3) sync to the display
    RenderEntities(gameState);     // (4) draw entities  -> ordering table
    DrawSync(0);
    <layer render callback>;       // (5) draw tile layers -> ordering table
    DrawSync(0);
}
```

The shape worth holding onto is the **split between thinking and drawing**.
Everything that *changes the world* — input, mode logic, entity updates, physics,
AI, collisions — happens in steps (1) and (2), before a single pixel is touched.
Drawing, steps (4) and (5), only *reads* the world the tick already settled. By
the time `RenderEntities` runs, the frame's outcome is decided; rendering is just
photography. That separation is what makes the world coherent on screen — the
renderer never catches an entity halfway through changing its mind, because the
renderer runs *after* all the minds are made up.

A word on the clock. You'd expect a tick to be pinned to the PlayStation's
vertical blank — one tick per displayed frame — and usually it is. But the sync
in step (3) is *conditional* (`WaitForVBlankIfNeeded`), and the engine only stalls
for frame-rate timing when a flag is set. So the tick is better thought of as
"one full pass of update-then-draw," nominally one v-blank on this PAL disc but
not religiously locked to it. Speeds throughout the game are quoted in *pixels per
frame*, which is to say: **the tick, not the second, is the engine's true unit of
time.** Everything that moves, moves per tick.

---

## Section 2 — Who gets a turn?

The loop above calls `EntityTickLoop` and waves a hand at "every live entity." But
the engine holds no master array of entities. Instead it threads each live entity
onto **intrusive linked lists** hanging off the `GameState`, and — this is the
trick — it keeps *three* of them, because the same entity needs to be visited in
three different orders for three different jobs:

| List | `GameState` | Visited in | Used for |
|------|-------------|-----------|----------|
| **Tick list** | `+0x1C` | insertion order | per-frame updates (`EntityTickLoop`) |
| **Render list** | `+0x20` | **z-order** (priority-sorted) | drawing (`RenderEntities`) |
| **Collision list** | `+0x24` | spatial | collision / event broadcast |

The render list is the one we met last chapter — kept sorted by priority so a
single walk paints back-to-front. The **tick list** is the one that matters here.
`EntityTickLoop` (`0x80020e1c`) is almost insultingly simple: walk the list, and
for each node, fire its tick callback.

```c
void EntityTickLoop(GameState *state) {
    Entity *e = state->tick_list_head;       // +0x1C
    while (e != NULL) {
        if (e->tick_fn != NULL)              // the +0x04 slot
            e->tick_fn(e);
        e = e->next;
    }
}
```

That's the whole scheduler. There is no priority on updates, no time-slicing, no
"AI budget." If you're on the list, you get a turn, every tick, in the order you
were added. To stop being updated, you come off the list.

This bluntness has a lovely consequence the engine exploits directly: **pausing
the game is just hiding the list.** When you hit Start, the pause handler saves the
tick-list head into a holding field and sets the head to null — `EntityTickLoop`
now walks an empty list and the entire world freezes mid-step. The pause menu's
*own* entities are spliced onto the (now otherwise empty) list, so they keep
animating while everything behind them holds its breath. Unpause swaps the saved
list back, and the world resumes from the exact step it stopped on. There is no
"paused" flag threaded through a thousand update functions; there is just a
different list.

---

## Section 3 — An entity's turn: state is a function pointer

So `EntityTickLoop` calls `e->tick_fn(e)`. What is that function, and how does an
entity decide what to do this frame?

Here we meet the single most important idiom in the engine, the one the FSM notes
call the **tagged callback slot**. Scattered through every entity are 8-byte pairs:

```c
typedef struct {
    s32 marker;   // how to interpret fn (and an offset)
    void *fn;     // the callback
} FSMStateSlot;
```

An entity carries several of these, each a different *channel* of behaviour:

| Slot | Offset | Channel |
|------|--------|---------|
| tick | `+0x00 / +0x04` | per-frame update (what `EntityTickLoop` calls) |
| event | `+0x08 / +0x0C` | messages: damage, pickups, collisions, *and the clock* |
| render | `+0x1C / +0x20` | screen-position / texture bridge, run during the tick |
| move X / Y | `+0x24…+0x30` | coordinate transforms (platform-riding, scaling) |

The marker word is not just a flag; it's a tiny instruction for the dispatcher,
read as two signed 16-bit halves:

| Marker high half | Meaning |
|------------------|---------|
| `0` | **disabled** — skip this slot entirely |
| `< 0` | **direct call**: `fn(entity + marker_low)` (the ubiquitous `0xFFFF0000` = call with offset 0) |
| `> 0` | **indexed**: treat `fn` as an entity-relative table, pick an entry, call through it |

The profound part is what this *replaces*. There is no `enum EntityState`, no
giant `switch (state)` at the top of each update function. **An entity's state
simply IS the function pointer currently sitting in its tick slot.** A walking
enemy has `EntityEventHandlerWalk` in its slots; an idle one has the idle handler.
"Transitioning to the walk state" means *overwriting the slot* with a new function
pointer. The proof that this is the dominant pattern: the function that installs a
new state, `EntitySetState` (`0x8001eaac`), is called from **146 different places**
across the codebase. The game is, mechanically, a few thousand function pointers
being swapped in and out of slots.

### The default turn

Most sprite entities don't write a tick function from scratch; they install the
stock one, `EntityUpdateCallback` (`0x8001cb88`), which does the bookkeeping every
animated thing needs:

1. **`TickEntityAnimation`** — count down the current frame's delay timer and, when
   it expires, advance to the next animation frame.
2. **Dispatch the render slot** (`+0x1C/+0x20`) — recompute the entity's on-screen
   position from its world position and the camera, and re-upload its texture to
   VRAM if the frame changed.
3. **Commit pending state** — if any change was *requested* this frame, apply it now.

That third step is a discipline worth calling out. When game code wants to change
an entity's animation frame, it doesn't write the frame directly — it stashes the
request in a "pending" field and raises a bit in `animChangeFlags`. The actual
write happens at *one* fixed point, here, at the end of the tick. It's the same
instinct as the think/draw split from Section 1, one level down: **request during
the chaos, commit at a single deterministic moment**, so nothing ever observes the
entity half-changed.

A custom entity layers its behaviour *on top* of this. The player's tick
(`PlayerTickCallback`) reads the controller, runs physics, picks a state — and
then calls `EntityUpdateCallback` to do the shared animation/render upkeep. An
enemy's AI tick does the same. The stock turn is the floor everyone builds on.

### Changing your mind mid-turn

When an entity *does* decide to become something else, it calls `EntitySetState`,
and the operation is more careful than a single pointer write. A state isn't one
slot — it's a *set* of them. Watch a real enemy switch into its walk state
(`EntityStateSetWalk`); it installs three callbacks and a sprite in one breath:

```c
// render slot: cleared (this state has no render hook)
// tick  slot: AIEntityRandomBehaviorTick   <- per-frame behaviour
// event slot: EntityEventHandlerWalk        <- how it reacts to messages
SetEntitySpriteId(e, spriteIds[1], 1);       // <- the walking sprite
```

`EntitySetState` also runs an **exit hook** first: the slot at `+0xA8/+0xAC` holds
an optional finalizer for the *outgoing* state — stop a looping sound, restore a
hitbox, re-enable input — which fires and clears before the new state moves in.
States can also queue *sequences*: a death or transformation plays a scripted run
of `[marker, fn]` entries one tick at a time (`EntityProcessCallbackQueue`,
`0x8001e928`), each entry handing off to the next when its animation completes.
Cutscene-grade choreography, built from the same 8-byte pairs as everything else.

So a single tick of one entity can: advance an animation, move, decide to change
state, run the old state's exit hook, install three new callbacks, and swap its
sprite — and the *next* tick it will be running entirely different code, because
the pointers in its slots are no longer the same ones. The entity didn't change
what it *is*; it changed what it *runs*.

---

## Section 4 — How an enemy decides anything

Now the interesting part. An idle monkey on a ledge has to, somehow, *decide* to
start walking, then decide to turn around, then — if you get close — decide to
attack. Where does decision-making live in a machine whose only verb is "call the
function in the slot"?

The answer is in two places, and they correspond to the two ways an entity learns
that time has passed or that the world has changed: the **clock** and the
**mailbox**.

### Decisions on the clock: timers and dice

The everyday behaviour of an enemy is driven by its per-frame tick callback, and
the canonical one is `EnemyAIUpdateWithRandomization` (`0x8004fb58`). Stripped to
its bones, every tick it does this:

```c
void EnemyAIUpdateWithRandomization(Entity *e) {
    e->ai_timer--;                          // count down
    if (e->ai_timer == 0) {                 // time to decide again
        e->ai_behavior = rand() & 7;        // roll a new behaviour, 0–7
        e->ai_timer    = (rand() & 0x1F) + 0x20;  // re-arm for 32–63 frames
    }
    EntityUpdateCallback(e);                // shared animation/render upkeep
    // ...then off-screen culling and collision broadcasts
}
```

This is the whole "AI" of a large fraction of the cast, and it's almost shockingly
modest. There is no pathfinding, no perception model, no behaviour tree. There is
a **countdown** and a **die**. Most ticks the timer just decrements and the enemy
keeps doing whatever it was doing. Every thirty-to-sixty frames the timer hits
zero, the enemy rolls a number, and that number picks its next behaviour — wander
left, wander right, pause, hop. Re-arm, repeat. The randomness is what reads, from
the couch, as *personality*: the enemy seems twitchy and alive precisely because
you can't predict the next roll.

The "decision," concretely, is choosing which state to install — which is to say,
which trio of function pointers to write into the slots. A timer expiry in the
idle handler calls `EntityStateSetWalk`; a wall ahead flips the facing flag; a
zero-HP result calls `EntitySetState` with a death state. Deciding and
state-switching are the same act.

### Decisions in the mailbox: events

Timers handle what an enemy does *on its own*. But an enemy also has to react to
things *done to it* — a fireball lands, the player stomps it, a trigger fires.
These arrive as **events**, delivered to the entity's event slot (`+0x08/+0x0C`)
through the very same marker-dispatch machinery, carrying a 16-bit event id:

| Event | id | Meaning |
|-------|-----|---------|
| `EVT_SPAWN` | `1` | come to life |
| `EVT_TICK` | `2` | a frame has passed |
| `EVT_DAMAGE` | `0x1000` | you were hit |
| `EVT_TOKEN_QUERY` / `CLAIM` / `SET_READY` | `0x1001`–`0x1008` | attack-coordination handshake |

Notice `EVT_TICK` in that list. **The clock is just another message.** Some
entities don't carry a dedicated tick callback at all — their per-frame logic
lives in their event handler, woken by an `EVT_TICK` the engine delivers like any
other event. Time and damage and being-touched all come down the same wire, and an
entity's event handler is a little `switch` that decides how to react to each. To
the engine there is no special "update" verb; there is only message-passing, and
one of the messages happens to mean "a frame went by."

### Emergent coordination: the attack token

The events most worth dwelling on are the `TOKEN` family, because they're where
the AI stops being purely solipsistic and the enemies start, in effect, *talking
to each other*. The problem they solve is a classic one: if every enemy on screen
independently decides to lunge at you the instant you're in range, you get an
unfair, unreadable dogpile. The engine's fix is a **shared attack token**, passed
around through events:

```c
case EVT_TOKEN_CLAIM:              // "may I be the one who attacks?"
    if (token_slot == 0) {         // token is free
        token_slot = attacker;     // take it
        result = 1;                // claim granted
    }                              // else: someone else has it — denied
    break;
case EVT_TOKEN_QUERY:              // "...are you done?"
    if (attacker == token_slot)    // the holder is releasing it
        token_slot = 0;            // token is free again
    break;
```

An enemy that wants to attack must first *claim the token*; if another enemy
already holds it, the claim is denied and the would-be attacker keeps its distance,
circling, waiting. When the holder finishes, it releases the token and the next one
can take a turn. No enemy has any idea how many others exist or where they are —
each only knows whether a single shared slot is occupied — and yet the visible
result is a group that **takes turns**, attacking one at a time while the rest
menace. It's coordination with no coordinator, built from the same three-line
`switch` as everything else in this chapter.

---

## Putting one tick together

Trace a single tick end to end and the whole machine is visible at once:

```
TICK N
  ├─ read controllers           input becomes "held" + "just-pressed" bits
  ├─ GameModeCallback           level transitions, pause, death, checkpoints
  ├─ EntityTickLoop             walk the tick list; for each entity:
  │     ├─ run its tick slot    physics / AI / animation timers
  │     │     ├─ timer hits 0?  roll the dice, pick a new behaviour
  │     │     ├─ wall / ledge?  flip facing
  │     │     ├─ HP == 0?       EntitySetState(death)  — swap the pointers
  │     │     └─ EntityUpdateCallback: advance frame, COMMIT pending state
  │     └─ events delivered      EVT_TICK / EVT_DAMAGE / token handshakes
  ├─ update camera              follow the player, clamp to level bounds
  └─ ─────── world is now settled ───────
        RenderEntities + layers  read the settled world -> ordering table -> screen
```

Two properties fall out of this structure for free, and the engine leans on both.

The first is **determinism**. Because a tick is a pure function of (current state,
this frame's input) — settle everything, *then* draw — the same inputs replayed
against the same starting state reproduce the same game, frame for frame. That is
not a hypothetical: the game ships **recorded-input demos** (the attract-mode
sequences) that are nothing but a stored stream of controller states, fed into this
exact loop in place of a live pad. The one seam in the determinism is `rand()` —
the dice the AI rolls — which is why a faithful replay has to start from the same
RNG seed as the recording. Pin the seed and the input, and the world is a
foregone conclusion.

The second is **uniformity**. Player, enemy, pickup, particle, boss, menu cursor,
even the camera — to `EntityTickLoop` they are identical: a node on a list with a
function pointer to call. There is no privileged "player update" path and no
special-cased enemy manager. Klaymen is the same kind of object as the sparkle
he leaves behind; he merely has more interesting functions in his slots. That
sameness is why the engine is so small for the variety it produces — write one
scheduler, one dispatcher, one state-swapper, and then express *everything*, from a
boss's attack pattern to a coin's idle bob, as different pointers passing through
the same three slots.

Stop the loop and the world doesn't pause so much as *cease* — there is no "current
state of the swamp" to hold, only the entities on a list and the functions in their
slots, waiting for the next turn of the crank that will never come. Start it again
and the monkeys go right back to rolling their dice.

---

## Where the machinery lives

| The mechanism | Function / field | Address |
|---------------|------------------|---------|
| The main loop (one tick) | `main` | `0x800828b0` |
| Per-frame mode logic | `GameModeCallback` | `0x8007e654` |
| The scheduler | `EntityTickLoop` | `0x80020e1c` |
| Tick list / render list / collision list | `GameState +0x1C / +0x20 / +0x24` | — |
| Tick callback slot | `Entity +0x00 / +0x04` | — |
| Event handler slot | `Entity +0x08 / +0x0C` | — |
| Default sprite-entity tick | `EntityUpdateCallback` | `0x8001cb88` |
| Animation frame advance | `TickEntityAnimation` | `0x8001d290` |
| Install a new state | `EntitySetState` | `0x8001eaac` |
| Install an exit/finalizer hook | `EntitySetCallback` | `0x8001ec18` |
| Step a scripted state sequence | `EntityProcessCallbackQueue` | `0x8001e928` |
| Randomized enemy AI tick | `EnemyAIUpdateWithRandomization` | `0x8004fb58` |
| Draw the settled world | `RenderEntities` | `0x80020e80` |
| Event id definitions | `include/Game/entity_events.h` | — |

---

### Further reading (the working notes behind this chapter)

- [Game Loop & Player Creation](../systems/game-loop.md)
- [FSM / Callback Dispatch Patterns](../systems/fsm-callback-patterns.md)
- [Enemy AI System Overview](../systems/enemy-ai-overview.md)
- [Entity Events](../../include/Game/entity_events.h)
- Previous chapter: [What's in a World?](01-whats-in-a-world.md)
- Next chapter: [Making Klaymen Move](04-making-klaymen-move.md)
