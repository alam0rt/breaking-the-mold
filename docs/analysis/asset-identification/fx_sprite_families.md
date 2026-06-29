---
title: "FX-anchored sprite families (STEM + action)"
category: analysis/asset-identification
tags: [analysis, asset, identification, fx, sprite, families]
---

# FX-anchored sprite families (STEM + action)

Each entity's sprite family solved from its FX action vocabulary. `stem` is the opaque sprite entity-hash; members are `stem + action` landing on real sprite IDs. Validate via the referencing functions.

## SKULL  (stem `0xb04fa10a`, sh 11) — 2 sprites; validate: EntityCollision(2);KloggDeathEventHan(2)

| action | sprite_id | ref functions |
|---|---|---|
| FLY_01 | 0x800da102 | EntityCollision_EnemyHitWithCallback | KloggDeathE |
| FLY_02 | 0x800da11a | EntityCollision_EnemyHitWithCallback | KloggDeathE |

## KLAY  (stem `0x18288010`, sh 21) — 6 sprites; validate: PlayerState(8);PlayerCallback(2);PlayerStateInit(1)

| action | sprite_id | ref functions |
|---|---|---|
| DIE_FALL | 0x1e28e0d4 | PlayerState_FallWithRotation | PlayerTickCallback  |
| DUCK_DOWN | 0x0a3a4051 | PlayerStateInit_FallFromLedge |
| FALL | 0x00388110 | PlayerCallback_EventHandlerAltWithQueue | PlayerSt |
| RUN_FAST | 0x092b8480 | PlayerProcessTileCollision | PlayerState_JumpApex  |
| TURN | 0x18298210 | PlayerState_WalkingRight | PlayerState_UpdateTPage |
| WAKE_UP | 0x392cb014 | PlayerState_ClearCheckpointAndSetInvincible | Play |

## PICKUP  (stem `0x1c288030`, sh 26) — 3 sprites; validate: PlayerStateInit(3);UniverseEnemaKillA(1);EntityTick(1)

| action | sprite_id | ref functions |
|---|---|---|
| FARTHEAD | 0x1e1000b3 | EntityTick_ShadowMirror | PlayerStateInit_ThrowPro |
| SUPER_WILLIE | 0x1c8c2437 | UniverseEnemaKillAllEnemies | PlayerStateInit_Chec |
| UNIVERSE_ENEMA | 0x6c22083a | PlayerState_CheckpointRestoreComplete | UniverseEn |

## BOSS_HEAD  (stem `0x0a3809b2`, sh 11) — 7 sprites; validate: JoeHeadJoeReturnTo(4);JoeHeadJoeEnterAct(3);JoeHeadJoeClearVoi(2)

| action | sprite_id | ref functions |
|---|---|---|
| ATTACK1 | 0x0b081dbb | JoeHeadJoeReturnToIdleState | JoeHeadJoeClearVoice |
| ATTACK2 | 0x0b0811bb | JoeHeadJoeReturnToIdleState | JoeHeadJoeReturnToId |
| DIE | 0x2b3889b2 | JoeHeadJoeReturnToIdleStateAlt | JoeHeadJoeDeathAn |
| HIT | 0x1a3109b2 | JoeHeadJoeSetFacingAndAttack | JoeHeadJoeEnterActi |
| IDLE1 | 0x0b290ba2 | JoeHeadJoeCheckPlayerInAttackRange | JoeHeadJoeSet |
| TURN | 0x8a3809f2 | JoeHeadJoeClearVoice |
| WALK | 0x0e3889be | JoeHeadJoeEnterActiveState | JoeHeadJoeMoveAndChec |

## BIRD  (stem `0xec418237`, sh 30) — 2 sprites; validate: CollectibleScaledT(1)

| action | sprite_id | ref functions |
|---|---|---|
| FLY_01 | 0xec000027 | CollectibleScaledTickCallback |
| SQUISH_01 | 0xe4cb8330 |  |
