---
title: "Cohort swap-candidate report"
category: analysis/asset-identification
tags: [analysis, asset, identification, cohort, swap, candidates]
---

# Cohort swap-candidate report

For each named function that touches multiple asset IDs, we compute the pairwise XOR-delta rotation class and look up which suffix-token pairs would produce that class. A small number of candidates with strong semantic match to the function's name is the gold signal.

## `EntityTick_ShadowMirror` (12 ids)

| pair (IDs) | class | popcount | suffix-pair candidates |
|---|---|---|---|
| `0x052aa082↔0x0538a2ea` | `0x0002404d` | 6 | (none in vocab) |
| `0x052aa082↔0x0628a800` | `0x01810441` | 6 | `UP↔LAND` |
| `0x052aa082↔0x0708a4a0` | `0x01110211` | 6 | `UP↔IDLE` |
| `0x052aa082↔0x112ae088` | `0x00100285` | 5 | `21↔F17` |
| `0x052aa082↔0x2524e089` | `0x002c8039` | 8 | (none in vocab) |
| `0x052aa082↔0x8139a088` | `0x000a8413` | 7 | (none in vocab) |
| `0x052aa082↔0x8524a880` | `0x001c1005` | 6 | `UP↔FALL` |
| `0x052aa082↔0x8569a090` | `0x00128043` | 6 | `UP↔DOWN` |
| `0x052aa082↔0x9629a000` | `0x00829303` | 8 | (none in vocab) |
| `0x052aa082↔0xa628e009` | `0x022e8c09` | 10 | (none in vocab) |
| `0x052aa082↔0xd70880a4` | `0x01369111` | 10 | (none in vocab) |
| `0x0538a2ea↔0x0628a800` | `0x00aea031` | 10 | (none in vocab) |
| `0x0538a2ea↔0x0708a4a0` | `0x0064a023` | 8 | (none in vocab) |
| `0x0538a2ea↔0x112ae088` | `0x04909885` | 9 | (none in vocab) |
| `0x0538a2ea↔0x2524e089` | `0x00e21319` | 10 | (none in vocab) |
| `0x0538a2ea↔0x8139a088` | `0x004098a1` | 7 | (none in vocab) |
| `0x0538a2ea↔0x8524a880` | `0x003814d5` | 10 | (none in vocab) |
| `0x0538a2ea↔0x8569a090` | `0x00a204f5` | 10 | (none in vocab) |
| `0x0538a2ea↔0x9629a000` | `0x02ea9311` | 12 | (none in vocab) |
| `0x0538a2ea↔0xa628e009` | `0x042e3a31` | 12 | (none in vocab) |
| `0x0538a2ea↔0xd70880a4` | `0x0224ed23` | 12 | (none in vocab) |
| `0x0628a800↔0x0708a4a0` | `0x00090065` | 6 | `58↔NORMAL`, `IDLE↔LAND` |
| `0x0628a800↔0x112ae088` | `0x02488817` | 9 | (none in vocab) |
| `0x0628a800↔0x2524e089` | `0x0c488923` | 10 | (none in vocab) |
| `0x0628a800↔0x8139a088` | `0x08888711` | 9 | (none in vocab) |
| `0x0628a800↔0x8524a880` | `0x002020c3` | 6 | `61↔ROLL`, `FALL↔LAND` |
| `0x0628a800↔0x8569a090` | `0x0422420d` | 8 | `DOWN↔LAND` |
| `0x0628a800↔0x9629a000` | `0x00108009` | 4 | `U↔TORSO`, `23↔TEXT`, `73↔YELL`, `90↔LIFE`, `F44↔CHEST` |
| `0x0628a800↔0xa628e009` | `0x0002404d` | 6 | (none in vocab) |
| `0x0628a800↔0xd70880a4` | `0x01452689` | 10 | (none in vocab) |

## `CreateMenuEntities` (8 ids)

| pair (IDs) | class | popcount | suffix-pair candidates |
|---|---|---|---|
| `0x00e2f188↔0x80e85ea0` | `0x00155e51` | 11 | (none in vocab) |
| `0x00e2f188↔0x88a28194` | `0x01c07221` | 9 | (none in vocab) |
| `0x00e2f188↔0x902c0002` | `0x0cef18a9` | 15 | (none in vocab) |
| `0x00e2f188↔0x9158a0f6` | `0x17e91ba5` | 17 | (none in vocab) |
| `0x00e2f188↔0xa9240484` | `0x0ca9c6f5` | 16 | (none in vocab) |
| `0x00e2f188↔0xb8700ca1` | `0x125fa537` | 17 | (none in vocab) |
| `0x00e2f188↔0xe8628689` | `0x00ee03d1` | 12 | (none in vocab) |
| `0x80e85ea0↔0x88a28194` | `0x0212b7cd` | 14 | (none in vocab) |
| `0x80e85ea0↔0x902c0002` | `0x08622f51` | 12 | (none in vocab) |
| `0x80e85ea0↔0x9158a0f6` | `0x08d87f2b` | 16 | (none in vocab) |
| `0x80e85ea0↔0xa9240484` | `0x0a731689` | 13 | (none in vocab) |
| `0x80e85ea0↔0xb8700ca1` | `0x009c4c29` | 10 | (none in vocab) |
| `0x80e85ea0↔0xe8628689` | `0x052d115b` | 13 | (none in vocab) |
| `0x88a28194↔0x902c0002` | `0x032c311d` | 12 | (none in vocab) |
| `0x88a28194↔0x9158a0f6` | `0x0b10cfd1` | 14 | (none in vocab) |
| `0x88a28194↔0xa9240484` | `0x02186851` | 9 | (none in vocab) |
| `0x88a28194↔0xb8700ca1` | `0x0d28d353` | 14 | (none in vocab) |
| `0x88a28194↔0xe8628689` | `0x001c7583` | 11 | (none in vocab) |
| `0x902c0002↔0x9158a0f6` | `0x005d283d` | 12 | (none in vocab) |
| `0x902c0002↔0xa9240484` | `0x0090c721` | 9 | (none in vocab) |
| `0x902c0002↔0xb8700ca1` | `0x0328ca17` | 12 | (none in vocab) |
| `0x902c0002↔0xe8628689` | `0x09d0d16f` | 15 | (none in vocab) |
| `0x9158a0f6↔0xa9240484` | `0x0f948e47` | 15 | (none in vocab) |
| `0x9158a0f6↔0xb8700ca1` | `0x158ae525` | 14 | (none in vocab) |
| `0x9158a0f6↔0xe8628689` | `0x133fbc9d` | 19 | (none in vocab) |
| `0xa9240484↔0xb8700ca1` | `0x02094455` | 9 | (none in vocab) |
| `0xa9240484↔0xe8628689` | `0x041a828d` | 10 | (none in vocab) |
| `0xb8700ca1↔0xe8628689` | `0x0128a285` | 9 | (none in vocab) |

## `ToggleMenuOptionState` (7 ids)

| pair (IDs) | class | popcount | suffix-pair candidates |
|---|---|---|---|
| `0x0ad0f813↔0x29c0e211` | `0x0111880d` | 8 | (none in vocab) |
| `0x0ad0f813↔0x2ad0f011` | `0x00004011` | 3 | `0↔44`, `1↔54`, `2↔64`, `3↔74`, `3↔HAIR` |
| `0x0ad0f813↔0x68c0f413` | `0x00188403` | 6 | (none in vocab) |
| `0x0ad0f813↔0x69c04050` | `0x086c6217` | 12 | (none in vocab) |
| `0x0ad0f813↔0x69c8f473` | `0x018c0c63` | 10 | (none in vocab) |
| `0x0ad0f813↔0x90810000` | `0x02734a3f` | 15 | (none in vocab) |
| `0x29c0e211↔0x2ad0f011` | `0x00018809` | 5 | (none in vocab) |
| `0x29c0e211↔0x68c0f413` | `0x00160241` | 6 | `26↔EXIT` |
| `0x29c0e211↔0x69c04050` | `0x00028905` | 6 | `F59↔WINGS` |
| `0x29c0e211↔0x69c8f473` | `0x00205989` | 8 | (none in vocab) |
| `0x29c0e211↔0x90810000` | `0x078846e5` | 13 | (none in vocab) |
| `0x2ad0f011↔0x68c0f413` | `0x00402421` | 5 | `49↔COLLECT`, `52↔F46`, `F26↔NOSE`, `WAIT↔CHEST` |
| `0x2ad0f011↔0x69c04050` | `0x0414310b` | 9 | (none in vocab) |
| `0x2ad0f011↔0x69c8f473` | `0x008c4863` | 9 | (none in vocab) |
| `0x2ad0f011↔0x90810000` | `0x011ba51f` | 14 | (none in vocab) |
| `0x68c0f413↔0x69c04050` | `0x00b44301` | 8 | (none in vocab) |
| `0x68c0f413↔0x69c8f473` | `0x00084003` | 4 | `0↔FLAME`, `0↔FRAME`, `8↔ARM`, `E↔INNER`, `F↔FLAME` |
| `0x68c0f413↔0x90810000` | `0x04fe107d` | 15 | (none in vocab) |
| `0x69c04050↔0x69c8f473` | `0x0008b423` | 8 | `PHASE↔POWER` |
| `0x69c04050↔0x90810000` | `0x0143e505` | 11 | (none in vocab) |
| `0x69c8f473↔0x90810000` | `0x1cfe527d` | 19 | (none in vocab) |

## `InitMenuStage1` (7 ids)

| pair (IDs) | class | popcount | suffix-pair candidates |
|---|---|---|---|
| `0x10094096↔0x3080820d` | `0x044e14d9` | 12 | (none in vocab) |
| `0x10094096↔0x3080840d` | `0x044e24d9` | 12 | (none in vocab) |
| `0x10094096↔0x30808e0d` | `0x044e74d9` | 14 | (none in vocab) |
| `0x10094096↔0x38a0c119` | `0x031e5153` | 13 | (none in vocab) |
| `0x10094096↔0x40b18011` | `0x021d42e3` | 12 | (none in vocab) |
| `0x10094096↔0x68c01218` | `0x192a51cf` | 15 | (none in vocab) |
| `0x3080820d↔0x3080840d` | `0x00000003` | 2 | `0↔1`, `0↔E`, `0↔G`, `1↔2`, `1↔F` |
| `0x3080820d↔0x30808e0d` | `0x00000003` | 2 | `0↔1`, `0↔E`, `0↔G`, `1↔2`, `1↔F` |
| `0x3080820d↔0x38a0c119` | `0x020810c5` | 7 | (none in vocab) |
| `0x3080820d↔0x40b18011` | `0x021c7031` | 10 | (none in vocab) |
| `0x3080820d↔0x68c01218` | `0x01558409` | 9 | (none in vocab) |
| `0x3080840d↔0x30808e0d` | `0x00000005` | 2 | `0↔2`, `0↔D`, `0↔H`, `1↔3`, `1↔E` |
| `0x3080840d↔0x38a0c119` | `0x02081145` | 7 | (none in vocab) |
| `0x3080840d↔0x40b18011` | `0x031041c7` | 10 | (none in vocab) |
| `0x3080840d↔0x68c01218` | `0x02585561` | 11 | (none in vocab) |
| `0x30808e0d↔0x38a0c119` | `0x020813c5` | 9 | (none in vocab) |
| `0x30808e0d↔0x40b18011` | `0x0310e1c7` | 12 | (none in vocab) |
| `0x30808e0d↔0x68c01218` | `0x02705561` | 11 | (none in vocab) |
| `0x38a0c119↔0x40b18011` | `0x0228210f` | 9 | (none in vocab) |
| `0x38a0c119↔0x68c01218` | `0x015060d3` | 10 | (none in vocab) |
| `0x40b18011↔0x68c01218` | `0x049438c9` | 11 | (none in vocab) |

## `KloggDeathEventHandler` (6 ids)

| pair (IDs) | class | popcount | suffix-pair candidates |
|---|---|---|---|
| `0x800da102↔0x800da116` | `0x00000005` | 2 | `0↔2`, `0↔D`, `0↔H`, `1↔3`, `1↔E` |
| `0x800da102↔0x800da11a` | `0x00000003` | 2 | `0↔1`, `0↔E`, `0↔G`, `1↔2`, `1↔F` |
| `0x800da102↔0x800da132` | `0x00000003` | 2 | `0↔1`, `0↔E`, `0↔G`, `1↔2`, `1↔F` |
| `0x800da102↔0x800da152` | `0x00000005` | 2 | `0↔2`, `0↔D`, `0↔H`, `1↔3`, `1↔E` |
| `0x800da102↔0x888da193` | `0x00012211` | 5 | `17↔CHEST`, `27↔LEG`, `43↔F31` |
| `0x800da116↔0x800da11a` | `0x00000003` | 2 | `0↔1`, `0↔E`, `0↔G`, `1↔2`, `1↔F` |
| `0x800da116↔0x800da132` | `0x00000009` | 2 | `0↔3`, `0↔C`, `0↔I`, `1↔4`, `1↔D` |
| `0x800da116↔0x800da152` | `0x00000011` | 2 | `0↔4`, `0↔B`, `0↔J`, `1↔5`, `1↔C` |
| `0x800da116↔0x888da193` | `0x00010a11` | 5 | `15↔CHEST`, `25↔LEG`, `41↔F31`, `61↔GLIDE` |
| `0x800da11a↔0x800da132` | `0x00000005` | 2 | `0↔2`, `0↔D`, `0↔H`, `1↔3`, `1↔E` |
| `0x800da11a↔0x800da152` | `0x00000009` | 2 | `0↔3`, `0↔C`, `0↔I`, `1↔4`, `1↔D` |
| `0x800da11a↔0x888da193` | `0x00011211` | 5 | `16↔CHEST`, `26↔LEG`, `42↔F31`, `46↔RIGHT`, `95↔COLLECT` |
| `0x800da132↔0x800da152` | `0x00000003` | 2 | `0↔1`, `0↔E`, `0↔G`, `1↔2`, `1↔F` |
| `0x800da132↔0x888da193` | `0x00014211` | 5 | `18↔CHEST`, `26↔DIE`, `28↔LEG`, `41↔CLOSING`, `44↔F31` |
| `0x800da152↔0x888da193` | `0x00018211` | 5 | `19↔CHEST`, `29↔LEG`, `45↔F31`, `46↔F30` |

## `ClayballState_DestroyWithDebris` (5 ids)

| pair (IDs) | class | popcount | suffix-pair candidates |
|---|---|---|---|
| `0x462c6040↔0x46346040` | `0x00000003` | 2 | `0↔1`, `0↔E`, `0↔G`, `1↔2`, `1↔F` |
| `0x462c6040↔0x46386040` | `0x00000005` | 2 | `0↔2`, `0↔D`, `0↔H`, `1↔3`, `1↔E` |
| `0x462c6040↔0x463e6040` | `0x00000009` | 2 | `0↔3`, `0↔C`, `0↔I`, `1↔4`, `1↔D` |
| `0x462c6040↔0xd6bc2450` | `0x04242411` | 7 | `64↔PIECE` |
| `0x46346040↔0x46386040` | `0x00000003` | 2 | `0↔1`, `0↔E`, `0↔G`, `1↔2`, `1↔F` |
| `0x46346040↔0x463e6040` | `0x00000005` | 2 | `0↔2`, `0↔D`, `0↔H`, `1↔3`, `1↔E` |
| `0x46346040↔0xd6bc2450` | `0x04242211` | 7 | `R↔DEACTIVATE`, `63↔PIECE`, `F37↔KICK` |
| `0x46386040↔0x463e6040` | `0x00000003` | 2 | `0↔1`, `0↔E`, `0↔G`, `1↔2`, `1↔F` |
| `0x46386040↔0xd6bc2450` | `0x04242111` | 7 | `62↔PIECE`, `MIRROR↔CLOSING` |
| `0x463e6040↔0xd6bc2450` | `0x04242091` | 7 | `61↔PIECE`, `77↔BONUS` |

## `PlayerCallback_DeathDebrisSpawner` (5 ids)

| pair (IDs) | class | popcount | suffix-pair candidates |
|---|---|---|---|
| `0x146ce002↔0x3d348056` | `0x02a14ac3` | 11 | (none in vocab) |
| `0x146ce002↔0xb468d0c6` | `0x00218625` | 8 | (none in vocab) |
| `0x146ce002↔0xb868d0c6` | `0x010c312b` | 10 | (none in vocab) |
| `0x146ce002↔0xbe68d0c6` | `0x02186255` | 10 | (none in vocab) |
| `0x3d348056↔0xb468d0c6` | `0x0895c509` | 11 | (none in vocab) |
| `0x3d348056↔0xb868d0c6` | `0x0855c509` | 11 | (none in vocab) |
| `0x3d348056↔0xbe68d0c6` | `0x06b8a121` | 11 | (none in vocab) |
| `0xb468d0c6↔0xb868d0c6` | `0x00000003` | 2 | `0↔1`, `0↔E`, `0↔G`, `1↔2`, `1↔F` |
| `0xb468d0c6↔0xbe68d0c6` | `0x00000005` | 2 | `0↔2`, `0↔D`, `0↔H`, `1↔3`, `1↔E` |
| `0xb868d0c6↔0xbe68d0c6` | `0x00000003` | 2 | `0↔1`, `0↔E`, `0↔G`, `1↔2`, `1↔F` |

## `RunnEvent_CutsceneDebrisAndLevel` (5 ids)

| pair (IDs) | class | popcount | suffix-pair candidates |
|---|---|---|---|
| `0x146ce002↔0x3d348056` | `0x02a14ac3` | 11 | (none in vocab) |
| `0x146ce002↔0xb468d0c6` | `0x00218625` | 8 | (none in vocab) |
| `0x146ce002↔0xb868d0c6` | `0x010c312b` | 10 | (none in vocab) |
| `0x146ce002↔0xbe68d0c6` | `0x02186255` | 10 | (none in vocab) |
| `0x3d348056↔0xb468d0c6` | `0x0895c509` | 11 | (none in vocab) |
| `0x3d348056↔0xb868d0c6` | `0x0855c509` | 11 | (none in vocab) |
| `0x3d348056↔0xbe68d0c6` | `0x06b8a121` | 11 | (none in vocab) |
| `0xb468d0c6↔0xb868d0c6` | `0x00000003` | 2 | `0↔1`, `0↔E`, `0↔G`, `1↔2`, `1↔F` |
| `0xb468d0c6↔0xbe68d0c6` | `0x00000005` | 2 | `0↔2`, `0↔D`, `0↔H`, `1↔3`, `1↔E` |
| `0xb868d0c6↔0xbe68d0c6` | `0x00000003` | 2 | `0↔1`, `0↔E`, `0↔G`, `1↔2`, `1↔F` |

## `BossCollision_SpawnDebrisAndLayers` (5 ids)

| pair (IDs) | class | popcount | suffix-pair candidates |
|---|---|---|---|
| `0x146ce002↔0x3d348056` | `0x02a14ac3` | 11 | (none in vocab) |
| `0x146ce002↔0xb468d0c6` | `0x00218625` | 8 | (none in vocab) |
| `0x146ce002↔0xb868d0c6` | `0x010c312b` | 10 | (none in vocab) |
| `0x146ce002↔0xbe68d0c6` | `0x02186255` | 10 | (none in vocab) |
| `0x3d348056↔0xb468d0c6` | `0x0895c509` | 11 | (none in vocab) |
| `0x3d348056↔0xb868d0c6` | `0x0855c509` | 11 | (none in vocab) |
| `0x3d348056↔0xbe68d0c6` | `0x06b8a121` | 11 | (none in vocab) |
| `0xb468d0c6↔0xb868d0c6` | `0x00000003` | 2 | `0↔1`, `0↔E`, `0↔G`, `1↔2`, `1↔F` |
| `0xb468d0c6↔0xbe68d0c6` | `0x00000005` | 2 | `0↔2`, `0↔D`, `0↔H`, `1↔3`, `1↔E` |
| `0xb868d0c6↔0xbe68d0c6` | `0x00000003` | 2 | `0↔1`, `0↔E`, `0↔G`, `1↔2`, `1↔F` |

## `FinnDamageEventHandler` (5 ids)

| pair (IDs) | class | popcount | suffix-pair candidates |
|---|---|---|---|
| `0x146ce002↔0x2e809804` | `0x00c75d8f` | 15 | (none in vocab) |
| `0x146ce002↔0x2e811804` | `0x00c75dbf` | 17 | (none in vocab) |
| `0x146ce002↔0x2e839804` | `0x00c75def` | 17 | (none in vocab) |
| `0x146ce002↔0x3d348056` | `0x02a14ac3` | 11 | (none in vocab) |
| `0x2e809804↔0x2e811804` | `0x00000003` | 2 | `0↔1`, `0↔E`, `0↔G`, `1↔2`, `1↔F` |
| `0x2e809804↔0x2e839804` | `0x00000003` | 2 | `0↔1`, `0↔E`, `0↔G`, `1↔2`, `1↔F` |
| `0x2e809804↔0x3d348056` | `0x061484ed` | 12 | (none in vocab) |
| `0x2e811804↔0x2e839804` | `0x00000005` | 2 | `0↔2`, `0↔D`, `0↔H`, `1↔3`, `1↔E` |
| `0x2e811804↔0x3d348056` | `0x09dacc29` | 14 | (none in vocab) |
| `0x2e839804↔0x3d348056` | `0x09db8c29` | 14 | (none in vocab) |

## `PlaySoundEffect` (5 ids)

| pair (IDs) | class | popcount | suffix-pair candidates |
|---|---|---|---|
| `0x00248e52↔0x246166fa` | `0x0488bd15` | 12 | (none in vocab) |
| `0x00248e52↔0x44d4c8d8` | `0x0468a44f` | 12 | (none in vocab) |
| `0x00248e52↔0x5860c640` | `0x024b0889` | 9 | (none in vocab) |
| `0x00248e52↔0x70d006d8` | `0x0f4888a7` | 13 | (none in vocab) |
| `0x246166fa↔0x44d4c8d8` | `0x05ad7113` | 14 | (none in vocab) |
| `0x246166fa↔0x5860c640` | `0x00682e9f` | 13 | (none in vocab) |
| `0x246166fa↔0x70d006d8` | `0x0112a58b` | 11 | (none in vocab) |
| `0x44d4c8d8↔0x5860c640` | `0x039681d3` | 13 | (none in vocab) |
| `0x44d4c8d8↔0x70d006d8` | `0x001a0267` | 9 | (none in vocab) |
| `0x5860c640↔0x70d006d8` | `0x0260a2c3` | 10 | (none in vocab) |

## `Callback_800570a4` (5 ids)

| pair (IDs) | class | popcount | suffix-pair candidates |
|---|---|---|---|
| `0x462c6040↔0x46346040` | `0x00000003` | 2 | `0↔1`, `0↔E`, `0↔G`, `1↔2`, `1↔F` |
| `0x462c6040↔0x46386040` | `0x00000005` | 2 | `0↔2`, `0↔D`, `0↔H`, `1↔3`, `1↔E` |
| `0x462c6040↔0x463e6040` | `0x00000009` | 2 | `0↔3`, `0↔C`, `0↔I`, `1↔4`, `1↔D` |
| `0x462c6040↔0xd6bc2450` | `0x04242411` | 7 | `64↔PIECE` |
| `0x46346040↔0x46386040` | `0x00000003` | 2 | `0↔1`, `0↔E`, `0↔G`, `1↔2`, `1↔F` |
| `0x46346040↔0x463e6040` | `0x00000005` | 2 | `0↔2`, `0↔D`, `0↔H`, `1↔3`, `1↔E` |
| `0x46346040↔0xd6bc2450` | `0x04242211` | 7 | `R↔DEACTIVATE`, `63↔PIECE`, `F37↔KICK` |
| `0x46386040↔0x463e6040` | `0x00000003` | 2 | `0↔1`, `0↔E`, `0↔G`, `1↔2`, `1↔F` |
| `0x46386040↔0xd6bc2450` | `0x04242111` | 7 | `62↔PIECE`, `MIRROR↔CLOSING` |
| `0x463e6040↔0xd6bc2450` | `0x04242091` | 7 | `61↔PIECE`, `77↔BONUS` |

## `PlayerCallback_8005cce0` (5 ids)

| pair (IDs) | class | popcount | suffix-pair candidates |
|---|---|---|---|
| `0x4260c8b5↔0x54608016` | `0x0024518b` | 9 | (none in vocab) |
| `0x4260c8b5↔0x5838cc16` | `0x0094634b` | 11 | (none in vocab) |
| `0x4260c8b5↔0x91a9d01d` | `0x151a7923` | 14 | (none in vocab) |
| `0x4260c8b5↔0xd167c005` | `0x0708b093` | 11 | (none in vocab) |
| `0x54608016↔0x5838cc16` | `0x00031613` | 8 | (none in vocab) |
| `0x54608016↔0x91a9d01d` | `0x00bc5c95` | 13 | (none in vocab) |
| `0x54608016↔0xd167c005` | `0x004e141d` | 10 | (none in vocab) |
| `0x5838cc16↔0x91a9d01d` | `0x02f26447` | 13 | (none in vocab) |
| `0x5838cc16↔0xd167c005` | `0x04e257c3` | 14 | (none in vocab) |
| `0x91a9d01d↔0xd167c005` | `0x01840ce1` | 9 | `HEAD↔SMOKE` |

## `PlayerCallback_8005d404` (5 ids)

| pair (IDs) | class | popcount | suffix-pair candidates |
|---|---|---|---|
| `0x146ce002↔0x3d348056` | `0x02a14ac3` | 11 | (none in vocab) |
| `0x146ce002↔0xb468d0c6` | `0x00218625` | 8 | (none in vocab) |
| `0x146ce002↔0xb868d0c6` | `0x010c312b` | 10 | (none in vocab) |
| `0x146ce002↔0xbe68d0c6` | `0x02186255` | 10 | (none in vocab) |
| `0x3d348056↔0xb468d0c6` | `0x0895c509` | 11 | (none in vocab) |
| `0x3d348056↔0xb868d0c6` | `0x0855c509` | 11 | (none in vocab) |
| `0x3d348056↔0xbe68d0c6` | `0x06b8a121` | 11 | (none in vocab) |
| `0xb468d0c6↔0xb868d0c6` | `0x00000003` | 2 | `0↔1`, `0↔E`, `0↔G`, `1↔2`, `1↔F` |
| `0xb468d0c6↔0xbe68d0c6` | `0x00000005` | 2 | `0↔2`, `0↔D`, `0↔H`, `1↔3`, `1↔E` |
| `0xb868d0c6↔0xbe68d0c6` | `0x00000003` | 2 | `0↔1`, `0↔E`, `0↔G`, `1↔2`, `1↔F` |

## `EntityInitCallback_80070850` (5 ids)

| pair (IDs) | class | popcount | suffix-pair candidates |
|---|---|---|---|
| `0x146ce002↔0x3d348056` | `0x02a14ac3` | 11 | (none in vocab) |
| `0x146ce002↔0xb468d0c6` | `0x00218625` | 8 | (none in vocab) |
| `0x146ce002↔0xb868d0c6` | `0x010c312b` | 10 | (none in vocab) |
| `0x146ce002↔0xbe68d0c6` | `0x02186255` | 10 | (none in vocab) |
| `0x3d348056↔0xb468d0c6` | `0x0895c509` | 11 | (none in vocab) |
| `0x3d348056↔0xb868d0c6` | `0x0855c509` | 11 | (none in vocab) |
| `0x3d348056↔0xbe68d0c6` | `0x06b8a121` | 11 | (none in vocab) |
| `0xb468d0c6↔0xb868d0c6` | `0x00000003` | 2 | `0↔1`, `0↔E`, `0↔G`, `1↔2`, `1↔F` |
| `0xb468d0c6↔0xbe68d0c6` | `0x00000005` | 2 | `0↔2`, `0↔D`, `0↔H`, `1↔3`, `1↔E` |
| `0xb868d0c6↔0xbe68d0c6` | `0x00000003` | 2 | `0↔1`, `0↔E`, `0↔G`, `1↔2`, `1↔F` |

## `SpawnCollectibleParticles` (4 ids)

| pair (IDs) | class | popcount | suffix-pair candidates |
|---|---|---|---|
| `0x5a89815f↔0x5ab9815f` | `0x00000003` | 2 | `0↔1`, `0↔E`, `0↔G`, `1↔2`, `1↔F` |
| `0x5a89815f↔0x5ad9815f` | `0x00000005` | 2 | `0↔2`, `0↔D`, `0↔H`, `1↔3`, `1↔E` |
| `0x5a89815f↔0x98004092` | `0x07370a27` | 14 | (none in vocab) |
| `0x5ab9815f↔0x5ad9815f` | `0x00000003` | 2 | `0↔1`, `0↔E`, `0↔G`, `1↔2`, `1↔F` |
| `0x5ab9815f↔0x98004092` | `0x07370ae7` | 16 | (none in vocab) |
| `0x5ad9815f↔0x98004092` | `0x07370b67` | 16 | (none in vocab) |

## `GlennYntisDeathEventHandler` (4 ids)

| pair (IDs) | class | popcount | suffix-pair candidates |
|---|---|---|---|
| `0x18e88310↔0x18e88510` | `0x00000003` | 2 | `0↔1`, `0↔E`, `0↔G`, `1↔2`, `1↔F` |
| `0x18e88310↔0x18e88910` | `0x00000005` | 2 | `0↔2`, `0↔D`, `0↔H`, `1↔3`, `1↔E` |
| `0x18e88310↔0x18e8c704` | `0x00001105` | 4 | `Y↔TEETH`, `01↔HEAD`, `02↔24`, `06↔20`, `12↔34` |
| `0x18e88510↔0x18e88910` | `0x00000003` | 2 | `0↔1`, `0↔E`, `0↔G`, `1↔2`, `1↔F` |
| `0x18e88510↔0x18e8c704` | `0x00001085` | 4 | `R↔ABOVE`, `Z↔TEETH`, `01↔24`, `02↔HEAD`, `11↔34` |
| `0x18e88910↔0x18e8c704` | `0x00001385` | 6 | `03↔HEAD`, `99↔CHARGE` |

## `JoeHeadJoeMoveAndCheckAttack` (4 ids)

| pair (IDs) | class | popcount | suffix-pair candidates |
|---|---|---|---|
| `0x0b290ba2↔0x0e3889be` | `0x01446087` | 9 | (none in vocab) |
| `0x0b290ba2↔0x603588e8` | `0x0694d639` | 14 | (none in vocab) |
| `0x0b290ba2↔0xf2810224` | `0x0130df35` | 14 | (none in vocab) |
| `0x0e3889be↔0x603588e8` | `0x01566e0d` | 13 | (none in vocab) |
| `0x0e3889be↔0xf2810224` | `0x1735f973` | 19 | (none in vocab) |
| `0x603588e8↔0xf2810224` | `0x15992569` | 14 | (none in vocab) |

## `ShrineyGuardSoundUpdateTick` (4 ids)

| pair (IDs) | class | popcount | suffix-pair candidates |
|---|---|---|---|
| `0x086e44c0↔0x11eac4c0` | `0x00003309` | 6 | `53↔WAVE` |
| `0x086e44c0↔0x204e44c0` | `0x00000141` | 3 | `6↔02`, `7↔12`, `8↔00`, `8↔22`, `9↔10` |
| `0x086e44c0↔0x205e40c2` | `0x00402283` | 6 | (none in vocab) |
| `0x11eac4c0↔0x204e44c0` | `0x00006349` | 7 | (none in vocab) |
| `0x11eac4c0↔0x205e40c2` | `0x008c6d21` | 10 | (none in vocab) |
| `0x204e44c0↔0x205e40c2` | `0x00080201` | 3 | `0↔94`, `A↔44`, `A↔57`, `A↔83`, `B↔54` |

