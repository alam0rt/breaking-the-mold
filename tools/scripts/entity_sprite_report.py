#!/usr/bin/env python3
"""
Entity Sprite Report Generator

Scans evil-engine extracted assets and generates an HTML report showing:
- All entity types found across levels
- Associated sprite images (first frame of each animation)
- Level/stage distribution

Run from btm project root:
    python3 tools/scripts/entity_sprite_report.py

Output: docs/reports/entity_sprites.html
"""

import os
import sys
import json
import base64
import re
from pathlib import Path
from collections import defaultdict

# Configuration
EVIL_ENGINE_PATH = Path.home() / "projects" / "evil-engine"
EXTRACTED_PATH = EVIL_ENGINE_PATH / "extracted"
OUTPUT_PATH = Path(__file__).parent.parent.parent / "docs" / "reports"

# Level folder order (from entity_sprites.gd)
LEVEL_FOLDERS = [
    "MENU", "GLEN", "SCIE", "CRYS", "WEED", "HEAD",
    "BOIL", "TMPL", "CAVE", "FOOD", "CSTL", "CLOU",
    "PHRO", "WIZZ", "BRG1", "MOSS", "SOAR", "EGGS",
    "FINN", "GLID", "KLOG", "SNOW", "EVIL", "RUNN",
    "MEGA", "SEVN"
]

# Official enemy names from docs/reference/official-enemy-names.md
# Format: internal_type -> { "name": ..., "category": ..., "verified": bool }
KNOWN_ENTITIES = {
    # === BOSSES (All verified by level appearance) ===
    71: {"name": "Monkey Mage", "category": "Boss", "verified": True, "levels": ["WIZZ"]},
    100: {"name": "Glenn Yntis", "category": "Boss", "verified": True, "levels": ["GLEN"]},
    101: {"name": "Shriney Guard", "category": "Boss", "verified": True, "levels": ["MEGA"]},
    102: {"name": "Joe-Head-Joe", "category": "Boss", "verified": True, "levels": ["HEAD"]},
    103: {"name": "Klogg", "category": "Boss", "verified": True, "levels": ["KLOG"]},
    
    # === COLLECTIBLES ===
    2: {"name": "Clay (Orange Balls)", "category": "Collectible", "verified": False, "levels": []},
    3: {"name": "Green Bullets (Energy Balls)?", "category": "Collectible", "verified": False, "levels": []},
    4: {"name": "Pickup", "category": "Collectible", "verified": False, "levels": []},
    8: {"name": "Halo?", "category": "Collectible", "verified": False, "levels": []},
    24: {"name": "Special Pickup", "category": "Collectible", "verified": False, "levels": []},
    25: {"name": "Phart Head (Fart Clone)", "category": "Collectible", "verified": False, "levels": []},  # On Layer 2
    95: {"name": "1970s Icons", "category": "Collectible", "verified": False, "levels": ["SEVN"]},
    
    # === ENEMIES - Need visual verification ===
    5: {"name": "Flying Enemy (Flapper OR Flying Ynt Centurion?)", "category": "Enemy", "verified": False, "levels": []},
    6: {"name": "Flying Enemy Alt (Flapper OR Flying Ynt Centurion?)", "category": "Enemy", "verified": False, "levels": []},
    26: {"name": "Path Enemy (Worker Ynt? Swarm-o-Ynts?)", "category": "Enemy", "verified": False, "levels": []},
    27: {"name": "Path Enemy Variant (Worker Ynt? Swarm-o-Ynts?)", "category": "Enemy", "verified": False, "levels": []},
    29: {"name": "Ground Patrol Enemy", "category": "Enemy", "verified": False, "levels": []},
    
    # === ENEMY TYPES 17-23 - Ground enemies from manual ===
    # These need mapping to: Clay Keeper, Loud Mouth, Mental Monkey, 
    # Tempest Pulsating Monkey, Jumpy the Gorilla, Triple Laser BBM, El Barfo
    17: {"name": "Ground Enemy 17 (Clay Keeper? Loud Mouth?)", "category": "Enemy", "verified": False, "levels": []},
    18: {"name": "Ground Enemy 18 (Mental Monkey?)", "category": "Enemy", "verified": False, "levels": []},
    19: {"name": "Ground Enemy 19 (Tempest Pulsating?)", "category": "Enemy", "verified": False, "levels": []},
    20: {"name": "Ground Enemy 20 (Jumpy the Gorilla?)", "category": "Enemy", "verified": False, "levels": []},
    21: {"name": "Ground Enemy 21 (Triple Laser BBM?)", "category": "Enemy", "verified": False, "levels": []},
    22: {"name": "Ground Enemy 22 (El Barfo?)", "category": "Enemy", "verified": False, "levels": []},
    23: {"name": "Ground Enemy 23", "category": "Enemy", "verified": False, "levels": []},
    
    # === PLATFORMS ===
    28: {"name": "Platform A", "category": "Platform", "verified": False, "levels": []},
    47: {"name": "Platform", "category": "Platform", "verified": False, "levels": []},
    48: {"name": "Platform B", "category": "Platform", "verified": False, "levels": []},
    
    # === INTERACTIVE ===
    42: {"name": "Teleport Ball (Portal)", "category": "Interactive", "verified": False, "levels": []},
    43: {"name": "Teleport Ball (Portal)", "category": "Interactive", "verified": False, "levels": []},
    44: {"name": "Teleport Ball (Portal)", "category": "Interactive", "verified": False, "levels": []},
    45: {"name": "Ma Bird (Checkpoint)", "category": "Interactive", "verified": False, "levels": []},
    
    # === EFFECTS ===
    61: {"name": "Sparkle Effect", "category": "Effect", "verified": False, "levels": []},
}


def image_to_base64(image_path: Path) -> str:
    """Convert image to base64 data URI for embedding in HTML."""
    if not image_path.exists():
        return ""
    with open(image_path, "rb") as f:
        data = base64.b64encode(f.read()).decode("utf-8")
    return f"data:image/png;base64,{data}"


def find_sprite_images(level_path: Path) -> dict:
    """Find all sprite images in a level's extracted folder.
    
    Returns: { sprite_id: { "path": Path, "frames": [Path...] } }
    """
    sprites = {}
    
    # Check primary and stage folders
    for subdir in ["primary/sprites", "stage0/sprites", "stage1/sprites", 
                   "stage2/sprites", "stage3/sprites", "stage4/sprites", "stage5/sprites"]:
        sprites_dir = level_path / subdir
        if not sprites_dir.exists():
            continue
            
        # Find all .res files (sprite definitions)
        for res_file in sprites_dir.glob("sprite_*.res"):
            # Extract sprite ID from filename (e.g., sprite_0x8c510186.res or sprite_1074567325.res)
            name = res_file.stem  # e.g., "sprite_0x8c510186"
            match = re.match(r"sprite_(0x[0-9a-fA-F]+|\d+)", name)
            if not match:
                continue
            
            sprite_id_str = match.group(1)
            if sprite_id_str.startswith("0x"):
                sprite_id = int(sprite_id_str, 16)
            else:
                sprite_id = int(sprite_id_str)
            
            # Find associated frame images
            frames = sorted(sprites_dir.glob(f"{name}_anim*_f*.png"))
            if frames:
                sprites[sprite_id] = {
                    "path": res_file,
                    "frames": frames,
                    "subdir": subdir,
                }
    
    return sprites


def parse_tres_file(tres_path: Path) -> dict:
    """Parse a .tres resource file to extract entity data.
    
    Returns dict with sprite_id -> path mappings.
    """
    if not tres_path.exists():
        return {}
    
    content = tres_path.read_text()
    
    # Extract sprite_paths dictionary
    # Format: sprite_paths = { id: "path", ... }
    sprite_paths = {}
    
    # Find sprite_paths section
    match = re.search(r'sprite_paths\s*=\s*\{([^}]+)\}', content, re.DOTALL)
    if match:
        paths_text = match.group(1)
        # Parse each entry: 123456: "path/to/sprite.res"
        for line in paths_text.split('\n'):
            entry_match = re.match(r'\s*(\d+):\s*"([^"]+)"', line)
            if entry_match:
                sprite_id = int(entry_match.group(1))
                path = entry_match.group(2)
                sprite_paths[sprite_id] = path
    
    return sprite_paths


def scan_level(level_name: str) -> dict:
    """Scan a level's extracted data for entities and sprites.
    
    Returns: {
        "level": level_name,
        "sprites": { sprite_id: { frames: [...], path: ... } },
        "tres_sprites": { sprite_id: path },
    }
    """
    level_path = EXTRACTED_PATH / level_name
    if not level_path.exists():
        return None
    
    result = {
        "level": level_name,
        "sprites": find_sprite_images(level_path),
        "tres_sprites": {},
    }
    
    # Parse .tres file if exists
    tres_file = level_path / f"{level_name}.tres"
    if tres_file.exists():
        result["tres_sprites"] = parse_tres_file(tres_file)
    
    return result


def generate_html_report(level_data: list) -> str:
    """Generate HTML report with embedded sprite images."""
    
    # Collect all unique sprites across levels
    all_sprites = {}  # sprite_id -> { levels: [...], frames: [...] }
    
    for data in level_data:
        if not data:
            continue
        level = data["level"]
        for sprite_id, info in data["sprites"].items():
            if sprite_id not in all_sprites:
                all_sprites[sprite_id] = {
                    "levels": [],
                    "frames": info["frames"],
                    "path": info["path"],
                }
            all_sprites[sprite_id]["levels"].append(level)
    
    html = """<!DOCTYPE html>
<html>
<head>
    <title>Entity Sprite Verification Report</title>
    <style>
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
            background: #1a1a1a;
            color: #e0e0e0;
            margin: 20px;
        }
        h1 { color: #ff9800; }
        h2 { color: #4fc3f7; border-bottom: 1px solid #333; padding-bottom: 10px; }
        h3 { color: #81c784; }
        
        .sprite-grid {
            display: grid;
            grid-template-columns: repeat(auto-fill, minmax(250px, 1fr));
            gap: 20px;
            margin: 20px 0;
        }
        
        .sprite-card {
            background: #2a2a2a;
            border: 1px solid #444;
            border-radius: 8px;
            padding: 15px;
            transition: border-color 0.2s;
        }
        .sprite-card:hover {
            border-color: #ff9800;
        }
        .sprite-card.verified {
            border-color: #4caf50;
            background: #1b3a1b;
        }
        .sprite-card.unverified {
            border-color: #f44336;
        }
        
        .sprite-preview {
            display: flex;
            gap: 5px;
            flex-wrap: wrap;
            margin: 10px 0;
            background: #111;
            padding: 10px;
            border-radius: 4px;
            min-height: 64px;
            align-items: center;
        }
        .sprite-preview img {
            max-width: 64px;
            max-height: 64px;
            image-rendering: pixelated;
            background: #333;
        }
        
        .sprite-id {
            font-family: monospace;
            color: #ff9800;
            font-size: 12px;
        }
        .levels {
            font-size: 11px;
            color: #888;
        }
        
        .entity-name {
            font-weight: bold;
            color: #fff;
        }
        .category {
            display: inline-block;
            padding: 2px 8px;
            border-radius: 4px;
            font-size: 11px;
            margin-left: 8px;
        }
        .category.Boss { background: #f44336; }
        .category.Enemy { background: #ff5722; }
        .category.Collectible { background: #4caf50; }
        .category.Platform { background: #2196f3; }
        .category.Interactive { background: #9c27b0; }
        .category.Effect { background: #607d8b; }
        .category.Special { background: #ffeb3b; color: #000; }
        .category.Unknown { background: #616161; }
        
        .verification-status {
            margin-top: 10px;
            padding: 5px;
            border-radius: 4px;
            font-size: 12px;
        }
        .verification-status.verified {
            background: #1b5e20;
            color: #a5d6a7;
        }
        .verification-status.pending {
            background: #b71c1c;
            color: #ef9a9a;
        }
        
        .verification-checkbox {
            display: flex;
            align-items: center;
            gap: 8px;
            margin-top: 10px;
        }
        .verification-checkbox input {
            width: 18px;
            height: 18px;
        }
        
        .filter-bar {
            background: #2a2a2a;
            padding: 15px;
            border-radius: 8px;
            margin-bottom: 20px;
            display: flex;
            gap: 15px;
            flex-wrap: wrap;
            align-items: center;
        }
        .filter-bar label {
            display: flex;
            align-items: center;
            gap: 5px;
            cursor: pointer;
        }
        .filter-bar input[type="checkbox"] {
            width: 16px;
            height: 16px;
        }
        
        .stats {
            background: #2a2a2a;
            padding: 15px;
            border-radius: 8px;
            margin-bottom: 20px;
        }
        .stats span {
            margin-right: 20px;
        }
        
        #known-entities, #unknown-sprites {
            margin-top: 30px;
        }
    </style>
</head>
<body>
    <h1>🎮 Entity Sprite Verification Report</h1>
    <p>Generated from evil-engine extracted assets. Review sprites and mark as verified.</p>
    
    <div class="filter-bar">
        <strong>Filter:</strong>
        <label><input type="checkbox" id="show-verified" checked> Show Verified</label>
        <label><input type="checkbox" id="show-unverified" checked> Show Unverified</label>
        <label><input type="checkbox" id="show-unknown" checked> Show Unknown</label>
        <label>
            Level: <select id="level-filter">
                <option value="all">All Levels</option>
"""
    
    # Add level options
    for level in LEVEL_FOLDERS:
        if any(d and d["level"] == level for d in level_data):
            html += f'                <option value="{level}">{level}</option>\n'
    
    html += """            </select>
        </label>
    </div>
    
    <div class="stats">
        <span id="total-count">Total: 0</span>
        <span id="verified-count">Verified: 0</span>
        <span id="pending-count">Pending: 0</span>
    </div>
    
    <h2>Known Entity Types</h2>
    <div id="known-entities" class="sprite-grid">
"""
    
    # Generate cards for known entity types
    for entity_type, info in sorted(KNOWN_ENTITIES.items()):
        name = info["name"]
        category = info["category"]
        verified = info["verified"]
        
        verified_class = "verified" if verified else "unverified"
        status_class = "verified" if verified else "pending"
        status_text = "✅ Verified" if verified else "⏳ Needs Verification"
        
        html += f"""
        <div class="sprite-card {verified_class}" data-entity-type="{entity_type}" data-category="{category}" data-verified="{str(verified).lower()}" data-levels="">
            <div>
                <span class="entity-name">Type {entity_type}: {name}</span>
                <span class="category {category}">{category}</span>
            </div>
            <div class="sprite-preview" id="preview-{entity_type}">
                <em style="color: #666;">No sprite linked yet - use Ghidra to find sprite ID</em>
            </div>
            <div class="sprite-id">Internal Type: {entity_type}</div>
            <div class="levels">Levels: <span id="levels-{entity_type}">Checking...</span></div>
            <div class="verification-status {status_class}">{status_text}</div>
            <div class="verification-checkbox">
                <input type="checkbox" id="verify-{entity_type}" {"checked" if verified else ""}>
                <label for="verify-{entity_type}">Mark as verified (visual match confirmed)</label>
            </div>
        </div>
"""
    
    html += """
    </div>
    
    <h2>All Extracted Sprites (by Sprite ID)</h2>
    <p>These are all sprites found in the extracted assets. Match them to entity types above.</p>
    <div id="unknown-sprites" class="sprite-grid">
"""
    
    # Generate cards for all sprites found
    for sprite_id in sorted(all_sprites.keys()):
        info = all_sprites[sprite_id]
        levels = ", ".join(sorted(set(info["levels"])))
        
        # Get first frame as thumbnail
        frames = info["frames"][:4]  # Show up to 4 frames
        
        html += f"""
        <div class="sprite-card" data-sprite-id="{sprite_id}" data-levels="{levels}">
            <div class="sprite-id">Sprite ID: 0x{sprite_id:08x} ({sprite_id})</div>
            <div class="sprite-preview">
"""
        
        for frame in frames:
            img_data = image_to_base64(frame)
            if img_data:
                html += f'                <img src="{img_data}" title="{frame.name}">\n'
        
        if len(info["frames"]) > 4:
            html += f'                <span style="color: #888;">+{len(info["frames"]) - 4} more</span>\n'
        
        html += f"""            </div>
            <div class="levels">Found in: {levels}</div>
            <div>
                <label>
                    Maps to entity type: 
                    <input type="number" class="sprite-entity-map" data-sprite-id="{sprite_id}" 
                           placeholder="Entity type" style="width: 80px; background: #333; border: 1px solid #555; color: #fff; padding: 3px;">
                </label>
            </div>
        </div>
"""
    
    html += """
    </div>
    
    <h2>Export Verification Data</h2>
    <button onclick="exportVerification()" style="padding: 10px 20px; font-size: 14px; cursor: pointer;">
        📋 Copy Verification JSON
    </button>
    <pre id="export-preview" style="background: #2a2a2a; padding: 15px; border-radius: 8px; margin-top: 10px; max-height: 300px; overflow: auto;"></pre>
    
    <script>
        // Filter functionality
        function updateFilters() {
            const showVerified = document.getElementById('show-verified').checked;
            const showUnverified = document.getElementById('show-unverified').checked;
            const showUnknown = document.getElementById('show-unknown').checked;
            const levelFilter = document.getElementById('level-filter').value;
            
            let total = 0, verified = 0, pending = 0;
            
            document.querySelectorAll('.sprite-card').forEach(card => {
                const isVerified = card.dataset.verified === 'true';
                const isUnknown = !card.dataset.entityType;
                const levels = card.dataset.levels || '';
                
                let show = true;
                
                if (isVerified && !showVerified) show = false;
                if (!isVerified && !isUnknown && !showUnverified) show = false;
                if (isUnknown && !showUnknown) show = false;
                
                if (levelFilter !== 'all' && !levels.includes(levelFilter)) show = false;
                
                card.style.display = show ? 'block' : 'none';
                
                if (show) {
                    total++;
                    if (isVerified) verified++;
                    else if (!isUnknown) pending++;
                }
            });
            
            document.getElementById('total-count').textContent = 'Total: ' + total;
            document.getElementById('verified-count').textContent = 'Verified: ' + verified;
            document.getElementById('pending-count').textContent = 'Pending: ' + pending;
        }
        
        document.getElementById('show-verified').addEventListener('change', updateFilters);
        document.getElementById('show-unverified').addEventListener('change', updateFilters);
        document.getElementById('show-unknown').addEventListener('change', updateFilters);
        document.getElementById('level-filter').addEventListener('change', updateFilters);
        
        // Export verification data
        function exportVerification() {
            const data = {
                verified_entities: {},
                sprite_mappings: {},
                timestamp: new Date().toISOString(),
            };
            
            // Collect verified checkboxes
            document.querySelectorAll('.sprite-card[data-entity-type]').forEach(card => {
                const type = card.dataset.entityType;
                const checkbox = card.querySelector('input[type="checkbox"]');
                if (checkbox && checkbox.checked) {
                    data.verified_entities[type] = true;
                }
            });
            
            // Collect sprite -> entity mappings
            document.querySelectorAll('.sprite-entity-map').forEach(input => {
                if (input.value) {
                    data.sprite_mappings[input.dataset.spriteId] = parseInt(input.value);
                }
            });
            
            const json = JSON.stringify(data, null, 2);
            document.getElementById('export-preview').textContent = json;
            
            navigator.clipboard.writeText(json).then(() => {
                alert('Verification data copied to clipboard!');
            });
        }
        
        // Initial filter
        updateFilters();
    </script>
</body>
</html>
"""
    
    return html


def main():
    print("Entity Sprite Report Generator")
    print("=" * 40)
    
    # Ensure output directory exists
    OUTPUT_PATH.mkdir(parents=True, exist_ok=True)
    
    # Scan all levels
    level_data = []
    for level_name in LEVEL_FOLDERS:
        print(f"Scanning {level_name}...", end=" ")
        data = scan_level(level_name)
        if data:
            sprite_count = len(data["sprites"])
            print(f"Found {sprite_count} sprites")
            level_data.append(data)
        else:
            print("Not extracted")
    
    # Generate HTML report
    print("\nGenerating HTML report...")
    html = generate_html_report(level_data)
    
    output_file = OUTPUT_PATH / "entity_sprites.html"
    output_file.write_text(html)
    print(f"Report saved to: {output_file}")
    
    # Summary
    total_sprites = sum(len(d["sprites"]) for d in level_data if d)
    levels_scanned = sum(1 for d in level_data if d)
    print(f"\nSummary:")
    print(f"  Levels scanned: {levels_scanned}")
    print(f"  Total sprites found: {total_sprites}")
    print(f"  Known entity types: {len(KNOWN_ENTITIES)}")
    print(f"  Verified: {sum(1 for e in KNOWN_ENTITIES.values() if e['verified'])}")
    print(f"\nOpen the HTML report in a browser to verify sprites visually.")


if __name__ == "__main__":
    main()
