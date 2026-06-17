"""Generate clean target ID files (audio/sprite/everything) for kcrack."""
import csv

audio = []
sprite = []
all_unkn = []

with open('docs/analysis/asset-identification/cracked_names.csv') as f:
    for r in csv.DictReader(f):
        if not r['id_hex'].startswith('0x'):
            continue
        if r['status'] != 'uncracked':
            continue
        idv = int(r['id_hex'], 16)
        if idv == 0:
            continue  # skip the bogus 0 entry
        if r['type'] == 'audio':
            audio.append(idv)
        elif r['type'] == 'sprite':
            sprite.append(idv)
        all_unkn.append(idv)

with open('/tmp/targets_audio.txt', 'w') as f:
    for i in sorted(set(audio)):
        f.write(f'0x{i:08x}\n')

with open('/tmp/targets_sprite.txt', 'w') as f:
    for i in sorted(set(sprite)):
        f.write(f'0x{i:08x}\n')

with open('/tmp/targets_all.txt', 'w') as f:
    for i in sorted(set(all_unkn)):
        f.write(f'0x{i:08x}\n')

print(f'audio:  {len(set(audio))}  -> /tmp/targets_audio.txt')
print(f'sprite: {len(set(sprite))} -> /tmp/targets_sprite.txt')
print(f'all:    {len(set(all_unkn))} -> /tmp/targets_all.txt')
