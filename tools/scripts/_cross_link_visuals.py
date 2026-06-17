"""Cross-link cracked_names.csv with visual identifications from sprite_identification_template.csv.

For each uncracked sprite/audio in cracked_names.csv, if a corresponding
visual identification exists in sprite_identification_template.csv, append
the human_role to the notes column (without overwriting existing notes).

This makes cracked_names.csv self-contained — users browsing it for
uncracked IDs immediately see what each sprite is visually, even when the
hash hasn't been cracked.
"""
import csv
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
AID = ROOT / 'docs/analysis/asset-identification'
CRACKED = AID / 'cracked_names.csv'
SPRITES = AID / 'sprite_identification_template.csv'

# Load visual identifications by ID
visuals = {}
with SPRITES.open() as f:
    for r in csv.DictReader(f):
        if not r['sprite_hex'].startswith('0x'):
            continue
        if not r.get('human_role'):
            continue
        idv = int(r['sprite_hex'], 16)
        visuals[idv] = (r['human_role'], r.get('confidence', ''), r.get('notes', '') or '')

print(f'Loaded {len(visuals)} sprite visual identifications')

# Update cracked_names.csv
rows = []
fieldnames = None
with CRACKED.open() as f:
    reader = csv.DictReader(f)
    fieldnames = reader.fieldnames
    rows = list(reader)

updated = 0
# The notes/description field in cracked_names.csv is named "alts"
NOTES_COL = 'alts'
for r in rows:
    if r['status'] != 'uncracked':
        continue
    if not r['id_hex'].startswith('0x'):
        continue
    idv = int(r['id_hex'], 16)
    if idv not in visuals:
        continue
    role, conf, vnotes = visuals[idv]
    existing_notes = (r.get(NOTES_COL, '') or '').strip()
    # Build visual-tag: "VISUAL [HIGH]: Klogg boss cluster (KLOG)"
    visual_tag = f'VISUAL [{conf}]: {role}'
    if 'VISUAL' in existing_notes:
        continue  # already has a visual tag, skip
    if visual_tag not in existing_notes:
        sep = '; ' if existing_notes else ''
        r[NOTES_COL] = (existing_notes + sep + visual_tag).strip()
        updated += 1

# Write back (use extrasaction='ignore' to defend against unknown keys)
with CRACKED.open('w', newline='') as f:
    writer = csv.DictWriter(f, fieldnames=fieldnames, extrasaction='ignore')
    writer.writeheader()
    for r in rows:
        writer.writerow(r)

print(f'Updated {updated} cracked_names.csv notes with visual labels')

# Stats
uncracked_with_visual = sum(
    1 for r in rows
    if r['status'] == 'uncracked' and 'VISUAL' in (r.get(NOTES_COL) or '')
)
print(f'Uncracked sprites now with VISUAL label in notes: {uncracked_with_visual}')
