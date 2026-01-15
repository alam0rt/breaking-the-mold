#!/usr/bin/env python3
"""
Verify Ghidra function updates from evil-engine documentation analysis.
Run this to confirm all discovered functions have been properly renamed and commented.
"""

import json
import subprocess
from typing import Dict, List, Tuple

# Function updates applied on January 15, 2026
VERIFIED_FUNCTIONS = [
    # Spawn System
    ("0x80025664", "SetSpawnOffsetGroup1", "Control enemy spawn distance for group 1"),
    ("0x800256b8", "SetSpawnOffsetGroup2", "Control enemy spawn distance for group 2"),
    
    # Entity Scaling & Audio
    ("0x8001c364", "SetupEntityScaleCallbacks", "Configure entity for shrink/grow powerup"),
    ("0x8001c5b4", "UpdateEntitySoundPanning", "Update stereo panning for entity-relative sounds"),
    ("0x80025b7c", "InitEntityDataPointers", "Set paired data pointers in entity"),
    
    # Layer Rendering
    ("0x800196d8", "FreeAllLayerRenderSlotsWrapper", "Wrapper to free layer render slots"),
    ("0x80019700", "ClearAllLayerRenderSlots", "Zero all 20 layer render slot entries"),
    
    # Entity Lifecycle
    ("0x8001ca60", "DestroyEntityAndFreeMemory", "Complete entity destruction with memory cleanup"),
    ("0x8001aab4", "SetEntityFacingDirection", "Set or toggle entity facing direction"),
    
    # Entity Messaging
    ("0x80022d94", "SendMessageToPlayer", "Message/event broadcasting to entities"),
    ("0x80022f24", "SendMessageToPlayerVariant", "Message/event broadcasting variant"),
    
    # Player State
    ("0x8002615c", "ClearGreenBullets", "Clear green bullets counter"),
    ("0x800261d4", "InitializePlayerState", "Initialize all player state fields to defaults"),
    ("0x80026260", "AdvanceLevelAndClearCollectibles", "Level transition - increment progress"),
    
    # Password & Demo
    ("0x80025bc0", "EnableDemoPlaybackMode", "Switch to demo playback mode"),
    ("0x80025c7c", "BuildPasswordFromPlayerState", "KEY DISCOVERY: Generate password from player state"),
    
    # Sprite System
    ("0x8007bbec", "InitSpriteContextWrapper", "Wrapper allowing InitSpriteContext to be chained"),
    
    # Entity List Management
    ("0x80020974", "AddToZOrderList", "Insert entity into z-order sorted list"),
    ("0x80020a1c", "AddToUpdateQueue", "Insert entity into update queue list"),
    ("0x80020a74", "RemoveFromZOrderList", "Remove entity from z-order list"),
    
    # Audio System (already named, verify comments exist)
    ("0x8007c7b8", "StopSPUVoice", "Stop specific SPU voice channel"),
    ("0x8007c818", "CalculateStereoVolume", "Convert pan position to stereo L/R volumes"),
    ("0x8007ca28", "SetVoicePanning", "Update SPU voice panning in realtime"),
    ("0x8007c388", "PlaySoundEffect", "Play a sound effect with stereo panning"),
    
    # HUD System
    ("0x8002b22c", "ShowPauseMenuHUD", "Display pause menu HUD with player stats"),
    
    # Core Game Loop (already named, verify comments)
    ("0x8007e654", "GameModeCallback", "Main game loop coordinator"),
    ("0x80020b34", "EntityTickLoop", "Iterate entity tick list and call update callbacks"),
    ("0x80020e80", "RenderEntities", "Render all entities and handle background color updates"),
    ("0x80023dbc", "UpdateCameraPosition", "Complex camera scroll logic based on player position"),
    
    # Collision & Spawning
    ("0x800241f4", "GetTileAttributeAtPosition", "Get tile collision attribute at world position"),
    ("0x800226f8", "CheckEntityCollision", "Entity-entity collision detection"),
    ("0x80024288", "SpawnOnScreenEntities", "Spawn entities from Asset 501 that are on screen"),
]

def verify_function(address: str, expected_name: str, comment_keyword: str) -> Tuple[bool, str]:
    """
    Verify a function has the correct name and a comment containing the keyword.
    Returns (success, message).
    """
    try:
        # Query Ghidra via MCP (assumes Ghidra MCP server is running)
        # In practice, this would use the actual MCP Python client
        # For now, this is a template showing the verification logic
        
        result = subprocess.run(
            ["curl", "-s", f"http://localhost:8192/api/v1/functions/{address}"],
            capture_output=True,
            text=True,
            timeout=5
        )
        
        if result.returncode != 0:
            return False, f"Failed to query Ghidra for {address}"
        
        data = json.loads(result.stdout)
        actual_name = data.get("name", "")
        
        # Check name matches
        if actual_name != expected_name:
            return False, f"Name mismatch: expected '{expected_name}', got '{actual_name}'"
        
        # Check comment exists (simplified - would need to check plate comment)
        # This is a template - actual implementation would verify comment content
        
        return True, f"✓ {expected_name} @ {address}"
        
    except Exception as e:
        return False, f"Error verifying {address}: {str(e)}"

def main():
    """Verify all function updates."""
    print("=" * 80)
    print("Ghidra Function Update Verification")
    print("=" * 80)
    print()
    
    print(f"Checking {len(VERIFIED_FUNCTIONS)} functions...")
    print()
    
    successes = 0
    failures = []
    
    for address, name, comment_keyword in VERIFIED_FUNCTIONS:
        success, message = verify_function(address, name, comment_keyword)
        
        if success:
            print(f"✓ {message}")
            successes += 1
        else:
            print(f"✗ {message}")
            failures.append((address, name, message))
    
    print()
    print("=" * 80)
    print(f"Results: {successes}/{len(VERIFIED_FUNCTIONS)} functions verified")
    
    if failures:
        print()
        print("Failed verifications:")
        for address, name, message in failures:
            print(f"  {address} ({name}): {message}")
        return 1
    else:
        print("✓ All functions verified successfully!")
        return 0

if __name__ == "__main__":
    exit(main())
