---
title: "Throwing Clay"
category: book
tags: [book, combat, projectiles, weapons, damage, death, powerups, fixed-point]
---

# Throwing Clay

> *A note on names.* The clean-room caveat holds hardest in this chapter, because
> combat is where the older notes and the game's own menus disagree most. Names
> like `SpawnAngledProjectile` and `PlayerState_DamageKnockback` are behaviour
> guesses; the powerup slot numbers and the trig math are read off the code and are
> firmer. Where a value is genuinely unknown, this chapter says "unknown" rather
> than inventing one — and where the game's *own cheat menu* mislabels things (it
> does, spectacularly), we trust the code over the label.

Klaymen can now move through the world and land on it. But a platformer is a
conversation of force: the world throws enemies and hazards at you, and you throw
something back. This chapter is about that exchange — how Klaymen **deals** damage
(a bullet of clay, and a small arsenal of stranger weapons) and how he **takes** it
(no health bar, just lives, mercy frames, a knockback, and a respawn). It's the
chapter where the earlier machinery finally does something violent:

1. **Firing a bullet** — turning an angle into a projectile, with trig and
   fixed-point.
2. **The arsenal** — the six weapons and two shields, and where they're counted.
3. **Getting hit** — lives, not hit points, and the mercy-frame lie revisited.
4. **Dying and coming back** — the death-and-respawn state dance.

---

## Section 1 — Firing a bullet

Press Circle with ammo in the tank and Klaymen throws a **Green Bullet** — a little
projectile of clay. The whole act is one function, `SpawnAngledProjectile`
(`0x80070414`), and it's a beautiful little demonstration of every trick from
Chapter 4 pointed in a new direction: instead of moving a body, it *launches* one.

The problem it solves: you have an **angle** (which way to throw) and a **speed**
(how hard), and you need an X and a Y velocity. That's trigonometry — `vx = cos θ`,
`vy = sin θ` — but there is no floating point and no `math.h` on a PlayStation.
Skullmonkeys uses the PSY-Q **fixed-point trig** routines, `ccos`/`csin`
(`0x8008ce7c` / `0x8008d100`), which take an angle in a **4096-per-circle** unit
(so `0x1000` is a full turn, `0x400` is 90°) and return a sine/cosine already scaled
by 4096 — i.e. `1.0` comes back as `0x1000`. Divide by 4096 (a `>> 12` shift) after
multiplying and you're back in normal units:

```c
void SpawnAngledProjectile(Entity *player, uint angle, int speed) {
    int a = 0xC00 - (angle & 0xFFFF);      // rotate into the game's frame
    int vx = (ccos(a) * speed) >> 12;      // cos * speed, undo the 4096 scale
    int vy = (csin(a) * speed) >> 12;      // sin * speed

    s16 spawn_x = player->worldX + vx;     // muzzle: player + one step of travel
    s16 spawn_y = player->worldY - vy;     // (screen Y is inverted, so subtract)

    Entity *p = AllocateFromHeap(heap, 0x114, 1, 0);   // a small 276-byte entity
    InitProjectile(p, /*sprite*/ 0x168254b5,
                   spawn_x, spawn_y,
                   (vx << 16) >> 6,        // -> 16.16 velocity, then scaled down
                   (vy * -0x10000) >> 6);  // (negated: up is negative Y)
    AddEntityToSortedRenderList(g_pGameState, p);       // Chapter 1's OT list
}
```

Three things to notice, each an echo of an earlier chapter.

**The velocity is 16.16, same as Klaymen's.** The `(vx << 16) >> 6` turns the pixel
velocity into the fixed-point form from Chapter 4 — the bullet integrates its
position with the *exact same* `(whole<<16 | frac)` accumulator idiom every frame,
because it's just another entity and there's only one way to move in this engine.

**It's a full entity, dropped onto the lists.** The bullet isn't a special "bullet
system" — it's `AllocateFromHeap`'d (a lean 276 bytes vs. a full entity's 0x44C),
handed the projectile sprite hash `0x168254b5` (Chapter 1's type→hash→sprite join),
and spliced onto the sorted render list. From that instant it ticks, draws, and
collides like everything else. The clay bullet is the sparkle is the monkey is
Klaymen — different functions in the same slots (Chapter 2).

**One function, many patterns.** Because the launch is parameterised by angle and
speed, the *same* function makes a straight shot (call it once, angle forward) or a
radial burst — the code that fires eight bullets in a ring just calls it in a loop,
stepping the angle by `0x200` (45°) each time:

```c
for (uint angle = 0; angle < 0x1000; angle += 0x200)   // 0x1000 = full circle
    SpawnAngledProjectile(player, angle, (angle >> 9) + 0x10);
// -> 8 bullets at 0°, 45°, 90°, … 315°  (the "explosion" pattern)
```

Aim is just a number; a spray is just a loop over that number.

---

## Section 2 — The arsenal

The Green Bullet is Klaymen's staple, but Skullmonkeys is generous with weirder
ordnance, and the whole inventory is nothing more than a **row of counters in the
player-state block** (`g_pPlayerState`), one byte or bit each. There is no "weapon
object"; owning the Phoenix Hand means the byte at `[0x14]` is nonzero, and firing
it decrements that byte.

| Weapon / item | Slot | Max | Button | What it does |
|---------------|------|-----|--------|--------------|
| Green Bullets | `[0x13]` | 20 | Circle | the staple clay shot (Section 1) |
| Phoenix Hand | `[0x14]` | 7 | L1 | a homing bird attack |
| Phart Head | `[0x15]` | 7 | L2 | a ghost clone that scouts ahead |
| Universe Enema | `[0x16]` | 7 | R1 | screen-wide destruction |
| Super Willie | `[0x1C]` | 7 | R2 | auto-collects every item on screen |
| Hamsters | `[0x1A]` | 3 | — | orbiting shield, soaks 3 hits |
| Halo | `[0x17]` bit 0 | 1 | — | one-hit shield |

Firing anything is the same three-step transaction every time: **check the counter,
act, decrement the counter.**

```c
if (g_pPlayerState[0x13] == 0) return;   // no Green Bullets -> can't shoot
SpawnAngledProjectile(player, forward, speed);
g_pPlayerState[0x13]--;                  // spend one
```

The two *defensive* items (Hamsters, Halo) invert the pattern — they're counters
that get decremented not when you press a button but when you'd otherwise take a
hit, which is the bridge to the next section.

> **A cautionary tale about names.** The game's own cheat menu labels two of these
> backwards: the cheat captioned "Max Swirly Q's" actually fills your Green Bullets
> (`[0x13] = 20`), and the one captioned "Max Green Bullets" actually gives you
> Hamsters (`[0x1A] = 3`). This is a perfect miniature of the whole project's
> epistemics: a label — even one shipped by the original developers, in the retail
> binary — can simply be wrong, and the only ground truth is what the code *does*
> with the byte. (The Swirly Q at `[0x1B]`, incidentally, isn't a weapon at all —
> it's the counter for the secret ending; collect 48 and something changes.)

---

## Section 3 — Getting hit

Now the receiving end. Here Skullmonkeys makes a choice that shapes its whole feel:
**there are no hit points.** Klaymen doesn't have a health bar that whittles down —
he has *lives* (a single byte at `g_pPlayerState[0x11]`, starting at 5, max 99), and
the rule is brutally simple: **one hit is one life.** Touch an enemy, catch a
fireball, land on a spike — the distinction between "a little hurt" and "dead"
doesn't exist. You were fine, and now you have one fewer life and you're being
knocked backwards.

That "hit" arrives the way *everything* arrives in this engine: as an **event**
(Chapter 2's mailbox). When an enemy's body overlaps Klaymen, the collision walk
dispatches a damage event into his event slot, and his handler decides what it
means. And the *first* thing it checks is whether the hit counts at all — because of
the two gates that stand between a collision and a lost life:

**The mercy-frame gate.** After any hit, an **invincibility timer** runs, and while
it's nonzero, incoming damage is simply ignored. This is the exact same mechanism
Chapter 4 met from the *tile* side — where a death tile is quietly rewritten to the
safe-zone value `0x65` during i-frames. Here it's the entity side of the same lie:
the hit is real, the timer says "not now," and the hit evaporates. It's why you can
run through a cluster of enemies right after getting tagged and only lose one life,
not five.

**The shield gate.** If a defensive item is up, it eats the hit instead of a life.
A **Halo** (`[0x17]` bit 0) is a one-shot: the hit clears the bit and you're fine.
**Hamsters** (`[0x1A]`, up to 3) are a small stack — each hit spends one before any
life is touched. Only when both gates pass — no i-frames, no shield — does the
counter at `[0x11]` actually drop.

When a hit *does* land, Klaymen enters a **damage-knockback state**
(`PlayerState_DamageKnockback`, `0x8006b4f4`, via the knockback physics at
`PlayerCallback_KnockbackPhysics`, `0x80066990`). This is a *state* in the Chapter 2
sense — `EntitySetState` swaps his tick/event pointers to the knockback code — and
it's built on the Chapter 4 physics: a knockback velocity is loaded (away from the
attacker, up and back), and the same fixed-point integrator that handles a jump now
handles being launched. Control is taken away for the duration; the i-frame timer is
armed; the sprite flashes (an RGB modulation, so you can *see* you're briefly
untouchable); and when the arc finishes he lands and control returns. Damage,
mechanically, is just another movement state that you didn't ask for.

> **What's honestly unknown here.** The *amount* of damage per source is barely
> pinned down. The code reads a base value from the attacker (`entity+0x44`) and
> halves it in shrink mode (`entity+0x16 == 0x8000`), which strongly implies the
> engine once had graded damage — but in practice almost everything resolves to
> "one life," and the per-enemy values haven't been extracted. This is a genuine
> gap, not a simplification; treat the "1 life" rule as observed behaviour and the
> damage-value machinery as scaffolding whose numbers we don't yet know.

---

## Section 4 — Dying, and coming back

When the last gate falls and it was your *last* life — or you hit an instant-death
zone, which skips the whole lives dance — Klaymen dies, and death is its own little
state sequence, exactly the scripted `[marker, fn]` runs from Chapter 2.

The death itself is a state (`PlayerDeathGrowingTick`, `0x8005334c`) that plays the
death animation and spawns the death effect (`SpawnPlayerDeathEffect`,
`0x8005b2d4`) — the burst of particles that are, of course, just more entities
dropped onto the lists. There's a nice engine-level detail here noted in the symbol
table: at the moment of death the player's **vtable is swapped to a "destroyed"
table** and only restored a couple of frames into respawn, so the dying and
just-respawned Klaymen literally runs *different code* than the live one — the same
"state is a pointer" idea, applied to death.

Then **respawn** (`RespawnAfterDeath`, `0x8007cfc0`) runs the reverse ceremony:
decrement lives, clean up the entities the old life left lying around
(`CleanupRespawnEntities`, `0x8007eed8` — you don't want the previous attempt's
bullets and particles persisting), and re-place Klaymen at the last **checkpoint**.
Checkpoints, recall from Chapter 4, are just tile attribute `0x53` — stepping on one
armed a respawn position — so "coming back" is: read the saved checkpoint, set
position, arm a fresh stretch of invincibility (`PlayerState_ClearCheckpointAndSet‑
Invincible`, `0x8006c7cc`), and swap the tick pointer back to the normal player
state. The world, meanwhile, never stopped — the tick list kept walking (Chapter 2);
only Klaymen's own entity cycled through death and rebirth.

Run out of lives entirely and there's no respawn to run — the mode callback
(Chapter 2's `GameModeCallback`) notices the lives counter hit zero and transitions
the *whole game* to the game-over screen, which is a different mode with a different
tick list entirely.

---

## Putting one hit together

Combat, start to finish, is two entities exchanging events and a fistful of
counters changing:

```
KLAYMEN ATTACKS
  ├─ button pressed + counter > 0 ?      [0x13] Green Bullets, [0x14] Phoenix, …
  ├─ angle + speed -> vx, vy             ccos/csin, >>12  (fixed-point trig)
  ├─ spawn projectile entity             276 bytes, sprite 0x168254b5, 16.16 velocity
  │     └─ onto the tick/render/collision lists (Chapter 2)  -> OT (Chapter 1)
  └─ counter--                           spend the ammo

KLAYMEN IS HIT   (an EVT_DAMAGE event lands in his event slot — Chapter 2)
  ├─ invincible?      i-frames running -> ignore the hit entirely
  ├─ shield up?       Halo (1) or Hamsters (≤3) -> spend that instead of a life
  ├─ else lives--     [0x11], starts at 5
  │     ├─ lives left?  -> DamageKnockback state: launch back, flash, i-frames
  │     │                   (Chapter 4 physics, driven by a Chapter 2 state swap)
  │     └─ lives == 0?  -> death sequence -> respawn at checkpoint (tile 0x53)
  │                          or, out of lives, -> game-over mode
  └─ world keeps ticking throughout — only Klaymen's entity cycles
```

Step back and combat added almost no new machinery — that's the point. Dealing
damage is *the movement chapter's fixed-point, aimed*: an angle through a trig table
becomes a velocity, a velocity becomes an entity, an entity rides the same lists and
the same ordering table as everything else. Taking damage is *the tick chapter's
events and states, weaponised*: a collision becomes a message, a message becomes a
state swap, a state swap becomes a knockback arc built from the same integrator as a
jump. Even death and respawn are just pointers swapping in and out of slots while the
world's clock never pauses.

The only genuinely new *ideas* in this chapter are design ones, not engine ones:
that health should be **lives, not points** (one hit, one life — no chip damage, no
bar to babysit), and that fairness comes from **mercy frames and stackable shields**
rather than from a damage economy. Skullmonkeys spends its cleverness on *feel*, and
implements that feel with the plainest possible parts — a handful of counters and the
same three callback slots doing yet another job.

---

## Where the machinery lives

| The mechanism | Function / field | Address |
|---------------|------------------|---------|
| Fire an angled projectile | `SpawnAngledProjectile` | `0x80070414` |
| Fixed-point cosine / sine | `ccos` / `csin` | `0x8008ce7c` / `0x8008d100` |
| Projectile sprite hash | — | `0x168254b5` |
| Ammo / weapon counters | `g_pPlayerState[0x13…0x1C]` | — |
| Lives counter | `g_pPlayerState[0x11]` (u8, default 5) | — |
| Halo / Hamsters shields | `g_pPlayerState[0x17]` bit0 / `[0x1A]` | — |
| Damage-knockback state | `PlayerState_DamageKnockback` | `0x8006b4f4` |
| Knockback physics | `PlayerCallback_KnockbackPhysics` | `0x80066990` |
| Death (growing) tick | `PlayerDeathGrowingTick` | `0x8005334c` |
| Spawn death effect | `SpawnPlayerDeathEffect` | `0x8005b2d4` |
| Respawn after death | `RespawnAfterDeath` | `0x8007cfc0` |
| Clean up old-life entities | `CleanupRespawnEntities` | `0x8007eed8` |
| Clear checkpoint + i-frames | `PlayerState_ClearCheckpointAndSetInvincible` | `0x8006c7cc` |
| Checkpoint tile attribute | — | `0x53` |

---

### Further reading (the working notes behind this chapter)

- [Projectile & Weapon System](../systems/projectiles.md)
- [Combat & Damage System](../systems/combat-system.md) · [Damage (complete)](../systems/damage-system-complete.md)
- [Checkpoint System](../systems/checkpoint-system.md)
- [Secret Ending System](../systems/secret-ending-system.md) (the Swirly Q counter)
- [Physics Quick Reference](../PHYSICS_QUICK_REFERENCE.md) (projectile constants)
- Previous chapter: [Making Klaymen Move](04-making-klaymen-move.md)
