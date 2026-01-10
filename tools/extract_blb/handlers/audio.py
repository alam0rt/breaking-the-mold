"""
Audio Sample Bank Handler (Asset Types 601, 700)

Extracts audio sample banks to:
1. Raw bytes (.bin) via default handler
2. Individual audio samples as WAV files (converted from PSX ADPCM)

Audio Format (Asset 601):
- Header: u16 sample_count, u16 reserved
- Sample entries (12 bytes each): sample_id (u32), size (u32), offset (u32)
- ADPCM data follows

PSX ADPCM Format:
- 16-byte blocks
- Each block: 2 bytes header + 14 bytes compressed samples
- Header byte 0: shift (low 4 bits), filter (bits 4-5)
- Header byte 1: flags (loop start/end/repeat)
- Each block decodes to 28 samples (16-bit PCM)
"""

import json
import struct
import wave
from pathlib import Path

from . import register_handler, default_handler


# PSX ADPCM filter coefficients
ADPCM_FILTERS = [
    (0.0, 0.0),
    (60.0 / 64.0, 0.0),
    (115.0 / 64.0, -52.0 / 64.0),
    (98.0 / 64.0, -55.0 / 64.0),
    (122.0 / 64.0, -60.0 / 64.0),
]


def decode_adpcm_block(block: bytes, prev1: float, prev2: float) -> tuple[list[int], float, float]:
    """
    Decode a 16-byte PSX ADPCM block to 28 16-bit PCM samples.
    
    Returns:
        Tuple of (samples, new_prev1, new_prev2)
    """
    if len(block) < 16:
        return [], prev1, prev2
    
    # Parse header
    shift = block[0] & 0x0F
    filter_idx = (block[0] >> 4) & 0x07
    
    # Get filter coefficients
    if filter_idx < len(ADPCM_FILTERS):
        f0, f1 = ADPCM_FILTERS[filter_idx]
    else:
        f0, f1 = 0.0, 0.0
    
    samples = []
    
    # Decode 14 data bytes -> 28 samples (2 nibbles per byte)
    for i in range(2, 16):
        byte = block[i]
        
        for nibble_idx in range(2):
            # Extract nibble (low first, then high)
            if nibble_idx == 0:
                nibble = byte & 0x0F
            else:
                nibble = (byte >> 4) & 0x0F
            
            # Sign extend nibble
            if nibble >= 8:
                nibble -= 16
            
            # Apply shift and filter
            sample = float(nibble << (12 - shift))
            sample += prev1 * f0 + prev2 * f1
            
            # Clamp to 16-bit range
            sample = max(-32768.0, min(32767.0, sample))
            
            samples.append(int(sample))
            
            prev2 = prev1
            prev1 = sample
    
    return samples, prev1, prev2


def decode_adpcm(data: bytes, offset: int, size: int) -> bytes:
    """
    Decode PSX ADPCM audio to 16-bit PCM.
    
    Args:
        data: Full asset data
        offset: Start offset of ADPCM data
        size: Size of ADPCM data in bytes
        
    Returns:
        Raw 16-bit PCM samples (little-endian)
    """
    pcm_samples = []
    prev1, prev2 = 0.0, 0.0
    
    # Process 16-byte blocks
    end_offset = min(offset + size, len(data))
    pos = offset
    
    while pos + 16 <= end_offset:
        block = data[pos:pos + 16]
        
        # Check for end flag (byte 1, bit 0 = end of sample)
        flags = block[1]
        
        samples, prev1, prev2 = decode_adpcm_block(block, prev1, prev2)
        pcm_samples.extend(samples)
        
        # Check for end marker
        if flags & 0x01:
            break
        
        pos += 16
    
    # Convert to bytes (16-bit little-endian)
    pcm_bytes = bytearray()
    for sample in pcm_samples:
        pcm_bytes.extend(struct.pack('<h', sample))
    
    return bytes(pcm_bytes)


def save_wav(pcm_data: bytes, sample_rate: int, path: Path) -> bool:
    """Save PCM data as WAV file. Returns True on success."""
    try:
        with wave.open(str(path), 'wb') as wav:
            wav.setnchannels(1)  # Mono
            wav.setsampwidth(2)  # 16-bit
            wav.setframerate(sample_rate)
            wav.writeframes(pcm_data)
        return True
    except Exception:
        return False


@register_handler(601)
def audio_sample_bank_handler(
    data: bytes,
    asset_info,
    output_dir: Path,
    context: dict
) -> list[Path]:
    """
    Handler for asset type 601 (audio sample bank).
    
    Extracts:
    1. Raw bytes (via default handler)
    2. Individual audio samples as WAV files in audio/ subdirectory
    """
    # First, run default handler to save raw bytes
    output_files = default_handler(data, asset_info, output_dir, context)
    
    # Create audio subdirectory
    audio_dir = output_dir / "audio"
    audio_dir.mkdir(parents=True, exist_ok=True)
    
    # Parse audio sample bank header
    if len(data) < 4:
        return output_files
    
    sample_count, reserved = struct.unpack_from('<HH', data, 0)
    
    # Sanity check
    if sample_count > 500 or sample_count * 12 + 4 > len(data):
        return output_files
    
    # PSX SPU sample rate (most common)
    # Could be 11025, 22050, or 44100 - defaulting to 22050
    sample_rate = 22050
    
    samples_extracted = 0
    
    # Read sample entries
    for i in range(sample_count):
        entry_offset = 4 + i * 12
        if entry_offset + 12 > len(data):
            break
        
        sample_id, sample_size, sample_data_offset = struct.unpack_from('<III', data, entry_offset)
        
        # Calculate absolute offset (relative to start of data after header + entries)
        data_start = 4 + sample_count * 12
        abs_offset = data_start + sample_data_offset
        
        if abs_offset + sample_size > len(data):
            continue
        
        if sample_size == 0:
            continue
        
        try:
            # Decode ADPCM to PCM
            pcm_data = decode_adpcm(data, abs_offset, sample_size)
            
            if len(pcm_data) == 0:
                continue
            
            # Save as WAV
            wav_path = audio_dir / f"sample_{sample_id:08x}.wav"
            if save_wav(pcm_data, sample_rate, wav_path):
                output_files.append(wav_path)
                samples_extracted += 1
        except Exception:
            continue
    
    # Write audio extraction summary
    summary_path = audio_dir / "_summary.json"
    summary = {
        "sample_count": sample_count,
        "samples_extracted": samples_extracted,
        "sample_rate": sample_rate,
        "level": asset_info.level,
        "segment": asset_info.segment,
    }
    summary_path.write_text(json.dumps(summary, indent=2))
    output_files.append(summary_path)
    
    return output_files


@register_handler(700)
def spu_samples_handler(
    data: bytes,
    asset_info,
    output_dir: Path,
    context: dict
) -> list[Path]:
    """
    Handler for asset type 700 (additional SPU audio samples).
    
    Similar to 601 but uses a slightly different header format.
    Only appears in 9 levels.
    """
    # First, run default handler to save raw bytes
    output_files = default_handler(data, asset_info, output_dir, context)
    
    # Create audio subdirectory
    audio_dir = output_dir / "audio_700"
    audio_dir.mkdir(parents=True, exist_ok=True)
    
    # Parse Asset 700 header (different from 601)
    # u16 entry_count, u16 reserved, then entries
    if len(data) < 16:
        return output_files
    
    entry_count, reserved = struct.unpack_from('<HH', data, 0)
    
    # Sanity check
    if entry_count > 100 or entry_count == 0:
        return output_files
    
    sample_rate = 22050
    samples_extracted = 0
    
    # Parse entries (similar format to 601: id, size, offset)
    for i in range(entry_count):
        entry_offset = 4 + i * 12
        if entry_offset + 12 > len(data):
            break
        
        entry_id, data_size, data_offset = struct.unpack_from('<III', data, entry_offset)
        
        if data_offset + data_size > len(data):
            continue
        
        if data_size == 0:
            continue
        
        try:
            # Decode ADPCM to PCM
            pcm_data = decode_adpcm(data, data_offset, data_size)
            
            if len(pcm_data) == 0:
                continue
            
            # Save as WAV
            wav_path = audio_dir / f"spu_{entry_id:08x}.wav"
            if save_wav(pcm_data, sample_rate, wav_path):
                output_files.append(wav_path)
                samples_extracted += 1
        except Exception:
            continue
    
    # Write summary
    summary_path = audio_dir / "_summary.json"
    summary = {
        "entry_count": entry_count,
        "samples_extracted": samples_extracted,
        "sample_rate": sample_rate,
        "level": asset_info.level,
        "segment": asset_info.segment,
    }
    summary_path.write_text(json.dumps(summary, indent=2))
    output_files.append(summary_path)
    
    return output_files
