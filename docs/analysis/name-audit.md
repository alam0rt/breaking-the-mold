# Function name/purpose audit

Running log of the clean-room function-name review (see CLAUDE.md: every name is
an inferred guess and may be wrong). Adds purpose comments in-source; this file
tracks **names worth changing** for a later, deliberate rename pass (rename
ripples through `symbol_addrs.txt` + all callers, so it's done separately).

Severity: **PLACEHOLDER** = auto/hash name, no meaning · **MISLEADING** = name
implies behavior the body doesn't have · **NIT** = accurate but could be clearer.

## enemies.c (pass A — decompiled C, in progress)

| Function | Severity | Finding / suggested name |
|---|---|---|
| `EntityEventHandlerWalk` | NIT | Name reflects the *installing state*; body is attack-token arbitration (no walking). Maybe `EntityWalkTokenHandler`. |
| `EntityEventHandlerIdle` | NIT | Same — idle-state token handler (+ EVT_TICK queue pump). |
| `EntityEventHandler0x1001_1002_1008` | PLACEHOLDER | Hash-of-event-ids name. It's the token handler + EVT_TICK +0x110 countdown. (shelved as asm.) |
| `EntityEventHandler0x1001_1002_1008_V2` | PLACEHOLDER | Behaviorally **identical** to `EntityEventHandlerIdle` — a second copy. Rename to convey "idle token handler (copy)". |
