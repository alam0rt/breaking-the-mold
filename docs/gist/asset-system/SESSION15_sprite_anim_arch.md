# Session 15 — Sprite animation architecture + sound playback flags

## Sound playback-flags layer (PlaySoundEffect, flag word DAT_8009cc6a from Asset 602)
Per-sound behavior encoded in the 602 flag word:
- **`& 0xf00` = probability gate** (random skip): `0x100`=play 1-in-4, `0x200`=1-in-2,
  `0x400`=3-in-4. Used for sparse/occasional one-shots (drips, ambient hits).
- **`& 0xf0` = pitch randomization**: `0x10/0x20/0x40` apply rand()-based pitch spread over
  different ranges (footstep/impact variation — "vary pitch each play").
- **`& 1` = loop flag** (pitch base 0x400 -> 0x800).
- Voice alloc: round-robin over 24 SPU voices (SpuGetKeyStatus/SpuSetKey); pan via FUN_8007c818
  (divide-by-10 pan curve). 70-entry SPU table cap.
=> distinguishes looped vs pitch-varied vs probability-gated sounds — behavioral metadata for
the whole SFX set (lives in Asset 602 data, per sound).

## Sprite animation architecture (SetupAlternateEntitySpriteContext @ ~0x80033xxx)
An entity sprite is NOT one image. Its loaded data (entity+0x100) holds:
- +0x1C frame count, +0x38 compare value, **+0x04 = anim-sequence-1 ID**, **+0x20 = seq-2 ID**.
- Two render types by sprite-metadata flag (0x180104 / 0x1498810C / 0x8148080):
  Type A (0x60-byte ctx, vtable 0x80010B00, has secondary obj) vs Type B (0x5C, 0x80010AE8).
- `FUN_800255c8(GameState, seqID_u16, ...)` resolves each sequence — **seqID is a u16 INDEX**
  (via FUN_8007b850/80c against the loaded sprite), NOT a 32-bit hash.
  => Asset 503 animation sequences are **index-based, not name-hashed**. No hidden second tier
  of hash-named sequence assets exists to crack. (Closes that avenue cleanly.)

## THE KEY TABLE: 0x8009b144 = entity-type -> sprite-ID lookup
`SetEntitySpriteId(entity, *(u32*)(&DAT_8009b144 + entity_type*4), 1)` (line 11983).
- A **4-byte-per-entry array indexed by entity type** holding the sprite HASH for every type.
- This is the COMPLETE entity-roster -> sprite map (all 121 types).
- It's a **data-segment array (DAT_8009b144)** — the decomp .c shows only the access, not the
  values. **Dumping these bytes from the SLES EXE data section would give entity-type ->
  sprite-id for the whole roster**, tying every uncracked entity sprite to its type slot.
  Concrete, high-value extraction target (needs the raw EXE, not the .c).

## Net
- Sound behavior-flag system decoded (probability/pitch/loop) — explains _01/_02 variant groups
  and ambient sparseness.
- Sprite anim architecture decoded: entity = frame-count + 2 indexed sub-sequences; sequences
  are u16 indices (not hashes) — that crack avenue is closed, definitively.
- Identified `0x8009b144` as the master entity-type->sprite-id table; flagged for EXE data dump.

## Files
- (no new artifact; findings documented here. Next concrete step: dump 0x8009b144 from SLES EXE.)
