# Sound Effect Reference

**Status**: Extracted from PlaySoundEffect calls (2026-01-15)  
**Source**: SLES_010.90.c decompilation

This document catalogs all sound effect IDs found in the Skullmonkeys codebase.

---

## Overview

Sounds are identified by **32-bit hash values** (likely CRC32 or similar of original sound names).

**PlaySoundEffect** @ 0x8007c388 takes:
- `sound_id`: 32-bit hash
- `pan_pos`: -160 to +160 (left to right)
- `force_flag`: 0=respect mute, 1=force play

---

## Known Sound IDs

### Gameplay Sounds

| Sound ID | Hex | Context | Line | Force? | Description |
|----------|-----|---------|------|--------|-------------|
| 0x248e52 | 2,379,346 | Player jump/checkpoint | 17943 | No | Jump sound when landing on checkpoint |
| 0x7003474c | 1,879,337,804 | Item pickup | - | - | Item collection (0x32-0x3B tiles) |

**Known from tile-collision-complete.md**:
- `0x248e52`: Jump sound (plays when landing on checkpoint tile 0x02-0x07)
- `0x7003474c`: Item pickup sound (plays when collecting items 0x32-0x3B)

### Level-Specific Sounds

| Sound ID | Hex | Context | Line | Force? | Description |
|----------|-----|---------|------|--------|-------------|
| 0x646c2cc0 | 1,684,887,744 | ? | 11789, 37507 | Both | Appears in 2 places |
| 0x90810000 | 2,425,356,288 | ? | 11812, 42728 | Both | Appears in 2 places |
| 0x4810c2c4 | 1,208,959,684 | ? | 32617 | No | Single usage |
| 0x421586c2 | 1,108,896,450 | ? | 33306 | No | Single usage |
| 0x121941c4 | 303,890,884 | ? | 38108 | No | Single usage |
| 0x2990901 | 43,848,961 | ? | 38181 | No | Single usage |
| 0x40023e30 | 1,073,954,352 | ? | 38196 | No | Single usage |
| 0x65281e40 | 1,697,070,656 | ? | 41829 | Yes | Forced playback |
| 0x4c60f249 | 1,281,679,945 | ? | 41894 | Yes | Forced playback |

**Pan Position**: All calls use `0xa0` (160 decimal) = **center panning**

**Force Flag**:
- `0` = Respect mute setting (most sounds)
- `1` = Force playback even if muted (critical sounds like death, checkpoints)

---

## Sound Table Format

Sounds are loaded from Asset 601 (Audio ADPCM samples) and stored in:
- **g_SoundTable** @ 0x8009cc64
- **g_SoundTableCount** @ 0x800a6078

### SoundEntry Structure (12 bytes)

```c
struct SoundEntry {
  u32 sound_id;      // +0x00: Hash (e.g., 0x248e52)
  u32 spu_address;   // +0x04: SPU RAM address (0x1010+)
  u16 base_volume;   // +0x08: Default volume (0-0x3FFF)
  u16 flags;         // +0x0A: Playback flags
};
```

### Playback Flags

| Flag | Hex | Effect |
|------|-----|--------|
| 0x001 | - | Use 0x800 ADSR envelope (longer sustain) |
| 0x010 | - | Random pitch ±383 (±0x17F, subtle variation) |
| 0x020 | - | Random pitch ±767 (±0x2FF, moderate variation) |
| 0x040 | - | Random pitch ±1535 (±0x5FF, extreme variation) |
| 0x100 | - | 25% playback probability (skips 75% of calls) |
| 0x200 | - | 50% playback probability (skips 50% of calls) |
| 0x400 | - | 75% playback probability (skips 25% of calls) |

**Random pitch**: Used for ambient sounds to prevent repetition
**Probability**: Used for footsteps, impact sounds to avoid overwhelming audio

---

## Sound Remapping System

**Mode-dependent remapping** if `null_00h_800a6082 != 0`:

Remapping table @ `0x8009d0fc` maps common sounds to mode-specific variants:

```c
// Known remapped IDs (from PlaySoundEffect @ line 40360-40385)
if (sound_id == 0x70d006d8) remap_index = 0;  // Common sound 0
if (sound_id == 0x246166fa) remap_index = 1;  // Common sound 1
if (sound_id == 0x44d4c8d8) remap_index = 2;  // Common sound 2
if (sound_id == 0x248e52)   remap_index = 3;  // Jump sound
if (sound_id == 0x5860c640) remap_index = 4;  // Common sound 4

// Lookup: mode_sounds[(mode * 5) + remap_index]
```

**Purpose**: Different levels/game modes can have unique sound variants for the same action.

---

## SPU Voice Allocation

**Hardware limit**: 24 simultaneous voices (0-23)

**Allocation strategy**: Round-robin starting at `null_00h_800a6088`

**Voice state tracking**:
- `null_00h_8009cc18[voice_index]` = sound table index for this voice
- `SpuGetKeyStatus(voice_mask)` checks if voice is playing
  * 0 = stopped (available)
  * 3 = stopped (available)
  * Other = playing (busy)

**If all 24 voices busy**: PlaySoundEffect returns 0xFFFFFFFF (failed)

---

## Pan Position Mapping

Pan position (`-160` to `+160`) maps to stereo L/R volumes:

| Pan | Left % | Right % | Description |
|-----|--------|---------|-------------|
| -160 | 100% | 0% | Full left |
| -80 | 75% | 25% | Mostly left |
| 0 | 50% | 50% | Center |
| +80 | 25% | 75% | Mostly right |
| +160 | 0% | 100% | Full right |

**Standard usage**: `0xa0` (160) = center

**Dynamic panning**: Entities can call `SetVoicePanning` to update sound position as they move

---

## Related Functions

| Address | Name | Purpose |
|---------|------|---------|
| 0x8007c388 | PlaySoundEffect | Play sound with panning |
| 0x8007c7b8 | StopSoundEffect | Stop specific voice |
| 0x8007c7e0 | StopAllSPUVoices | Emergency stop (all 24) |
| 0x8007c818 | CalculateStereoVolume | Pan → L/R volumes |
| 0x8007ca28 | SetVoicePanning | Update voice pan realtime |
| 0x8007ca60 | StartCDAudioForLevel | CD music control |

---

## Global Variables

| Address | Name | Type | Purpose |
|---------|------|------|---------|
| 0x8009cc64 | g_SoundTable | SoundEntry[] | Loaded sound metadata |
| 0x800a6078 | g_SoundTableCount | u32 | Number of sounds loaded |
| 0x800a6088 | null_00h_800a6088 | u8 | Next voice to allocate |
| 0x800a6087 | null_00h_800a6087 | u8 | Mute flag (1=muted) |
| 0x800a6082 | null_00h_800a6082 | u8 | Sound remap mode |
| 0x800a607f | null_04h_800a607f | u8 | Master volume multiplier |
| 0x8009d0fc | s_remapping_table | u32[] | Mode-dependent sound remaps |

---

## Usage Patterns

### Simple Sound Effect
```c
PlaySoundEffect(0x248e52, 0xa0, 0);  // Jump sound, center pan, respect mute
```

### Forced Critical Sound
```c
PlaySoundEffect(0x65281e40, 0xa0, 1);  // Force play even if muted
```

### Dynamic Panning
```c
uint voice = PlaySoundEffect(sound_id, initial_pan, 0);
// Later, as entity moves:
SetVoicePanning(voice, new_pan_based_on_screen_x);
```

### Stop Sound Early
```c
uint voice = PlaySoundEffect(sound_id, 0xa0, 0);
// Stop before natural end:
StopSoundEffect(voice);
```

---

## Asset References

**Audio assets** loaded from BLB:
- **Asset 601**: Primary audio container (SPU ADPCM samples)
- **Asset 700**: Additional audio (9 levels have extra sounds)

See `docs/blb-data-format.md` for Asset 601/700 format details.

---

## TODO: Sound ID Identification

To map sound IDs to actual sound names:

1. **Trace in PCSX-Redux**:
   ```lua
   -- Breakpoint on PlaySoundEffect @ 0x8007c388
   -- Log param_1 (sound_id) and surrounding context
   ```

2. **Cross-reference with game events**:
   - Player jump → 0x248e52
   - Item pickup → 0x7003474c
   - Death sound → ?
   - Hit sound → ?

3. **Audio asset extraction**:
   - Extract Asset 601 SPU samples
   - Analyze audio waveforms
   - Match to in-game events

4. **Update this table** with descriptive names once identified

---

## Verification Status

| Sound ID | Status | Method |
|----------|--------|--------|
| 0x248e52 | ✅ Verified | Code analysis (checkpoint jump) |
| 0x7003474c | ✅ Verified | Code analysis (item pickup) |
| 0x646c2cc0 | ⚠️ Unknown | Need trace/extraction |
| 0x90810000 | ⚠️ Unknown | Need trace/extraction |
| 0x4810c2c4 | ⚠️ Unknown | Need trace/extraction |
| 0x421586c2 | ⚠️ Unknown | Need trace/extraction |
| 0x121941c4 | ⚠️ Unknown | Need trace/extraction |
| 0x2990901 | ⚠️ Unknown | Need trace/extraction |
| 0x40023e30 | ⚠️ Unknown | Need trace/extraction |
| 0x65281e40 | ⚠️ Unknown | Need trace/extraction |
| 0x4c60f249 | ⚠️ Unknown | Need trace/extraction |

---

## Implementation Notes (for evil-engine)

When implementing sound system:

1. **Load sound table** from Asset 601
2. **Hash sound names** to 32-bit IDs (match original algorithm)
3. **Implement voice allocator** (round-robin, 24 voices max)
4. **Calculate stereo volumes** from pan position
5. **Support probability flags** for random playback
6. **Support pitch variation** for ambient sounds
7. **Track voice usage** for dynamic panning/stopping

Godot 4.x equivalent:
```gdscript
class_name SoundManager

var active_voices: Array[AudioStreamPlayer] = []
var next_voice_index: int = 0

func play_sound(sound_id: int, pan: float, force: bool) -> int:
    if muted and not force: return -1
    
    # Find free voice
    for i in range(24):
        var voice = active_voices[(next_voice_index + i) % 24]
        if not voice.playing:
            # Setup and play
            voice.stream = get_sound_by_id(sound_id)
            voice.set_pan(remap(pan, -160, 160, -1.0, 1.0))
            voice.play()
            return (next_voice_index + i) % 24
    
    return -1  # All voices busy
```
