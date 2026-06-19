# Session 16 — The entity-type sprite table + exhaustive sprite-usage harvest

## 0x8009b144 entity-type->sprite table: confirmed NOT in the .c dump
`SetEntitySpriteId(entity, *(u32*)(&DAT_8009b144 + type*4), 1)` reads the master
entity-type -> sprite-id array. Verified it is **read-only static data**:
- never written at runtime (no memcpy/store into 0x8009b1xx anywhere),
- this Ghidra .c export contains **zero array initializers** (`= {` count = 0) — only bare
  `undefined DAT_8009b144;` extern declarations.
=> the table's bytes live in the EXE .data/.rodata, NOT recoverable from this .c. (Neighbors
`DAT_8009b174/180/18c` are separate menu-Klaymen sprite-context pointers, not this table.)

## Recoverable equivalent: exhaustive call-site harvest (sprite_usage_full.json)
Every `SetEntitySpriteId / InitEntitySprite / InitParticleEntity / AllocSpriteRenderContext /
InitEntityWithSprite` call is effectively one row of the entity->sprite map. Swept all of them
with full context (function, call type, z-order):
- **116 distinct sprite ids bound in code** (the code-referenced subset of 412 sprites).
- **103 uncracked but role-known**; **37 have a semantically-named binding function.**

### New/refined role labels this pass
- 0x1C3AA013 PlayerState_LandingFromRide / PickupItem
- 0x002E2414 PlayerProcessBounceCollision (FOOD)
- 0x8AB92024 DisplayTransitionSprite
- 0x393C80C2 PlayerSetupBounceAnimation
- 0x08208902 PlayerState_CheckpointExit · 0x1B301085 PlayerState_Death
- 0x18298210 WalkingLeft · 0x292E8480 WalkingRight/Running · 0x092B8480 Jump · 0x0B2084D0 Falling
- 0x0244655D InitBossEntity · 0x6A351094 InitTimerDisplayEntity · 0x8AB92024 transition
- player variants: 0x21842018 normal, 0x3DA80D13 Finn, 0xCA1B20CB Soar, 0xEC95689B password

## Net
- Master entity-type sprite table located (0x8009b144) but proven un-dumpable from the .c;
  flagged for EXE extraction (484 bytes = 121 types x 4 = entire roster).
- Built `sprite_usage_full.json`: the definitive id->{name,status,funcs,calls,zorders} map for
  all 116 code-bound sprites — the recoverable form of the same entity->sprite information.

## Files
- `sprite_usage_full.json`
