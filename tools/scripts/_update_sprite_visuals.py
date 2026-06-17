"""Update sprite_identification_template.csv with visual identifications
gathered from review sheets 00-08 (manual visual review).

Updates the human_role, confidence, and notes columns for the listed sprites.
Does NOT touch the possible_original_name column (that's for verified hash hits).
Marks sprites in the LOCALIZED_23 cohort (confirmed by EN/FR/DE delta analysis).

Run:  python3 tools/scripts/_update_sprite_visuals.py
"""
import csv
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CSV_PATH = ROOT / 'docs/analysis/asset-identification/sprite_identification_template.csv'

# The 23 sprite IDs proven to be localized via EN/FR/DE rotation deltas.
# These are gameplay sprites with text/cosmetic differences per language.
LOCALIZED_23 = {
    0x2a6e0410, 0x326e0410, 0x46269090, 0x492c8430, 0x4a1ea052, 0x4a3ea110,
    0x4a4ec094, 0x4aee0118, 0x4baf8200, 0x4c2a8850, 0x512c8430, 0x523ea110,
    0x524ec094, 0x52ee0118, 0x53af8200, 0x542a8850, 0x5e269090, 0x612c8430,
    0x620ec210, 0x62ee0118, 0x63af8200, 0x7a0ec210, 0x92af8810,
}

# Visual identifications from sheets 00-08
# Format: id -> (human_role, confidence, notes)
# - confidence: HIGH (visually clear), MED (likely but not certain), LOW (guess)
# - All come from manual review of extracted PNG thumbnails
UPDATES = {
    # --- Sheets 00-02 (already partially documented earlier) ---
    0x28c080df: ('DEMO text stamp (renders the word "DEMO" on screen)',
                 'HIGH', 'Visible from sprite thumbnail; likely WRAP-namespace UI text'),
    0x80729081: ('Tutorial sign / hint totem; multi-frame with text: '
                 'CLAY BALL/SWIRLY Q/UNIVERSE ENEMA/SUPER WILLIE/SLAPPY HAMSTER',
                 'HIGH', 'Used by InitCheckpointEntity (line 46679). Parent sprite of 0x88329285 shadow'),
    0x88329285: ('Shadow/decoration for tutorial sign 0x80729081',
                 'HIGH', 'Paired with 0x80729081 in InitCheckpointEntity (line 46703)'),
    0x80e85ea0: ('Hamster Shield orbit icon (green @ symbol)',
                 'HIGH', 'Used by InitHamsterShieldCollectible (CollectibleHamsterShieldTickCallback line 29365)'),
    0x9158a0f6: ('Phoenix Hand projectile sprite (green hand)',
                 'HIGH', 'Per docs/reference/asset-hash-ids.md round 3 ground truth'),
    0x902c0002: ('Super Willie collectible icon',
                 'MED', 'Visually identified from sprite thumbnail'),
    0x88a28194: ('1970 hamster collectible sprite',
                 'HIGH', 'Visually confirmed; paired with FX_PICKUP_1970 audio'),
    0x8c510186: ('Phart Head feather/horn collectible',
                 'HIGH', 'Per docs round 3 ground truth'),
    0xa9240484: ('1-Up extra life collectible icon',
                 'HIGH', 'Per docs round 3 ground truth; paired with FX_PICKUP_ONE_UP audio'),
    0x6a351094: ('Sparkle/star particle (large)',
                 'HIGH', 'Per docs round 3 ground truth'),
    0x6c351094: ('Sparkle/star particle (small/variant of 0x6a351094)',
                 'HIGH', 'Sister of 0x6a351094 (same hash structure, low-byte differs)'),

    # --- Sheet 03 ---
    0xc9387d8c: ('1970 hamster collectible (CLOU level variant)',
                 'HIGH', 'Used in TeleporterActivate context; CLOU level only'),
    0x08624580: ('Small green chevron/triangle pickup',
                 'HIGH', 'Used by TriggerCollectible100CTickCallback (line 28933); plays FX 0x62000441'),
    0x800da102: ('Klogg boss death animation frame 1',
                 'HIGH', 'KloggDeath cohort; KloggDeathEventHandler. KLOG level only'),
    0x800da116: ('Klogg boss death animation frame 2',
                 'HIGH', 'KloggDeath cohort sister of 0x800da102'),
    0x800da11a: ('Klogg boss death animation frame 3',
                 'HIGH', 'KloggDeath cohort sister'),
    0x800da132: ('Klogg boss death animation frame 4',
                 'HIGH', 'KloggDeath cohort sister'),
    0x800da152: ('Klogg boss death animation frame 5',
                 'HIGH', 'KloggDeath cohort sister'),
    0x18e88110: ('Glenn Yntis boss death animation frame 1',
                 'HIGH', 'GlennYntisDeath cohort; GlennYntisDeathEventHandler. GLEN level only'),
    0x18e88310: ('Glenn Yntis boss death animation frame 2',
                 'HIGH', 'GlennYntisDeath cohort sister'),
    0x18e88510: ('Glenn Yntis boss death animation frame 3',
                 'HIGH', 'GlennYntisDeath cohort sister'),
    0x18e88910: ('Glenn Yntis boss death animation frame 4',
                 'HIGH', 'GlennYntisDeath cohort sister'),

    # --- Sheet 04 ---
    0x6c22083a: ('Joe-Head-Joe boss face close-up (dreadlocked head)',
                 'HIGH', 'HEAD level boss; visible large face'),
    0xa9387d8c: ('"1970" text label sprite (yellow text)',
                 'HIGH', 'CLOU level only; the bonus-stage 1970 indicator'),
    0x88783718: ('Moving Platform A (hopping/floating)',
                 'HIGH', 'Per docs round 3: "Moving-Platform-A 0x88783718"'),
    0x4aee0118: ('MEGA boss sprite (localized)',
                 'HIGH', 'One of the LOCALIZED_23 sprites; MEGA boss bank per docs'),
    0x4baf8200: ('WIZZ boss sprite (localized)',
                 'HIGH', 'One of the LOCALIZED_23 sprites; WIZZ boss bank per docs'),
    0x4a3ea110: ('Menu door reveal animation (door opens to show monkey)',
                 'HIGH', 'One of LOCALIZED_23; MENU level transition'),

    # --- Sheet 05 ---
    # FINN player directional family (16 ids around stem 0x10b95810).
    # These are documented in docs/analysis/asset-identification/family-structure-and-categories.md
    # as the FINN player 9-direction wheel (digits 1-9 clockwise from up).
    0x10b85810: ('FINN player directional sprite (Klaymen-flame, frame/dir 1)',
                 'HIGH', 'FINN family stem 0x10b95810 (16 dirs); cracked per round 17 family structure'),
    0x10b91810: ('FINN player directional sprite (frame/dir 2)',
                 'HIGH', 'FINN family stem 0x10b95810'),
    0x10b9181c: ('FINN player directional sprite (frame/dir 3)',
                 'HIGH', 'FINN family stem 0x10b95810'),
    0x10b94810: ('FINN player directional sprite (frame/dir 4)',
                 'HIGH', 'FINN family stem 0x10b95810'),
    0x10b9481c: ('FINN player directional sprite (frame/dir 5)',
                 'HIGH', 'FINN family stem 0x10b95810'),
    0x10b95010: ('FINN player directional sprite (frame/dir 6)',
                 'HIGH', 'FINN family stem 0x10b95810'),
    0x10b9501c: ('FINN player directional sprite (frame/dir 7)',
                 'HIGH', 'FINN family stem 0x10b95810'),
    0x10b95a10: ('FINN player directional sprite (frame/dir 8)',
                 'HIGH', 'FINN family stem 0x10b95810'),
    0x10b95a1c: ('FINN player directional sprite (frame/dir 9)',
                 'HIGH', 'FINN family stem 0x10b95810'),
    0x10b95c10: ('FINN player directional sprite (frame/dir A)',
                 'HIGH', 'FINN family stem 0x10b95810'),
    0x10b95c1c: ('FINN player directional sprite (frame/dir B)',
                 'HIGH', 'FINN family stem 0x10b95810'),
    0x10b97810: ('FINN player directional sprite (frame/dir C)',
                 'HIGH', 'FINN family stem 0x10b95810'),
    0x10b9781c: ('FINN player directional sprite (frame/dir D)',
                 'HIGH', 'FINN family stem 0x10b95810'),
    0x10b9d810: ('FINN player directional sprite (frame/dir E)',
                 'HIGH', 'FINN family stem 0x10b95810'),
    0x10b9d81c: ('FINN player directional sprite (frame/dir F)',
                 'HIGH', 'FINN family stem 0x10b95810'),
    0x10bb5810: ('FINN player directional sprite (frame/dir 16)',
                 'HIGH', 'FINN family stem 0x10b95810'),
    # Other sheet 05 items
    0x88210498: ('Klaymen on fire animation (orange flame, CSTL/MOSS)',
                 'HIGH', ''),
    0xbebce258: ('Red axe weapon sprite (CSTL/SOAR)',
                 'HIGH', ''),
    0x2a6e0410: ('Klaymen hands/arms reaching (EGGS, localized)',
                 'HIGH', 'One of LOCALIZED_23'),
    0x523ea110: ('Klaymen arms reaching (EGGS, localized)',
                 'HIGH', 'One of LOCALIZED_23'),
    0x7a0ec210: ('Klaymen hands reaching (EGGS, localized)',
                 'HIGH', 'One of LOCALIZED_23'),
    0x080c8300: ('Curtain/wall texture (green/blue vertical bars, EVIL/FINN/RUNN)',
                 'MED', 'Repeated bar pattern, possibly background panel'),

    # --- Sheet 06 ---
    0x21842018: ('Klaymen base/player sprite (FINN)',
                 'HIGH', 'Per docs round 3 ground truth: "player/Klaymen 0x21842018"'),
    0x168254b5: ('Player projectile (green ring/oval, FINN)',
                 'HIGH', 'Per docs round 3 ground truth: "projectile 0x168254b5"'),
    0x006b9a6f: ('Yellow centurion/troop mech enemy (FOOD/SNOW)',
                 'HIGH', 'Per cohort docs: "CASTLETROOPER/TROOPER" family'),
    0x53209c1c: ('Yellow centurion mech (FOOD/SNOW sister)',
                 'HIGH', 'TROOPER cohort'),
    0x612c8430: ('Yellow centurion mech (FOOD/SNOW, localized)',
                 'HIGH', 'One of LOCALIZED_23; TROOPER cohort'),
    0x53af8200: ('Yellow centurion mech (FOOD/SNOW, localized sister)',
                 'HIGH', 'One of LOCALIZED_23; TROOPER cohort'),
    0x63af8200: ('Yellow centurion mech (FOOD/SNOW, localized sister)',
                 'HIGH', 'One of LOCALIZED_23; TROOPER cohort'),
    0x408a1c10: ('Yellow disc/pickup (CAVE level)',
                 'MED', ''),
    0xa00a0c30: ('Sparkle/confetti particles (CAVE)',
                 'MED', ''),
    0x407801dc: ('Spider robot (GLEN boss area)',
                 'HIGH', 'Per docs: GLEN level boss/enemy'),
    0x4a1ea052: ('Spider robot variant (GLEN, localized)',
                 'HIGH', 'One of LOCALIZED_23'),
    0x4a4ec094: ('Spider robot variant (GLEN, localized)',
                 'HIGH', 'One of LOCALIZED_23'),
    0x4d09d01c: ('Red juice/potion bottle (GLEN)',
                 'MED', ''),
    0x8c29e08c: ('Red potion/bottle (GLEN)',
                 'MED', ''),
    0x8a7ce022: ('Yellow gold demon/dragon enemy (GLID/SCIE/SOAR/TMPL)',
                 'HIGH', ''),
    0x90a19011: ('Yellow gold demon variant (GLID/SCIE/SOAR/TMPL)',
                 'HIGH', ''),

    # --- Sheet 07 ---
    0x2b3889b2: ('Joe-Head-Joe boss face (dreadlocks/braids, HEAD)',
                 'HIGH', 'HEAD-level boss main face sprite'),
    0x8a3809f2: ('Joe-Head-Joe full body silhouette (HEAD)',
                 'HIGH', ''),
    0x0c34aa22: ('Klogg-ball projectile (yellow ball, KLOG)',
                 'HIGH', 'Per docs round 3 ground truth: "Klogg-ball 0x0c34aa22"'),
    0x08bc8013: ('Klogg boss cluster sprite (KLOG)',
                 'HIGH', ''),
    0x193ca112: ('Klogg boss cluster (KLOG)',
                 'HIGH', ''),
    0x1b280858: ('Klogg shrine/tower with player on top (KLOG)',
                 'HIGH', ''),
    0x1b280c33: ('Klogg shrine with player (KLOG, variant)',
                 'HIGH', ''),
    0x5a088840: ('Klogg boss full body large (KLOG)',
                 'HIGH', ''),
    0x08192250: ('MEGA boss skeleton with skull-head (MEGA)',
                 'HIGH', ''),
    0x085860d4: ('MEGA boss skeleton (MEGA)',
                 'HIGH', ''),
    0x09382152: ('MEGA boss skeleton (MEGA)',
                 'HIGH', ''),
    0x0a1820d4: ('MEGA boss body (MEGA)',
                 'HIGH', ''),
    0x2c182010: ('MEGA boss body (MEGA)',
                 'HIGH', ''),
    0x4c106054: ('Large skeleton boss / Shriney Guard variant (MEGA)',
                 'HIGH', ''),
    0xa8482860: ('Skull-on-bar weapon sprite (MEGA)',
                 'HIGH', ''),
    0x52ee0118: ('MEGA boss bank (localized)',
                 'HIGH', 'One of LOCALIZED_23; MEGA boss per docs'),
    0x62ee0118: ('MEGA boss bank sister (localized)',
                 'HIGH', 'One of LOCALIZED_23'),
    0x0005c699: ('Small skull face (MENU)',
                 'HIGH', ''),
    0x0305a4f5: ('Skull face sprite (MENU, 42 frames)',
                 'HIGH', ''),
    0x28a0c119: ('Three crowned ducklings/characters (MENU)',
                 'HIGH', ''),
    0x30a0c119: ('Three crowned ducklings (MENU, 76 frames)',
                 'HIGH', ''),
    0x40b28011: ('Klaymen with arms up holding object (MENU, yellow garb)',
                 'HIGH', ''),
    0x40b48011: ('Klaymen with arms up (MENU variant)',
                 'HIGH', ''),
    0x5e269090: ('Klaymen with arms up (MENU, localized)',
                 'HIGH', 'One of LOCALIZED_23'),
    0x46269090: ('Klaymen with arms up (WIZZ, localized)',
                 'HIGH', 'One of LOCALIZED_23'),
    0x81100030: ('Controller mapping screen: "3 / NOT USED / JUMP / SHOOT / FAST RUN" (MENU)',
                 'HIGH', 'Default controls display - likely WRAP-namespace UI text'),
    0xe289c059: ('Lit candle sprite (MENU)',
                 'HIGH', ''),
    0xec95689b: ('Brown box / chest (MENU)',
                 'MED', ''),
    0x60181a0c: ('Skull-mage enemy (PHRO/SCIE)',
                 'HIGH', 'Per existing CSV "Plain monkey enemy" labeling'),
    0x611c5804: ('Skull-mage enemy variant (PHRO/SCIE)',
                 'HIGH', ''),

    # --- Sheet 08 ---
    0x88a16190: ('Purple/blue starburst portal (SEVN)',
                 'HIGH', ''),
    0x78d909e0: ('Blue snowball/ice ball (SNOW)',
                 'HIGH', ''),
    0x02098010: ('Chain link sprite (TMPL)',
                 'HIGH', ''),
    0x212a9c2d: ('Brown alien sloth/bear creature (TMPL)',
                 'HIGH', ''),
    0x35289fae: ('Brown alien creature variant (TMPL)',
                 'HIGH', ''),
    0x022e3442: ('Wizard boss cloaked figure (WIZZ)',
                 'HIGH', ''),
    0x0244655d: ('Red ring/halo/donut (WIZZ)',
                 'HIGH', ''),
    0x024d704e: ('Green tadpole projectile (WIZZ)',
                 'HIGH', ''),
    0x02ed1470: ('Wizard boss variant (WIZZ)',
                 'HIGH', ''),
    0x08de7215: ('Green tadpole projectile variant (WIZZ)',
                 'HIGH', ''),
    0x10057441: ('Blue blob projectile (WIZZ)',
                 'HIGH', ''),
    0x181c3854: ('Skull-bar weapon (WIZZ)',
                 'HIGH', ''),
    0x424d1840: ('Wizard boss (WIZZ variant)',
                 'HIGH', ''),
    0x492c8430: ('Wizard boss main sprite (WIZZ, localized)',
                 'HIGH', 'One of LOCALIZED_23; WIZZ boss bank per docs'),
    0x4c2a8850: ('Green tadpole projectile (WIZZ, localized)',
                 'HIGH', 'One of LOCALIZED_23'),
    0xa0149114: ('Green cross/plus icon (WIZZ)',
                 'HIGH', ''),
    0xa0cc1cd0: ('Bone/arm sprite (WIZZ)',
                 'HIGH', ''),
    0xca282402: ('Yellow energy bar (WIZZ)',
                 'HIGH', ''),

    # --- Additional from LOCALIZED_23 not visually identified above ---
    0x326e0410: ('Localized sprite (FOOD/TMPL) - sister of 0x2a6e0410',
                 'MED', 'One of LOCALIZED_23; sister of 0x2a6e0410'),
    0x524ec094: ('Common primary character bank (Klaymen) - localized',
                 'HIGH', 'One of LOCALIZED_23; per docs "common primary character bank"'),
    0x542a8850: ('Monkey/enemy character bank - localized',
                 'HIGH', 'One of LOCALIZED_23; per docs "monkey/enemy character bank"'),
    0x512c8430: ('Monkey/enemy character bank - localized sister',
                 'HIGH', 'One of LOCALIZED_23; sister of 0x542a8850 per docs'),
    0x620ec210: ('Repeated stage object/creature bank - localized',
                 'HIGH', 'One of LOCALIZED_23; per docs "repeated stage object / creature bank"'),
    0x92af8810: ('Localized sprite (per regional probe)',
                 'MED', 'One of LOCALIZED_23'),
}


def main():
    rows = []
    fieldnames = None
    with CSV_PATH.open() as f:
        reader = csv.DictReader(f)
        fieldnames = reader.fieldnames
        for r in reader:
            rows.append(r)

    print(f'Loaded {len(rows)} sprite rows from CSV')

    updated = 0
    added = 0
    existing_ids = {int(r['sprite_hex'], 16): r for r in rows if r['sprite_hex'].startswith('0x')}

    for id_int, (role, conf, notes) in UPDATES.items():
        id_hex = f'0x{id_int:08x}'
        if id_int in existing_ids:
            r = existing_ids[id_int]
            # Only overwrite if currently empty OR if our confidence is HIGH and current is lower
            if not r.get('human_role'):
                r['human_role'] = role
                r['confidence'] = conf
                # Append notes if not already there
                existing_notes = r.get('notes', '') or ''
                if notes and notes not in existing_notes:
                    r['notes'] = (existing_notes + ('; ' if existing_notes else '') + notes).strip('; ')
                updated += 1
            elif conf == 'HIGH' and r.get('confidence') != 'HIGH':
                # Upgrade with HIGH-confidence visual
                r['human_role'] = role
                r['confidence'] = conf
                existing_notes = r.get('notes', '') or ''
                if notes and notes not in existing_notes:
                    r['notes'] = (existing_notes + ('; ' if existing_notes else '') + notes).strip('; ')
                updated += 1
        else:
            # Not in CSV - add a new row
            new_row = {fn: '' for fn in fieldnames}
            new_row['sprite_hex'] = id_hex
            new_row['sprite_decimal'] = str(id_int)
            new_row['human_role'] = role
            new_row['confidence'] = conf
            new_row['notes'] = notes
            rows.append(new_row)
            added += 1

    # Mark LOCALIZED_23 sprites in notes if not already
    for r in rows:
        if not r['sprite_hex'].startswith('0x'):
            continue
        id_int = int(r['sprite_hex'], 16)
        if id_int in LOCALIZED_23:
            existing = r.get('notes', '') or ''
            if 'LOCALIZED_23' not in existing:
                r['notes'] = (existing + ('; ' if existing else '') + 'LOCALIZED_23 (EN/FR/DE variants exist)').strip('; ')

    # Write back
    with CSV_PATH.open('w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        for r in sorted(rows, key=lambda x: int(x['sprite_hex'], 16) if x['sprite_hex'].startswith('0x') else 0):
            writer.writerow(r)

    print(f'Updated {updated} existing rows, added {added} new rows')
    print(f'CSV now has {len(rows)} sprite rows')

    # Stats
    have_role = sum(1 for r in rows if r.get('human_role'))
    have_name = sum(1 for r in rows if r.get('possible_original_name'))
    print(f'Rows with human_role:           {have_role}')
    print(f'Rows with possible_original_name: {have_name}')


if __name__ == '__main__':
    main()
