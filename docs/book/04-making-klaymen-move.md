---
title: "Making Klaymen Move"
category: book
tags: [book, player, physics, movement, collision, camera, fixed-point]
---

# Making Klaymen Move

> *A note on names.* Same clean-room caveat as the earlier chapters: names like
> `velocityY_fixed`, `carryMotionX`, and `PlayerCallback_FallingPhysicsMain` are
> inferred from behaviour and can be wrong. The byte offsets, the fixed-point
> arithmetic, and the tile-attribute numbers are read off the running machine and
> off the disassembly — those are the parts to trust. Where this chapter is
> unusually confident, it's because a *second* implementation — a work-in-progress
> PC port that has to actually run — agreed with the reading. Where the port
> disagreed with our older notes, the port won, and this chapter says so.

The last two chapters built a world and gave it a heartbeat. But the whole point
of Skullmonkeys is one specific thing moving through that world: a lump of clay
with legs, falling, running, bonking his head, sliding down slopes, and landing on
platforms. This chapter is about how *one entity* — the player — turns a held
button and the pull of gravity into a new position on screen, sixty times a
second, without ever tunnelling through the floor.

It sounds simple. It is not, and the reasons it's not are the reasons every 2D
platformer of this era reads the way it does. Four questions:

1. **How do you move less than a pixel?** — sub-pixel positions and why they exist.
2. **How does gravity work?** — the fall integrator, and the accumulator bug that
   teleported the player to the corner of the map.
3. **How do you not fall through the floor?** — probing tiles instead of testing
   boxes.
4. **How does the camera keep up?** — easing, not snapping.

---

## Section 1 — Moving less than a pixel

Start with a problem the PlayStation hands you and won't help with: **the screen
is made of whole pixels, but motion isn't.** A character that walks at "two and a
half pixels per frame" has to advance 2, then 3, then 2, then 3 — and if you round
that off every frame you get either a character that visibly stutters or one whose
speed is quietly wrong. Worse, gravity adds a *tiny* amount to falling speed each
frame — a fraction of a pixel — and if fractions can't be stored, gravity can't
accelerate. The character would fall at a fixed integer speed like a lift, not a
body.

The universal 1990s fix, and Skullmonkeys' fix, is **16.16 fixed-point**: store a
position not as an integer number of pixels but as a 32-bit number where the top
16 bits are whole pixels and the bottom 16 bits are the *fraction* of a pixel.
`0x0002_8000` is "pixel 2 and a half." You do all your arithmetic on the full
32-bit value — adding a speed of `0x0002_8000` (2.5 px) per frame accumulates
correctly, fraction and all — and only when it's time to *draw* do you throw away
the bottom 16 bits and use the whole-pixel part.

The player entity stores this split across two pairs of fields, and getting the
split right is the whole game:

| Offset | Field | What it really is |
|--------|-------|-------------------|
| `+0x68` | `worldX` (s16) | **whole-pixel** X position |
| `+0x6A` | `worldY` (s16) | **whole-pixel** Y position |
| `+0x6C` | (`velocityX`?) | the **sub-pixel fraction** of X — *not* a velocity |
| `+0x6E` | (`velocityY`?) | the **sub-pixel fraction** of Y — *not* a velocity |

That last pair is a trap the project fell into and had to climb out of, so it's
worth stating flatly. The fields at `+0x6C/+0x6E` were long *labelled* velocity —
the name is even still on them in the header, flagged as wrong — but they are the
low 16 bits of the position accumulator. Klaymen's real velocity lives
somewhere else entirely: `velocityX_fixed @ +0x114` and `velocityY_fixed @ +0x110`,
both full 16.16 values. Having "velocity" apparently stored in *two* places was the
tell that one of the labels was a lie.

The reassembly is a single idiom you will see over and over, in the player, the
camera, and every moving entity:

```c
// pull the full 32-bit position back together...
int32_t pos = (worldX << 16) | (uint16_t)fracX;   // whole<<16 | fraction
pos += velocityX_fixed;                            // move by a 16.16 velocity
// ...and split it back apart
worldX = pos >> 16;                                // whole pixels  -> draw with this
fracX  = pos & 0xFFFF;                             // remembered fraction -> next frame
```

Whole pixels up top, fraction stashed for next frame, no motion lost to rounding.
Hold this picture; the next section is what happens when you get the *type* of that
`pos` variable wrong.

---

## Section 2 — Gravity, and the bug at the corner of the map

When Klaymen is not on the ground he runs `PlayerCallback_FallingPhysicsMain`
(`0x800650c4`) — at `0x18cc` bytes, the single largest function in the player code,
because "being in the air" is where most of the interesting physics happens.
Stripped to its spine, the vertical half is exactly the accumulator idiom from
Section 1, with gravity added in:

```c
// reassemble Y position (whole<<16 | fraction)
pos_y = (worldY << 16) | (uint16_t)fracY;

velocityY_fixed += 0x5000;                  // gravity: 0.3125 px/frame², downward
if (velocityY_fixed > 0x80000)              // terminal velocity: 8.0 px/frame
    velocityY_fixed = 0x80000;              // cap the fall

pos_y += velocityY_fixed;                   // integrate
worldY = pos_y >> 16;                       // new whole-pixel Y
fracY  = pos_y & 0xFFFF;                    // carry the fraction
```

Gravity is a constant `0x5000` (`0.3125 px/frame²`) *added to the velocity* every
frame, so the fall speed ramps up smoothly — a third of a pixel-per-frame faster
each frame — until it hits a terminal cap of `0x80000` (`8.0 px/frame`) and holds.
That ramp is only expressible *because* the velocity is 16.16: `0.3125` isn't a
whole number, and without the fractional bits it would round to 0 and Klaymen would
hang in the air.

> These are the values read from `PlayerCallback_FallingPhysicsMain` itself. An
> older note (the [Physics Quick Reference](../PHYSICS_QUICK_REFERENCE.md)) lists
> gravity as `0x8000` — but that's from a *different*, generic-entity fall routine
> (`EntityFallingGravityWithCollision`), not Klaymen's airborne path. When the two
> disagree, trust the function that actually runs for the thing you're watching.

The horizontal half is the same shape but carries an extra term, `carryMotionX`
(`+0x10C`) — momentum handed to you by a moving platform you were standing on, so
you keep the platform's motion for a moment after you step off, decaying by
`0x4000` per frame toward zero. It's the reason a jump off a moving lift carries
you the way you'd expect.

### The teleport-to-origin bug

Here is where this chapter earns its "the port won" caveat, because the fix is
recent and the failure was spectacular.

When the airborne function was first machine-translated to C, the tool guessed the
type of the X-position accumulator — the `pos_x` in the idiom above — as a **16-bit
`short`**. Re-read the reassembly line with that type in mind:

```c
short pos_x = (worldX << 16) | fracX;   // BUG: worldX<<16 in a 16-bit box
```

`worldX << 16` shifts the whole-pixel position *out of the bottom 16 bits
entirely* — into the top half of a 32-bit register, where it's supposed to live.
But a 16-bit variable has no top half. The shift threw the entire whole-pixel
position away and kept only the fraction. On the very first airborne frame the
player's X collapsed from its spawn column (around x=328) to **0**, and he
teleported to the left edge of the level and idled there off-screen forever. The
camera, dutifully following, eased toward the wrong spot too, so the whole frame
was wrong in a way that *looked* like a camera bug but wasn't.

The original machine code never had this problem: it kept the accumulator in a
full 32-bit register (`sra v0, a3, 16` to extract the whole pixels; `sh a3, 0x6C`
to store only the fraction), which is precisely the `pos >> 16` / `pos & 0xFFFF`
split. The fix was one character of type — `short` → `int` — and it's the whole
reason `+0x6C/+0x6E` now carry a "this is a fraction, not a velocity" comment in
the header. **The sub-pixel split isn't a nicety; get the width of the accumulator
wrong and the character doesn't jitter, he disappears.**

For the exact constants — walk speed, jump velocity, gravity, terminal velocity,
the landing cushion — see the [Physics Quick Reference](../PHYSICS_QUICK_REFERENCE.md);
this chapter is about the machinery those numbers flow through.

---

## Section 3 — Not falling through the floor

Now the hard part. Klaymen has a new position — but is he allowed to be
there? Between last frame and this one he might have moved far enough that his feet
are now *inside* a solid tile, and if you just let him sit there he'll clip into
the ground or fall straight through it. Every platformer needs an answer to "having
moved, am I now overlapping something solid, and if so, where do I get pushed to?"

Skullmonkeys' answer is **point probing**, and it's cheaper and more surgical than
testing the player's whole bounding box against the world. Instead of asking "does
my rectangle overlap any solid tile," the physics code asks a handful of very
specific yes/no questions — *is there a solid tile at this exact spot?* — at points
chosen around the body:

- **Feet / floor:** probes just below the feet, and one tile down, to find the
  ground to stand on (and to detect the lip of a slope).
- **Head / ceiling:** a probe up around `Y-64` — bonk here while rising and the
  upward velocity is killed.
- **Walls:** a *stack* of probes up the leading edge — roughly `Y-15`, `Y-16`,
  `Y-32`, `Y-48` — so a wall stops you whether it's knee-high or head-high, and a
  one-tile gap you could squeeze through doesn't read as a wall.

The tool that answers each probe is `EntityApplyMovementCallbacks`
(`0x8001fea8`) → `GetTileAttributeAtPosition` (`0x800241f4`): hand it a world (x, y)
and it returns the **collision attribute byte** of the tile there. The airborne
function calls it many times per frame — one call per probe point, at several
heights — which is why "check the floor" in the code is a little ladder of nearly
identical `EntityApplyMovementCallbacks(e, x, y - 0x10)`, `… y - 0x20`, `… y - 0x30`
calls, each testing a different rung of the body.

Why probe *through* the movement callbacks and not read the tilemap directly? Two
reasons from earlier chapters. First, an entity's on-screen position is its world
position run through its **move-callback FSM slots** (`+0x24…+0x30`, Chapter 2) —
platform-riding and shrink/grow scaling live there — so the *effective* probe point
has to go through the same transforms the renderer uses, or collision and graphics
would disagree. Second, it keeps the whole engine speaking one language: position
in, transformed position out, everywhere.

### What the tile says back

The byte that comes back is a small language of its own (full table in
[collision-complete.md](../systems/collision.md)). The ranges the falling code
cares about:

| Attribute | Meaning | Response |
|-----------|---------|----------|
| `0x00` | empty | keep falling / moving |
| `0x01`–`0x3B` | **solid** (value encodes slope height) | stop; snap to the surface |
| `0x53` | checkpoint | arm a respawn point |
| `0x65` | spawn / safe zone | treated as non-lethal even mid-hazard |
| `> 0x3B` (other) | **trigger** | fire an event — wind, bounce, death, exit |

The solid range is clever: `0x01`–`0x3B` isn't just "solid yes/no," the *value*
indexes a **slope-height table** (`g_SlopeHeightTable @ 0x8009d228`), so a single
attribute byte says both "you can't pass" and "the surface here is this many
sub-pixels up from the tile's base." That's how Klaymen walks smoothly up a
45° ramp instead of climbing it like a staircase: the floor probe doesn't just find
*a* solid tile, it finds *how high the solid is at his exact sub-pixel X*, and snaps
his feet to that height.

Triggers (`> 0x3B`) aren't surfaces at all — they're messages baked into the map.
Stepping on one doesn't stop you; it *fires an event* (Chapter 2's mailbox), and
the same collision handler routes bounce tiles, wind zones, death pits, and level
exits through the player's event slot. A death tile, notably, is ignored while the
invincibility timer is running (`sp20 = 0x65` — "pretend it was a safe zone") — the
mercy-frames rule, implemented as a one-line lie about what the tile said.

The upshot: **collision here is a conversation, not a physics solver.** The player
asks "what's at my feet? my head? my leading shin?", the map answers with a byte
apiece, and the player's response — stop, snap to a slope, bounce, die, or ignore —
is a small switch on those bytes. No swept volumes, no contact manifolds; just a
handful of point questions and a lookup table, which is exactly as much physics as
a 33 MHz CPU can afford sixty times a second.

---

## Section 4 — The camera keeps up (by lagging behind)

Klaymen now moves and collides correctly. But if the camera were bolted rigidly
to him, every little hop and slope-step would jerk the whole screen, and the world
would feel nauseous. So the camera doesn't *follow* the player; it **chases** him,
and the gap between chase and target is what makes the motion feel like a camera and
not a clamp.

`UpdateCameraPositionSmooth` (`0x800233c0`) runs after the player has settled, and
it's the same fixed-point accumulator machinery from Section 1, applied to the
camera instead of Klaymen. Each frame it:

1. **Finds the target.** Resolve the player's rendered position through the *same*
   move-callback FSM slots the collision probes used (so the camera frames where
   the player is actually *drawn*, scaling and platform-ride included), then offset
   by the hitbox, any glide, and the current camera mode.
2. **Eases toward it.** The difference between where the camera is and where it
   wants to be indexes an **acceleration table** (`0x8009b074` / `b0bc` / `b104`,
   one each for vertical / diagonal / horizontal). Far from target → accelerate
   hard; close → accelerate gently. The camera has *velocity*, stored 16.16 at
   `+0x4C/+0x50`, and eases that velocity toward the target rather than snapping
   position. This is why the view glides.
3. **Integrates with sub-pixels.** Same `(whole<<16 | frac)` split as the player,
   using the camera's own sub-pixel accumulators (`+0x5C/+0x5E`). The camera moves
   fractional pixels for the same reason the player does — otherwise smooth follow
   would stutter.
4. **Clamps to the level.** Re-apply the scroll limits so the camera never shows
   past the edge of the map, then add the screen-shake offset last (shake rides on
   *top* of the eased position, so a rumble doesn't fight the follow).

Notice this closes a loop with Chapter 1. The camera position computed here is the
very thing the tilemap layers read next frame to decide their `DR_OFFSET` scroll and
which edge strip to re-stamp. Player physics moves Klaymen; the camera eases
toward him; the layers scroll to match the camera; the parallax factors fan the
layers out by depth. Four systems, one number — the camera's (x, y) — threaded
between them.

And it's the loop where the teleport bug from Section 2 hurt twice: with the
player's X collapsed to 0, the camera dutifully eased toward the corner of the map
*correctly* — the camera wasn't broken, it was faithfully framing a broken player.
A good reminder that in a pipeline this tightly coupled, the *symptom* (a camera
staring at an empty corner) can be four systems downstream of the *cause* (one
`short` that should have been an `int`).

---

## Putting one player-tick together

Zoom out and a single frame of "being Klaymen" is one pass down a short
pipeline, each stage feeding the next:

```
PLAYER TICK
  ├─ read input                held/pressed bits (Chapter 2)
  ├─ pick horizontal speed      walk 2.0 / run 3.0 px/frame, +boost
  ├─ reassemble position        pos = (worldX<<16) | fracX          ── 16.16
  │     ├─ + velocity, + carryMotionX (platform momentum), + gravity
  │     └─ cap fall at terminal velocity (8 px/frame)
  ├─ split back                 worldX = pos>>16 ;  fracX = pos & 0xFFFF
  ├─ probe tiles                feet / head / wall-stack -> attribute bytes
  │     ├─ solid (0x01–0x3B)?   stop; snap to slope height
  │     ├─ trigger (>0x3B)?     fire event: bounce / wind / death / exit
  │     └─ invincible?          treat death tile as safe (0x65)
  ├─ maybe change state         EntitySetState(...) — swap the pointers (Chapter 2)
  └─ EntityUpdateCallback       advance animation, commit, upload texture (Chapter 2)

CAMERA TICK (after the player has settled)
  └─ ease camera velocity toward the player's rendered position, clamp, shake
        └─ next frame: layers scroll to match (Chapter 1)
```

Two ideas carry the whole thing, and both are older than this game. The first is
**fixed-point** — the fiction that a position can hold a fraction — without which
gravity can't accelerate, slopes can't be smooth, and the camera can't glide. The
second is **point-probing collision** — replacing "does my body overlap the world"
with "what's the tile at *this* spot," asked a handful of times — without which a
33 MHz CPU could not afford to keep Klaymen out of the floor at 60 Hz.

Get both right and a lump of clay falls, runs, and lands like it has weight. Get one
type wrong in the first one and he teleports to the corner of the map and sits there
— which is exactly what happened, and exactly what it took to understand this
chapter well enough to write it.

---

## Where the machinery lives

| The mechanism | Function / field | Address |
|---------------|------------------|---------|
| Airborne physics core | `PlayerCallback_FallingPhysicsMain` | `0x800650c4` |
| Player per-frame tick | `PlayerTickCallback` | `0x8005b414` |
| Apply position + collision | `PlayerApplyPositionWithCollision` | `0x8005a218` |
| Whole-pixel X / Y position | `Entity +0x68 / +0x6A` | — |
| **Sub-pixel** X / Y fraction (mislabelled "velocity") | `Entity +0x6C / +0x6E` | — |
| Real velocity (16.16) X / Y | `Entity +0x114 / +0x110` | — |
| Platform-momentum carry | `Entity +0x10C` (`carryMotionX`) | — |
| Probe a tile through move-FSM | `EntityApplyMovementCallbacks` | `0x8001fea8` |
| Tile attribute lookup | `GetTileAttributeAtPosition` | `0x800241f4` |
| Slope height table | `g_SlopeHeightTable` | `0x8009d228` |
| Semi-solid / one-way override | `CheckTileCollisionOverride` | `0x8005a5c4` |
| Trigger-zone dispatch | `PlayerProcessTileCollision` | `0x8005a914` |
| Smooth camera follow | `UpdateCameraPositionSmooth` | `0x800233c0` |
| Camera velocity (16.16) X / Y | `GameState +0x4C / +0x50` | — |
| Camera sub-pixel X / Y | `GameState +0x5C / +0x5E` | — |
| Camera accel tables (V / diag / H) | — | `0x8009b074 / b0bc / b104` |

---

### Further reading (the working notes behind this chapter)

- [Physics Quick Reference](../PHYSICS_QUICK_REFERENCE.md)
- [Player Physics System](../systems/player/player-physics.md)
- [Tile Collision System](../systems/collision.md) · [Collision (complete)](../systems/collision-complete.md)
- [Camera System](../systems/camera.md)
- Previous chapter: [What's in a World?](01-whats-in-a-world.md) · [Making Things Tick](02-making-things-tick.md)
- Next chapter: [Throwing Clay](05-throwing-clay.md)
