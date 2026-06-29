---
title: "Audio System"
category: systems
tags: [systems, audio, sound]
---

# Audio System

**Status**: Overview consolidated from the former `sound-system.md` (merged 2026-06-29). Names/addresses below are reconciled against `src/sound.c` and `symbol_addrs.txt`; treat any unconfirmed claim with the project's usual clean-room caution.

The game uses the PSX SPU (Sound Processing Unit) with ADPCM-encoded samples for sound effects, CD-XA streaming for music, and a small set of menu-driven audio settings. This document is the single high-level overview. Detailed playback-function analysis and the sound-ID tables live in the reference docs linked under [See also](#see-also).

---

## Architecture Overview

```
+-------------------------------------------------------------------+
|                    AUDIO SYSTEM ARCHITECTURE                      |
+-------------------------------------------------------------------+
|                                                                   |
|  +---------------------+        +---------------------+           |
|  |   CD-XA STREAMING   |        |    SPU SOUND FX     |           |
|  +---------------------+        +---------------------+           |
|  | StartCDAudioForLevel|        | PlaySoundEffect     |           |
|  | StopCDStreaming     |        | PlayEntityPosition. |           |
|  | TickCDStreamBuffer  |        | StopSPUVoice        |           |
|  | PauseCDAudio        |        | SetVoicePanning     |           |
|  | ResumeCDAudio       |        | StopAllSPUVoices    |           |
|  +----------+----------+        +----------+----------+           |
|             |                              |                      |
|             v                              v                      |
|  +-----------------------------------------------------+          |
|  |                 PSY-Q AUDIO LAYER                   |          |
|  |  SpuSetKey, SpuSetVoiceVolume, SpuSetVoicePitch,    |          |
|  |  SpuSetCommonAttr, SpuSetCommonCDVolume,            |          |
|  |  CD-ROM commands (StopCDAudio etc.)                 |          |
|  +-----------------------------------------------------+          |
|                                                                   |
|  +-----------------------------------------------------+          |
|  |                 AUDIO SETTINGS (globals)            |          |
|  |  REVERB_LEVEL_SETTING (0-4)  AUDIO_VOLUME_SETTING   |          |
|  |  STEREO_MODE_SETTING (1=mono, 2=stereo)  (0-4)      |          |
|  |  AUDIO_PITCHES_MUTED (pause mute flag)              |          |
|  +-----------------------------------------------------+          |
+-------------------------------------------------------------------+
```

---

## 1. Audio Asset Distribution

| Segment | Asset | Contents |
|---------|-------|---------|
| Secondary | 601 (0x259) | Audio sample bank with TOC |
| Secondary | 602 (0x25A) | Volume/pan table (per-sample) |
| Primary | 601 (0x259) | Alternative audio source |
| Primary | 602 (0x25A) | Alternative volume/pan |
| Tertiary | 700 (0x2BC) | **Demo replay data** (not audio) |

### Asset 601 - Audio Sample Bank

```
Offset  Size   Description
------  ----   -----------
0x00    u16    Sample count
0x02    u16    Reserved (always 0)
0x04    12xN   Sample entries
...     var    ADPCM audio data
```

#### Sample Entry (12 bytes)

```
Offset  Size  Type   Description
------  ----  ----   -----------
0x00    4     u32    Sample ID (hash identifier)
0x04    4     u32    SPU size (bytes in SPU RAM)
0x08    4     u32    Data offset (within audio block)
```

#### Example (SCIE Stage 0)

13 samples:

| Index | Size | SPU Offset |
|------:|-----:|-----------:|
| 0 | 4,432 | 0xA0 |
| 1 | 3,296 | 0x11F0 |
| 2 | 5,296 | 0x1ED0 |
| ... | ... | ... |
| 12 | 6,160 | 0xD1A0 |

### Asset 602 - Volume/Pan Table

4 bytes per sample:

```
Offset  Size  Type   Description
------  ----  ----   -----------
0x00    u16   Volume (0-0x3FFF, where 0x3FFF = max)
0x02    u16   Pan/flags (0 = center/none)
```

If Asset 602 is NULL, defaults are used: volume=0x3FFF, flags/pan=0.

#### Common Volume Values

| Volume | Hex | Meaning |
|--------|-----|---------|
| 16383 | 0x3FFF | Maximum |
| 8192 | 0x2000 | 50% |
| 4915 | 0x1333 | ~30% |

### Asset 700 - Demo Replay Data (Not Audio)

**CORRECTION (2026-01-19)**: Asset 700 is NOT audio data. It contains **demo/attract mode input replay data**.

Appears in 9 of 26 levels: MENU, SCIE, TMPL, BOIL, FOOD, BRG1, GLID, CAVE, WEED.

The "SPU-like" command bytes (0x80, 0xC0) were misidentified - they are actually PSX button bitmasks:

- `0x0080` = Left button
- `0x2080` = Left + Circle (jump)
- `0x20C0` = Left + Down + Circle

See [Demo/Attract Mode System](demo-attract-mode.md) for the complete format documentation.

### Cross-Asset Relationship

**Verified**: `Asset602.size = Asset601.sample_count x 4`

This holds for all 91 secondary segments with audio data.

---

## 2. Audio Loading and SPU Upload

### 2.1 Loading Flow

From `InitializeAndLoadLevel` @ 0x8007D1D0:

```c
// After loading the secondary segment
audioSamples   = GetAsset601Ptr(ctx);   // Secondary Asset 601
volumePanTable = GetAsset602Ptr(ctx);   // Secondary Asset 602
audioSize      = GetAsset601Size(ctx);
UploadAudioToSPU(audioSamples, volumePanTable, audioSize);
```

`GetAsset601Ptr` checks `ctx[1]` to select between the primary and secondary
audio source:

| Condition | Source | ctx offsets |
|-----------|--------|-------------|
| Primary mode | ctx+0x74 | 0x74, 0x78, 0x7C |
| Secondary mode | ctx+0x48 | 0x48, 0x4C, 0x50 |

#### LevelDataContext Audio Offsets

| Offset | Field | Description |
|--------|-------|-------------|
| +0x48 | ctx[18] | Asset 601 ptr (secondary) |
| +0x4C | ctx[19] | Asset 601 size |
| +0x50 | ctx[20] | Asset 602 ptr |
| +0x54 | ctx[21] | Asset 700 ptr (demo replay, not audio) |
| +0x58 | ctx[22] | Asset 700 size |
| +0x74 | ctx[29] | Asset 601 ptr (primary) |
| +0x78 | ctx[30] | Asset 601 size (primary) |
| +0x7C | ctx[31] | Asset 602 ptr (primary) |

### 2.2 SPU Upload

`UploadAudioToSPU @ 0x8007C088` (still `INCLUDE_ASM`):

```c
// Called during level loading
UploadAudioToSPU(asset601_ptr, asset602_ptr, asset601_size);

// SPU RAM layout:
// 0x0000-0x100F: System reserved
// 0x1010+:       Audio samples (sequential uploads)
```

The transfer uses the PSY-Q SPU DMA path:

```c
SpuSetTransferMode(SpuTransByDMA);
SpuSetTransferStartAddr(0x1010 + offset);
SpuWrite(data, size);
SpuIsTransferCompleted(SpuTransferWait);
```

Two runtime accumulators track upload state (see `src/sound.c`):

- `SPU_UPLOAD_USED_BYTES @ 0x800A6078` (u32) - running total of bytes uploaded to SPU RAM.
- `SPU_UPLOAD_BLOCK_COUNT @ 0x800A6081` (u8) - number of upload-block log entries currently live.

### 2.3 SPU Upload-Block Tracking

Each upload records a log entry so it can be rewound on free. The block table
is `SPU_UPLOAD_BLOCK_TABLE @ D_8009CFB0`, 12 bytes per entry. Only the leading
`u16 sz` (uploaded byte size) is read in `src/sound.c`; the remaining 10 bytes
are opaque:

```c
typedef struct {        // 12 bytes
    u16 sz;             // 0x00: uploaded byte size
    u8  _pad02[10];     // 0x02: opaque
} SpuUploadBlock;
```

`PopSPUUploadBlock @ 0x8007C2E0` decrements the count and subtracts the popped
entry's `sz` from `SPU_UPLOAD_USED_BYTES`. `ShutdownSPUAndResetSoundState @
0x8007C334` calls `SpuQuit()` and zeroes both accumulators.

### 2.4 Sound-Definition / Voice Tables (runtime)

| Symbol | Address | Stride | Purpose |
|--------|---------|--------|---------|
| `SOUND_DEFINITION_TABLE` | `D_8009CC68` | 12 bytes | Per-sound data; field `+0x00` = `s16 volume` (base mixer volume) |
| `VOICE_SOUND_INDEX_TABLE` | `D_8009CC18` | u8[24] | Maps voice number -> sound-definition index |
| `SAVED_VOICE_PITCHES` | `D_8009CC30` | u16[24] | Pitches saved while muted (pause) |
| `GAME_SOUND_EFFECT_ID_TABLE` | `D_8009CE64` | u32 | Numeric-ID -> hash table for `PlayGameSoundById` |

> Note: the older overview cited `g_SoundTable @ 0x8009cc60`. The live source
> places the sound-definition table at `D_8009CC68`; correct that drift when
> referencing it.

---

## 3. SPU Sound Effects

### 3.1 Core Functions

| Address | Function | Status | Purpose |
|---------|----------|--------|---------|
| 0x8007C388 | `PlaySoundEffect` | INCLUDE_ASM | Play sound with panning and mute/force handling |
| 0x8007C764 | `PlayGameSoundById` | matched | Play by numeric ID (0xC9-0xE4) |
| 0x8007C7B8 | `StopSPUVoice` | matched | Stop one specific voice |
| 0x8007C7EC | `StopAllSPUVoices` | matched | Stop all 24 voices |
| 0x8007C818 | `CalculateStereoVolume` | INCLUDE_ASM | Convert (volume, pan) to L/R volumes |
| 0x8007CA28 | `SetVoicePanning` | matched | Recompute a live voice's L/R volume from pan |
| 0x8001C4A4 | `PlayEntityPositionSound` | matched/named | Play a sound relative to an entity |
| 0x8001C5B4 | `UpdateEntitySoundPanning` | matched/named | Update a voice's pan as its entity moves |

> The former overview labeled `0x8007C764` as taking IDs `0xC9-0xE4`. Source
> confirms the guard `(soundId - 0xC9) >= 0x1C` (28 entries, 0xC9..0xE4) and the
> indirection through `GAME_SOUND_EFFECT_ID_TABLE`.

### 3.2 Sound Playback Flags

(From the playback-function analysis; see the functions reference.)

| Bit | Mask | Effect |
|-----|------|--------|
| 0 | 0x001 | Extended ADSR (0x800 instead of 0x400) |
| 4 | 0x010 | Random pitch +/-0x17F (small variation) |
| 5 | 0x020 | Random pitch +/-0x2FF (medium variation) |
| 6 | 0x040 | Random pitch +/-0x5FF (large variation) |
| 8 | 0x100 | 25% playback probability |
| 9 | 0x200 | 50% playback probability |
| 10 | 0x400 | 75% playback probability |

### 3.3 Stereo Panning

`SetVoicePanning(voice, pan)` looks up the voice's sound-definition volume and
calls `CalculateStereoVolume`, then `SpuSetVoiceVolume`. `CalculateStereoVolume`
applies a pan law that maps a pan range to a left/right volume split; in
`src/sound.c` the live pan range fed to it is `0..0x1E0` (0x1E0 = 480), and the
final stage divides by 4 (reverb scale).

The playback-function reference documents `PlaySoundEffect`'s pan as a screen-relative
**-160..+160** range (left to center to right); `PlayEntityPositionSound`
derives that pan from the entity's screen position relative to the camera:

```c
pan = entity.screen_x - camera_x;   // roughly -160..+160
PlaySoundEffect(sound_id, pan, 0);
```

### 3.4 Voice Allocation and the Voice Mask

There are 24 SPU voices (0-23). `StopSPUVoice(voice)` keys off a single voice
(`SpuSetKey(0, 1 << voice)`), and `StopAllSPUVoices()` keys off all 24
(`SpuSetKey(0, 0xFFFFFF)`) and clears `ACTIVE_SPU_VOICE_MASK`.

> The former overview named `0x800A6088` `g_NextVoiceIndex` and described it as
> a round-robin counter. In `src/sound.c` it is `ACTIVE_SPU_VOICE_MASK` and is
> *zeroed* by `StopAllSPUVoices` - i.e. it behaves as an active-voice mask, not
> a "next index" counter. Verify against the (still-ASM) `PlaySoundEffect`
> before relying on the round-robin claim.

---

## 4. CD-XA Streaming Music

### 4.1 Core Functions

| Address | Function | Status | Purpose |
|---------|----------|--------|---------|
| 0x8007CA9C | `StartCDAudioForLevel` | matched | Start music for a scene/sub-stage |
| 0x8007CB20 | `StopCDStreaming` | matched | Stop music playback |
| 0x8007CCB8 | `TickCDStreamBuffer` | matched | Per-frame streaming pump |
| 0x80038CAC | `PlayCDAudioTrack` | INCLUDE_ASM | Low-level CD play (track, channel) |
| 0x80038E0C | `StopCDAudio` | INCLUDE_ASM | Stop CD audio |
| 0x80038E50 | `PauseCDAudio` | matched | Pause CD audio |
| 0x80038EA0 | `ResumeCDAudio` | matched | Resume CD audio |
| 0x80038EF0 | `ProcessCDStreamState` | matched | Streaming state machine step |

### 4.2 Level-to-Music Mapping

`StartCDAudioForLevel(scene, sub)` indexes a 2-byte-stride pair table by
`(scene * 6 + (sub - 1))`. In `src/sound.c` the two byte streams alias the same
table, declared as separate externs so cc1 emits the natural two-base-pointer
pattern:

- `CD_LEVEL_TRACK_TABLE @ D_8009CFD8` - track id at even offsets
- `CD_LEVEL_CHANNEL_TABLE @ D_8009CFD9` - channel at odd offsets

```c
// scene < 26 and 1 <= sub <= 6
idx = (scene * 6 + (sub - 1)) * 2;
if (CD_LEVEL_TRACK_TABLE[idx] < 5) {        // only 5 CD-XA tracks exist
    PlayCDAudioTrack(CD_LEVEL_TRACK_TABLE[idx], CD_LEVEL_CHANNEL_TABLE[idx]);
    CD_STREAM_ACTIVE = 1;
}
```

A track id `>= 5` (the source uses values like 9) means "no music for this stage".

### 4.3 Streaming Pump

`TickCDStreamBuffer` is called every frame:

```c
void TickCDStreamBuffer(void) {
    if (CD_STREAM_ACTIVE == 0) return;
    if ((CD_STREAM_TICK_COUNTER++ & 3) == 0) {   // every 4th frame (~15 Hz @ 60 fps)
        ProcessCDStreamState();
    }
}
```

**Streaming globals** (corrected from the former overview, which listed
`g_CDStreamEnabled @ 0x800a5a08` / `g_FrameCounter @ 0x800a5a00` - those
addresses are not the CD-stream state; the live names/addresses are):

- `CD_STREAM_ACTIVE @ 0x800A6085` (u8) - streaming active flag
- `CD_STREAM_TICK_COUNTER @ 0x800A6074` (u32) - frame counter used for the every-4th-frame gate

---

## 5. Audio Settings

The three user-facing audio settings are stored in named globals (not in a
GameState sub-struct - the previously documented "GameState +0x12E..+0x130"
layout is contradicted by `include/Game/game_state.h`, where 0x12C is
`heap_debug_marker` and 0x130 is `bg_color_change_flag`).

### 5.1 Settings Globals (src/sound.c)

| Address | Name | Type | Range | Set by |
|---------|------|------|-------|--------|
| 0x800A607E | `STEREO_MODE_SETTING` | u8 | 1=mono, 2=stereo | `SetStereoMode` (validates `mode-1 < 2`) |
| 0x800A607F | `REVERB_LEVEL_SETTING` | u8 | 0-4 | `SetReverbLevel` (validates `< 5`) |
| 0x800A6080 | `AUDIO_VOLUME_SETTING` | u8 | 0-4 | `SetAudioVolume` (validates `< 5`) |
| 0x800A6082 | `GAME_MODE_SETTING` | u8 | 0-6 | `SetGameMode` (validates `< 7`) |
| 0x800A6087 | `AUDIO_PITCHES_MUTED` | u8 | 0/1 | pause/resume (see section 6) |
| 0x800A6088 | `ACTIVE_SPU_VOICE_MASK` | u8 | bitmask | `StopAllSPUVoices` clears it |

`SetAudioVolume(v)` also pushes the CD volume to hardware:
`scaled = (v * 0x7FFF) / 4; SpuSetCommonCDVolume(scaled, scaled);`.

### 5.2 Settings Functions

| Address | Function | Status | Purpose |
|---------|----------|--------|---------|
| 0x8007787C | `ApplyAudioSettings` | matched | Apply all three settings from a menu entity |
| 0x8007CC28 | `SetStereoMode` | matched | Set `STEREO_MODE_SETTING` (1/2) |
| 0x8007CC4C | `SetReverbLevel` | matched | Set `REVERB_LEVEL_SETTING` (0-4) |
| 0x8007CC68 | `SetAudioVolume` | matched | Set `AUDIO_VOLUME_SETTING` + push CD volume |
| 0x8007C36C | `SetGameMode` | matched | Set `GAME_MODE_SETTING` (0-6) |

`ApplyAudioSettings(OptionsMenuEntity *e)` (`src/passwd.c`) reads the menu
entity's settings and applies them:

```c
void ApplyAudioSettings(OptionsMenuEntity *e) {
    SetReverbLevel(e->reverbLevel);
    SetAudioVolume(e->audioVolume);
    SetStereoMode((u8)(e->stereoMode + 1));   // menu stores 0/1, global wants 1/2
}
```

This reconciles the two stereo encodings: the **menu UI** uses 0=mono / 1=stereo,
while the **global** `STEREO_MODE_SETTING` uses 1=mono / 2=stereo.

> `GAME_MODE_SETTING @ 0x800A6082` was previously documented as
> `g_SoundRemapMode` (0-6). The range matches `SetGameMode`'s `< 7` guard, but
> the name in the live source is `GAME_MODE_SETTING`; whether it specifically
> drives sound remapping is unconfirmed - treat the "sound remap" association as
> a hypothesis pending verification against `PlaySoundEffect`.

---

## 6. Pause / Mute System

When the game pauses, voice pitches are saved and zeroed (which mutes without
stopping playback), and CD audio is paused. `src/sound.c`:

```c
void SaveAndMuteAllVoicePitches(void) {        // @ 0x8007CB44
    if (AUDIO_PITCHES_MUTED == 0) {
        for (i = 0; i < 24; i++) {
            SpuGetVoicePitch(i, &saved);
            SAVED_VOICE_PITCHES[i] = saved;
            SpuSetVoicePitch(i, 0);            // mute
        }
        PauseCDAudio();
        AUDIO_PITCHES_MUTED = 1;
    }
}

void ResumeAllVoicePitches(void) {             // @ 0x8007CBC0
    if (AUDIO_PITCHES_MUTED != 0) {
        for (i = 0; i < 24; i++)
            SpuSetVoicePitch(i, SAVED_VOICE_PITCHES[i]);
        ResumeCDAudio();
        AUDIO_PITCHES_MUTED = 0;
    }
}
```

`PlaySoundEffect` is expected to respect the mute flag (the former overview
described an early-out returning `0xFFFFFFFF` when muted unless a `force` flag is
set); confirm against the still-ASM `PlaySoundEffect` body before relying on the
exact return value.

---

## 7. SPU Initialization

`InitSPUDefaults @ 0x8007BFB8` (still `INCLUDE_ASM`) sets the SPU common
attributes. As previously documented:

```c
SsUtReverbOff();                 // disable reverb

SpuCommonAttr attr;
attr.mask = 0x2C3;               // master vol + CD vol + CD mix
attr.mvol.left = 0x3FFF;         // max master volume
attr.mvol.right = 0x3FFF;
attr.cd.volume.left = 0x7FFF;    // max CD volume
attr.cd.volume.right = 0x7FFF;
attr.cd.mix = 1;                 // route CD audio to output
SpuSetCommonAttr(&attr);
```

---

## 8. Sound Entity System

Many entities emit sound and allocate an SPU voice that must be released when the
entity is destroyed. A large batch of these were named on 2026-01-19. A few are
now matched in C; most remain `INCLUDE_ASM` - check `/tmp/docaudit/asm_stubs.txt`
or `grep INCLUDE_ASM src/*.c` for current status rather than trusting a blanket
"all named" claim.

Representative functions (verified in `symbol_addrs.txt`):

| Address | Function | Purpose |
|---------|----------|---------|
| 0x8003C8D4 | `DestroySoundEmitterEntity` | Destroy sound emitter |
| 0x8003C950 | `SoundEmitterTickCallback` | Sound emitter tick |
| 0x80048D60 | `EntityStopSound` | Stop an entity's sound |
| 0x8004A768 | `HazardActivateWithSound` | Hazard activation + sound |
| 0x8004DD30 | `KloggUpdateWithSound` | Klogg boss sound update |
| 0x8005A3F8 | `IsEntityNearSoundTrigger` | Sound-trigger proximity test |

Additional named members of this family include `SoundEmitterStunnedTickCallback`,
`EnemyStartMovingWithSound`, `EntityInitSoundEmitterState`,
`InitSoundEmittingEnemy`, `DestroySoundEntityWithVoice`,
`SoundEmitterIdleTickCallback`, `DestroyEntityWithSoundAndChild`,
`CollectibleTickWithSoundPanning`, `SoundEmitterDestroyCallback`,
`SoundEmitterWithPanningTick`, `SoundEntityDestroyCallback`,
`HazardStopSound`, `HazardIdleWithSound`, `HazardStopSoundAlt`,
`DestroySoundEntity`, `FallingSoundEntityTick`, `JoeHeadJoeBallStopSound`,
`ShrineyGuardDeactivateWithSound`, `ShrineyGuardSoundUpdateTick`,
`FinnEntityDestroyWithSoundCleanup`, `EntityTick_ScaledSpriteWithSound`, and
`FinnDestroyWithSoundCleanup`.

### Sound-Emitter Cleanup Pattern

Sound emitters stop their voice before normal entity cleanup. Conceptually:

```c
void SoundEmitterDestroyCallback(Entity *entity, uint flags) {
    uint voice = entity->spuVoice;     // voice index, or 0xFF if none
    if (voice != 0xFF) {
        StopSPUVoice(voice);
    }
    DestroyEntityAndFreeMemory(entity, flags);
}
```

**Entity sound fields (observed; unverified against current structs)**:

- voice index (0-23, or 0xFF if none)
- sound ID hash
- volume level

> The former overview pinned these to `+0x110 / +0x112 / +0x114`. Those offsets
> have not been re-confirmed against the current entity layout - verify before
> using.

---

## See also

- [Audio System Functions Reference](audio-functions-reference.md) - detailed per-function analysis of the SPU playback functions (signatures, parameters, behavior).
- [Sound Effects Reference Table](sound-effects-reference.md) - sound-effect IDs mapped to gameplay contexts.
- [Complete Sound ID Reference](../reference/sound-ids-complete.md) - full extracted sound-ID table.
- [Demo/Attract Mode System](demo-attract-mode.md) - format of Asset 700 (input replay, not audio).
- [Asset Types](../blb/asset-types.md) - Asset 601, 602, 700 details.
- [Level Loading](level-loading.md) - when audio is uploaded to the SPU.
- [LevelDataContext](../reference/level-data-context.md) - audio pointer storage.

> Source of these overview sections: merged from the former `sound-system.md`
> (deleted), reconciled against `src/sound.c`, `src/passwd.c`, and
> `symbol_addrs.txt`. Sound-ID/effect/function detail intentionally lives in the
> reference docs above rather than being duplicated here.
