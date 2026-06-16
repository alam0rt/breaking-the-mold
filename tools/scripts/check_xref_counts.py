"""Sister/code-xref check for each currently-promising single hit. Lists 
xref count too.
"""
import sys, os, csv, subprocess
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

# All single-hit candidates from extracted-vocab run, plus a few from prior
CANDIDATES = [
    ("0x08041001", "audio", "WIZZ", 9, "FX_TIE_FX_8"),
    ("0x4824c0c5", "audio", "MEGA", 11, "FX_TINY_ENTER_9"),
    ("0x49609640", "audio", "KLOG", 13, "FX_KLAY_TURBO_2"),
    ("0x60041650", "audio", "CSTL", 13, "FX_KILL_DOOR_04"),
    ("0xd0a14440", "audio", "BRG1", 14, "FX_PICKUP_FLIP_4"),
    ("0xf2810224", "audio", "HEAD", 15, "FX_SKULL_GRUNT_01"),
    ("0x421d1000", "audio", "MENU", 16, "SFX_POWER_TOSS_4"),
    ("0x34014841", "audio", "KLOG;RUNN", 11, "FX_BEEP_BRONZE_08"),
    ("0x6220a452", "audio", "CSTL;MOSS", 11, "FX_CLICK_FLY_02"),
    ("0x02102152", "audio", "BOIL;FOOD", 12, "SFX_SOAR_TWIST_1"),
    ("0xc2820c70", "audio", "EGGS;GLID;WEED", 15, "FX_GLEAM_RUN_05"),
    ("0x40e0824c", "audio", "FINN", 12, "FX_PUFF_FALL_3"),   # COMMITTED
    ("0x40e28045", "audio", "BOIL;BRG1;CAVE;CLOU;...", 10, "<no candidate>"),  # POWERUP_END
    ("0x408a6461", "audio", "BOIL;CLOU;TMPL", 11, "FX_PICKUP_1970"),   # COMMITTED
    ("0x428a6465", "audio", "BOIL;CLOU;TMPL", 13, "FX_PICKUP_1970_03"),  # COMMITTED
    ("0xe0880448", "audio", "14 lvls", 13, "FX_PICKUP_SHIELD"),  # COMMITTED
]

# Check xref count for each
def xref_count(hid_str):
    # search for "0x...." in export/SLES_010.90.c
    try:
        out = subprocess.check_output(
            ["grep", "-c", hid_str, "export/SLES_010.90.c"], text=True
        ).strip()
        return int(out)
    except Exception:
        return 0

print(f"{'ID':12s} {'XRefs':>6s}  {'LEVELS':30s} {'Candidate':30s}")
print("-"*90)
for hid, t, lv, fl, c in CANDIDATES:
    x = xref_count(hid)
    print(f"{hid:12s} {x:>6d}  {lv:30s} {c}")
