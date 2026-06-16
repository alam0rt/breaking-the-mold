"""0x40e28045 plays when player[2].hitboxWidth counter expires (reaches 0).
Likely "power-up expired" or "powerup_off" sound.
"""
import sys, os, csv
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

uncracked = {}
cracked = {}
with open("docs/analysis/asset-identification/cracked_names.csv") as f:
    r = csv.DictReader(f)
    for row in r:
        try: hid = int(row["id_hex"], 16)
        except: continue
        if row["status"] == "uncracked":
            uncracked[hid] = (row["type"], row["levels"], int(row["floor"]))
        else:
            cracked[hid] = row.get("name","")

T = 0x40e28045
def mark(h):
    if h == T: return " <-- TARGET 0x40e28045"
    if h in cracked: return f" (CRACKED: {cracked[h]})"
    return ""

names = [
    "FX_POWERUP_END","FX_POWERUP_OFF","FX_POWERUP_LOST","FX_POWERUP_DONE",
    "FX_POWERUP_EXPIRE","FX_PWR_END","FX_PWR_OFF","FX_PWR_LOST",
    "FX_POWER_END","FX_POWER_OFF","FX_POWER_LOST","FX_POWER_DONE",
    "FX_BUFF_END","FX_BUFF_OFF","FX_BUFF_OUT","FX_BUFF_LOST",
    "FX_GLIDE_OFF","FX_GLIDE_END","FX_GLIDE_LOST","FX_GLIDE_DONE","FX_GLIDE_GONE",
    "FX_HALO_OFF","FX_HALO_END","FX_HALO_LOST","FX_HALO_DONE","FX_HALO_GONE","FX_HALO_DESPAWN",
    "FX_SHIELD_OFF","FX_SHIELD_END","FX_SHIELD_LOST","FX_SHIELD_DONE","FX_SHIELD_GONE",
    "FX_BIRD_OFF","FX_BIRD_END","FX_BIRD_LOST","FX_BIRD_DONE","FX_BIRD_GONE","FX_BIRD_DESPAWN",
    "FX_KLAY_DUCK_OFF","FX_KLAY_DUCK_END","FX_KLAY_DUCK_LOST",
    "FX_KLAY_POWER_END","FX_KLAY_PWR_END","FX_KLAY_BUFF_END",
    "FX_BUDDY_OFF","FX_BUDDY_END","FX_BUDDY_LOST","FX_BUDDY_GONE","FX_FRIEND_OFF","FX_FRIEND_END",
    "FX_FLASH","FX_FLASH_END","FX_FLASH_OFF","FX_FLASH_GONE",
    "FX_INVINCIBLE_END","FX_INVINCIBLE_OFF","FX_INVINCIBLE_LOST",
    "FX_INV_END","FX_INV_OFF","FX_INV_LOST","FX_INVUL_END","FX_INVUL_OFF",
    "FX_KLAY_HURT","FX_KLAY_HURT_END","FX_KLAY_HURT_OFF","FX_KLAY_HURT_LOST",
    "FX_KLAY_ARMOR","FX_KLAY_ARMOR_END","FX_KLAY_ARMOR_OFF","FX_KLAY_ARMOR_LOST",
    "FX_ARMOR_END","FX_ARMOR_OFF","FX_ARMOR_LOST","FX_ARMOR_GONE","FX_ARMOR_DONE",
    "FX_KLAY_HALO","FX_KLAY_SHIELD","FX_KLAY_GLIDE","FX_KLAY_BIRD","FX_KLAY_BUDDY",
    "FX_KLAY_POWERUP","FX_KLAY_PWR","FX_KLAY_POW","FX_KLAY_PWR_UP","FX_KLAY_POWERDOWN",
    "FX_KLAY_PWR_DOWN","FX_KLAY_POW_DOWN","FX_PWRDOWN","FX_POWERDOWN","FX_POWER_DOWN",
    "FX_PWR_DOWN","FX_PWRUP","FX_PWR_UP","FX_POWERUP","FX_POWER_UP",
    "FX_INVINCIBILITY_END","FX_HALO_FADE","FX_SHIELD_FADE","FX_GLIDE_FADE","FX_BIRD_FADE",
    "FX_DURATION_END","FX_TIMER_END","FX_TIMER_OFF",
    "FX_HALO_FLASH","FX_SHIELD_FLASH","FX_HALO_BLINK","FX_SHIELD_BLINK",
    "FX_HALO_FX","FX_SHIELD_FX","FX_HALO_LOOP","FX_SHIELD_LOOP",
    "FX_KLAY_FLASH","FX_KLAY_BLINK","FX_KLAY_HURT_OUT",
    "FX_GAS_OFF","FX_GAS_END","FX_GAS_OUT","FX_KLAY_GAS_END","FX_KLAY_GAS",
    "FX_GAS","FX_FART","FX_FART_OUT","FX_FART_END","FX_FART_DONE","FX_FART_OFF",
    "FX_KLAY_FART","FX_KLAY_FART_END","FX_KLAY_FART_OFF","FX_KLAY_FART_OUT",
    "FX_KLAY_FART_FADE","FX_KLAY_BURP","FX_KLAY_BELCH",
]
print("Probing 0x40e28045 (powerup-expire sound) names:")
hit = False
for n in names:
    h = calcHash(n)
    m = mark(h)
    if m and "TARGET" in m:
        print(f"  {n:30s} -> 0x{h:08x}{m}")
        hit = True
    elif m:
        print(f"  {n:30s} -> 0x{h:08x}{m}")
print("\nHit found!" if hit else "\nNo hit in this batch.")
