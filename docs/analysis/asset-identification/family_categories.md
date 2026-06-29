---
title: "Asset-family categories (structure + code refs)"
category: analysis/asset-identification
tags: [analysis, asset, identification, family, categories]
---

# Asset-family categories (structure + code refs)
68 families (size>=3). `sequence_style=YES` = members differ by one short sequential token (directional/frame numbering, like the FINN player family).

## Category counts
- **Uncategorized**: 54 families
- **Boss**: 3 families
- **Enemy**: 3 families
- **Menu/UI**: 3 families
- **Clayball/Projectile**: 2 families
- **Player**: 2 families
- **Effect/Particle**: 1 families

## Sequence-style families (highest value)

| stem_hash | n | refd | span | category | functions | levels |
|---|---|---|---|---|---|---|
| 0x10b95810 | 16 | 0 | 9-17 | Uncategorized |  | FINN |
| 0x08ce0610 | 6 | 1 | 24-29 | Uncategorized | InitPasswordEntity | CSTL;FOOD;SEVN;TMPL |
| 0xb0907444 | 6 | 0 | 20-25 | Uncategorized |  | TMPL |
| 0x081c8300 | 5 | 0 | 16-20 | Uncategorized |  | EVIL;FINN;RUNN |
| 0x800da112 | 5 | 5 | 2-6 | Boss | KloggDeathEventHandler | KLOG |
| 0x10b95810 | 5 | 0 | 9-17 | Uncategorized |  | FINN |
| 0x40ca1c10 | 4 | 0 | 24-27 | Uncategorized |  | CRYS |
| 0x463c6040 | 4 | 4 | 17-20 | Clayball/Projectile | ClayballState_DestroyWithDebris | Callback_800570a4 | BOIL;CAVE;CRYS;CSTL;EGGS |
| 0x081c8300 | 4 | 0 | 16-22 | Uncategorized |  | EVIL;FINN;RUNN |
| 0x10b95810 | 4 | 0 | 10-16 | Uncategorized |  | FINN |
| 0x10b9581c | 4 | 0 | 9-15 | Uncategorized |  | FINN |
| 0x40ca1c10 | 4 | 0 | 20-26 | Uncategorized |  | CAVE |
| 0x4c22d240 | 4 | 0 | 21-23 | Uncategorized |  | KLOG |
| 0x42ee0118 | 3 | 0 | 27-29 | Uncategorized |  | MEGA |
| 0x43af8200 | 3 | 0 | 27-29 | Uncategorized |  | MEGA |
| 0x61181a0c | 3 | 1 | 22-24 | Uncategorized | InitEntityState_Idle | BOIL;BRG1;CRYS;MOSS;SCIE |
| 0x0c041001 | 3 | 0 | 25-27 | Uncategorized |  | WIZZ |
| 0x18e88110 | 3 | 3 | 9-11 | Boss | GlennYntisDeathEventHandler | GLEN |
| 0x2e819804 | 3 | 3 | 15-17 | Enemy | FinnDamageEventHandler | KLOG |
| 0x3080860d | 3 | 3 | 9-11 | Menu/UI | InitMenuStage1 | MENU |
| 0x400c981d | 3 | 1 | 7-9 | Enemy | InitEnemyAnimatedWithDeathSpawn | BOIL;BRG1;CRYS;MOSS;SCIE |
| 0x40b08011 | 3 | 1 | 16-18 | Menu/UI | InitMenuStage1 | MENU |
| 0x49209640 | 3 | 0 | 22-24 | Uncategorized |  | KLOG |
| 0x412c8430 | 3 | 0 | 27-29 | Uncategorized |  | WIZZ |
| 0x4c22d240 | 3 | 0 | 21-23 | Uncategorized |  | KLOG |
| 0x5a99815f | 3 | 3 | 20-22 | Effect/Particle | SpawnCollectibleParticles | BOIL;BRG1;CLOU;CRYS;CSTL |
| 0xbc68d0c6 | 3 | 3 | 25-27 | Player | BossCollision_SpawnDebrisAndLayers | EntityInitCallback_8007 | BOIL;BRG1;CAVE;CLOU;CRYS |
| 0xc8649260 | 3 | 0 | 12-14 | Uncategorized |  | KLOG |
| 0x081c8300 | 3 | 0 | 16-22 | Uncategorized |  | EVIL;FINN;RUNN |
| 0x10b95810 | 3 | 0 | 10-16 | Uncategorized |  | FINN |
| 0x10b95810 | 3 | 0 | 11-17 | Uncategorized |  | FINN |
| 0x10b95810 | 3 | 0 | 9-15 | Uncategorized |  | FINN |
| 0x10b9581c | 3 | 0 | 9-15 | Uncategorized |  | FINN |
| 0x40ca1c10 | 3 | 0 | 19-25 | Uncategorized |  | CAVE |
| 0x40ca1c10 | 3 | 0 | 21-27 | Uncategorized |  | CLOU |
| 0x08ce0610 | 3 | 1 | 25-29 | Uncategorized | InitPasswordEntity | CSTL;FOOD;SEVN;TMPL |
| 0x08ce0610 | 3 | 0 | 24-28 | Uncategorized |  | CSTL;FOOD;SEVN;TMPL |
| 0x10b9581c | 3 | 0 | 10-14 | Uncategorized |  | FINN |
| 0x800da112 | 3 | 3 | 2-6 | Boss | KloggDeathEventHandler | KLOG |
| 0xb0907444 | 3 | 0 | 21-25 | Uncategorized |  | TMPL |
| 0xb0907444 | 3 | 0 | 20-24 | Uncategorized |  | TMPL |
| 0x1cf99931 | 3 | 1 | 24-25 | Player | PlayerStateInit_JumpFromPlatform | Callback_80069200 | BOIL;BRG1;CAVE;CLOU;CRYS |
| 0x38a0c119 | 3 | 1 | 27-28 | Menu/UI | InitMenuStage1 | MENU |
| 0x410808f9 | 3 | 3 | 11-12 | Clayball/Projectile | Callback_80051ea4 | ProjectileState_HomingActiveVariant3 | P | FOOD;TMPL |
| 0x60a91c9c | 3 | 0 | 8-24 | Uncategorized |  | BRG1 |
| 0x60aa1c9c | 3 | 0 | 9-25 | Uncategorized |  | BRG1 |
| 0x10b95810 | 3 | 0 | 9-17 | Uncategorized |  | FINN |
