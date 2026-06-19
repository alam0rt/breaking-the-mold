# Session 21 — game-loop.md + entity-identification.md harvest

Mined two RE doc pages (treated as LEADS, not ground truth — conflicting claims dropped).

## Conflicts dropped (doc contradicts our VERIFIED names — kept ours)
- 0x6A351094: doc says "Sparkle effect (type 61)"; we have **verified <PREFIX>UNIVERSE_ENEMA_1**
  (PICKUP namespace, exact math) + InitTimerDisplayEntity. Kept ours.
- 0x8C510186: doc says "Skullmonkey Standard walker (type 25)"; we have **verified <PREFIX>FARTHEAD**.
  Kept ours. (Doc's type->sprite is inferred; pickup-namespace math is exact.)

## 18 clean role additions (IDs we had NOTHING for) -> doc_entity_roles.json
Enemies (cross-validate our sister-clusters!):
- 0x004A981C / 0x024E981C / 0x425A399C = **Skullmonkey Fast walker (BLB type 27)** — note
  0x004A981C+0x024E981C are our session-6 "flapper" cluster -> actually the fast monkey enemy.
- 0x0408C01E = Skullmonkey Patrol (type 10)
- 0x004C9138 / 0x40489938 = **Flying enemy (type 26)** — also a known sister-pair.
- 0x88783718 / 0x8818A018 = Flying Monkey Hover/Pattern (also boss-part sprites, type 28/51).
- 0x80B92212 = Snow/Particle (type 64).
Collectibles: 0x09406D8A = Clayball (type 2), 0x0C34AA22 = Item/1-up (type 8).
Menu: 0x10094096 = menu button (reused x4 + password back), 0x68C01218/3080840D/3080820D/
30808E0D/38A0C119 = main-menu BG layers 1-5, 0x40B18011 = Klaymen-head bonus anim.

## New system facts
- **Menu has 4 stages**: 1 Main Menu (FUN_80076ba0), 2 Password Entry (FUN_80077068),
  3 Options/color-picker (FUN_800771c4), 4 Load Game/3 slots (FUN_800773fc).
- **PSX controller button bitfield** documented (Select=0x01 ... R1=0x800, DPad Up=0x1000,
  Right=0x2000); input struct @ 0x800259d4 with held/pressed/demo-playback fields.
- **Alternate Entity Spawning** (GameState+0x164, 128-byte stride): parallel to Asset 501,
  for animated tiles/particles/decor. Source struct: +0x00 entity_type (->0x8009b144 sprite
  lookup), +0x04 anim_seq_1, +0x1C frame_count, +0x20 anim_seq_2.
- **Vtable hierarchy** (8 classes): 0x80010AB8 alt-entity, 0x80010B00/AE8 sprite ctx A/B,
  0x8001044C full+anim, 0x8001039C basic, 0x800104AC menu, 0x80010AA8 colored overlay.
- **CD audio**: PauseCDAudio/ResumeCDAudio (CdlPause 0x09 / CdlResume 0x1B) + 24-voice SPU
  pitch save/mute on pause.
- **SEVN = "1970's secret bonus"**: BLB types 201-228 (0xC9-0xE4) all remap to internal 95,
  original type stored at entity+0xC as variant (one handler, many retro collectibles).
- Ground-enemy sprite TABLES @ 0x8009da50/5c/68 (values in .data; doc lists them).

## Net
- +18 clean entity ROLE labels (11 were previously totally dark) — esp. ties enemy NAMES to
  our sister-cluster sprite IDs (fast-monkey, flying-enemy clusters identified).
- Menu 4-stage structure, controller mapping, alt-entity system, vtable hierarchy, CD audio,
  SEVN bonus mechanic all documented.
- Discipline: dropped 2 doc claims conflicting with our verified names.

## Files
- `doc_entity_roles.json`
