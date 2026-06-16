"""Sister consistency: probe many siblings of proposed cracks against master.

For each plausible candidate from fx_strict, search for sibling assets that would
validate the family naming scheme.
"""
import csv

def calcHash(s):
    h, sh = 0, 0
    for ch in s:
        c = ord(ch)
        if 65 <= c <= 90: pass
        elif 97 <= c <= 122: c -= 32
        elif 48 <= c <= 57: c += 22
        else: continue
        sh = (sh + c - 64) & 31
        h ^= 1 << sh
    return h & 0xFFFFFFFF

ids = {}
with open('docs/analysis/asset-identification/cracked_names.csv') as f:
    for row in csv.DictReader(f):
        if row['id_hex'].startswith('0x'):
            ids[int(row['id_hex'], 16)] = row


def probe(roots, suffixes, label):
    print(f'\n=== {label} ===')
    for r in roots:
        for sfx in suffixes:
            s = r + sfx
            h = calcHash(s)
            if h in ids:
                row = ids[h]
                if row['status'] == 'verified' or row['status'] == 'uncracked':
                    print(f'  {s:38s} -> 0x{h:08x}  {row["status"]:10s} {row.get("name","-"):20s} levels={row["levels"][:30]}')


probe(['FX_PUFF_FALL','FX_PUFF','FX_FINN_FALL','FX_FINN_PUFF','FX_FINN_FART',
       'FX_FINN_HOVER','FX_FINN_FLY','FX_FINN_GAS','FX_FINN_BLOW','FX_FINN_BURP',
       'FX_FINN_DIE','FX_FINN_LAND','FX_FINN_JUMP','FX_FINN_HURT','FX_FINN_HIT',
       'FX_FINN_OUCH','FX_FINN_PUTT','FX_FINN_TURN','FX_FINN_BOB','FX_FINN_RIDE',
       'FX_FINN_BABY','FX_BABY_FALL','FX_BABY_FART','FX_BABY_PUFF',
       'FX_PHART','FX_PHART_FALL','FX_PHART_PUFF','FX_PHART_BLOW','FX_PHART_FART'],
      ['','_1','_2','_3','_4','_5','_6','_7','_8','_9','_01','_02','_03','_04','_05',
       '_LO','_HI','_BIG','_SM','_LG','_A','_B','_C','_LOOP','_END','_START','_MAIN','_ALT',
       '_LEFT','_RIGHT','_UP','_DN','_DOWN','_IN','_OUT'],
      'FINN-bonus level (0x40e0824c)')

probe(['FX_KLAY_DUCK','FX_KLAY_SLIDE','FX_KLAY_BEND','FX_KLAY_LOW','FX_KLAY_CRAWL',
       'FX_KLAY_TUMBLE','FX_KLAY_ROLL','FX_KLAY_FLOOR','FX_KLAY_DOWN'],
      ['','_1','_2','_3','_4','_5',
       '_LO','_HI','_DOWN','_DN','_UP','_LEFT','_RIGHT',
       '_LOOP','_END','_START','_MAIN','_ALT'],
      'KLAY duck (0x4428c941)')

probe(['FX_SKULL_EAT','FX_SKULL_BITE','FX_SKULL_CHOMP','FX_SKULL_CHEW','FX_SKULL_GULP',
       'FX_SKULL_SPIT','FX_SKULL_BURP','FX_SKULL_GROWL','FX_SKULL_PIERCE','FX_SKULL_BREATHE',
       'FX_SKULL_LICK','FX_SKULL_SUCK','FX_SKULL_LAUGH','FX_SKULL_HOWL','FX_SKULL_GROAN'],
      ['','_1','_2','_3','_4','_5','_01','_02','_03','_LOOP','_END','_LO','_HI','_C'],
      'SKULL eat (0x7000c248)')

probe(['FX_SKULL_UP','FX_SKULL_DN','FX_SKULL_DOWN','FX_SKULL_TAP','FX_SKULL_HIT','FX_SKULL_BUMP',
       'FX_SKULL_HOP','FX_SKULL_LIFT','FX_SKULL_RISE','FX_SKULL_DROP'],
      ['','_1','_2','_3','_4','_5','_LO','_HI','_LEFT','_RIGHT'],
      'SKULL up (0x30004240, BRG1 only)')

probe(['FX_BLOW_GALE','FX_BLOW_WIND','FX_WIND_GALE','FX_WIZZ_WIND','FX_WIZZ_GALE',
       'FX_WIZZ_BLOW','FX_WIZZ_HOWL','FX_WIZZ_RUMBLE','FX_WIZZ_AMBIENT'],
      ['','_1','_2','_3','_LO','_HI','_BIG','_SM','_LG','_LOOP','_END','_START','_MAIN','_ALT'],
      'WIZZ wind (0x0e041001)')

probe(['FX_PROJECTILE_SHOCK','FX_PROJ_SHOCK','FX_BLAST_SHOCK','FX_SHOCK','FX_BLAST',
       'FX_PROJECTILE_HIT','FX_PROJECTILE_BURN','FX_PROJECTILE_FIRE','FX_PROJECTILE_BOUNCE'],
      ['','_1','_2','_3','_LO','_HI','_BIG','_SM','_LG','_LOOP','_END','_LEFT','_RIGHT'],
      'PROJECTILE shock (0x502840c3)')

probe(['FX_MENU','FX_MENU_SELECT','FX_MENU_CONFIRM','FX_MENU_OK','FX_MENU_CANCEL','FX_MENU_NO',
       'FX_MENU_YES','FX_MENU_PAUSE','FX_MENU_OPEN','FX_MENU_CLOSE','FX_MENU_BLINK',
       'FX_MENU_SLIDE','FX_MENU_TICK','FX_MENU_BEEP','FX_MENU_BLIP',
       'FX_UI','FX_UI_SELECT','FX_UI_OK','FX_UI_CANCEL','FX_UI_PAUSE',
       'FX_CURSOR','FX_CURSOR_MOVE','FX_CURSOR_OK'],
      ['','_1','_2','_3','_LO','_HI','_LOOP','_END','_START','_MAIN','_ALT'],
      'MENU SELECT (0x90810000)')
