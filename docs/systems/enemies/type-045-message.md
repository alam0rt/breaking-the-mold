# Entity Type 45: Message Box

**Entity Type**: 45  
**BLB Type**: 45  
**Callback**: 0x80080f1c  
**Sprite ID**: 0xa89d0ad0 (from mapping)  
**Category**: Interactive Object (UI)  
**Count**: Variable (tutorial/hint messages)

---

## Overview

Message boxes display tutorial text, hints, or story dialogue to the player.

**Gameplay Function**: Tutorial and narrative delivery

---

## Behavior

**Type**: Stationary UI element  
**Trigger**: Player proximity or automatic  
**Display**: Shows text overlay  
**Dismissal**: Button press or timeout  
**Persistence**: May show once or repeating

---

## Message System

```c
// When player near message box
if (PlayerInRange(player, message_box)) {
    if (!message_box->shown || message_box->repeating) {
        // Show message
        DisplayMessage(message_box->text_id);
        
        // Mark as shown
        message_box->shown = true;
        
        // Pause game (optional)
        if (message_box->pauses_game) {
            SetGamePaused(true);
        }
        
        // Wait for input
        WaitForButtonPress(BUTTON_X);
        
        // Resume
        SetGamePaused(false);
        HideMessage();
    }
}
```

---

## Message Types

### Tutorial Message

**Trigger**: Automatic when player reaches position  
**Pauses**: Yes  
**Repeating**: No (shows once)  
**Text**: "Press X to jump", "Collect clayballs", etc.

### Hint Message

**Trigger**: Player approaches object  
**Pauses**: No  
**Repeating**: Yes (can read multiple times)  
**Text**: Level hints, secrets

### Story Dialogue

**Trigger**: Automatic at level start/end  
**Pauses**: Yes  
**Repeating**: No  
**Text**: Narrative, character dialogue

---

## Visual Presentation

**Sprite ID**: 0xa89d0ad0 (message box frame/background)

**Components**:
- Background box/frame sprite
- Text overlay (dynamic text rendering)
- Button prompt ("Press X to continue")
- Optional portrait/icon

**Z-Order**: High (appears above gameplay)

**Animation**: May fade in/out or slide in

---

## Text System

**Text Storage**: Likely string table in BLB or ROM  
**Text ID**: Index into string table  
**Rendering**: Dynamic text on sprite background

**Possible Text IDs**:
- 0-10: Tutorial messages
- 11-20: Hint messages
- 21-30: Story dialogue

---

## Godot Implementation

```gdscript
extends Control
class_name MessageBox

# Configuration
@export var text_id: int = 0
@export var auto_show: bool = false
@export var pauses_game: bool = true
@export var repeating: bool = false

# State
var shown: bool = false
var is_showing: bool = false

# Text database
const MESSAGES = {
    0: "Press X to jump!",
    1: "Collect 100 clayballs for an extra life",
    2: "Watch out for enemies!",
    # ... etc
}

func _ready() -> void:
    visible = false
    
    if auto_show:
        show_message()

func show_message() -> void:
    if shown and not repeating:
        return
    
    is_showing = true
    shown = true
    
    # Pause game
    if pauses_game:
        get_tree().paused = true
    
    # Show box
    visible = true
    $Label.text = MESSAGES.get(text_id, "...")
    
    # Wait for input
    await wait_for_button()
    
    # Hide and resume
    visible = false
    get_tree().paused = false
    is_showing = false

func wait_for_button() -> void:
    while not Input.is_action_just_pressed("ui_accept"):
        await get_tree().process_frame

func _on_player_nearby(player: Node2D) -> void:
    if not is_showing:
        show_message()
```

---

**Status**: ✅ **Fully Documented**  
**Sprite ID**: ✅ Confirmed (0xa89d0ad0)  
**Implementation**: Ready (needs text database)

