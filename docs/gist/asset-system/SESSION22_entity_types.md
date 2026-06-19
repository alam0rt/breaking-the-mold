# Session 22 — entity-types.md: categorization (not naming)

## Direct answer: helps CATEGORIZE, does NOT give a naming convention
- The doc's 121-entry callback table @ 0x8009d5f8 lets us GROUP entity types by **shared
  callback** (= same entity class): 112 active types -> **82 behavioral classes** (68 unique +
  14 shared groups). Real categorization layer.
- It does NOT advance NAMING: doc entity names are generic placeholders (EnemyA/PlatformA/
  Type00), and it states outright "Sprite IDs are 32-bit hashes that index the sprite TOC" —
  confirms our model, adds no new convention. Its sprite-id guesses are inferred (2 conflicted
  with our VERIFIED names: Sparkle vs UNIVERSE_ENEMA_1, walker vs FARTHEAD — dropped).

## Shared-callback families (one behavior, many types) -> entity_type_callbacks.json
- 0x80080ddc: types 42,43,44,53,54,55,60 = Portal/Particle (largest class, 7 types)
- 0x8008134c: 89,97,98,110,111  · 0x800812ec: 85,104,105
- 0x80080af8: 31,32,33 (Platform A) · 0x80080b60: 34,35,36 (Platform B) · 0x80080e4c: 47,48
- 0x8007efd0: 0,3,4 (default/ammo) · 0x8007f460: 24,118 (SpecialAmmo)
- + decoration groups 86-88, 106-108, 112-114, 115-117.

## Cross-validations (independent confirmation of our names)
- **MSG_UNIVERSE_ENEMA = 0x1018** + "projectile-hit also used by Universe Enema" -> confirms
  FX_PICKUP_UNIVERSE_ENEMA / <PREFIX>UNIVERSE_ENEMA_1 is correctly identified.
- Collision messages: 0x1000 CLAYBALL_COLLECT, 0x1002 PROJECTILE_HIT, 0x1007 CLAYBALL_VARIANT,
  0x1009 NOTIFY_LINKED, 3 COLLECTED.

## Entity population (rarity -> prioritization)
Clayball 5727, Ammo 308, PlatformB 297, SpecialAmmo 227, EnemyA 152, Item 144, PlatformA 99,
EnemyB 60. (Clayball dominates; bosses/specials are rare.)

## Net
- +categorization: 82 behavioral entity classes mapped from the callback table.
- Naming convention: unchanged — doc confirms the hash model but offers no key. The two data
  tables (0x8009b144 entity->sprite, 0x8009dab4 shared sprites) remain the only step-change,
  and need the raw EXE.
- 1 name cross-validated (Universe Enema via collision message).

## Files
- `entity_type_callbacks.json`
