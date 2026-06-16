"""More attempts at Swirly-Q and Hamster Shield pickup names."""
import sys, os, itertools
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

# Generate variants
T1 = 0x02847462  # Swirly-Q
T2 = 0x42906465  # Hamster (regular)
T3 = 0xc2906565  # Hamster (3rd)

# Try every reasonable name with various capitalizations
def gen_names():
    swirly_ents = ["SWIRLY","SWIRLEY","SWIRL","SWIRLY_Q","SWIRLYQ","SQ","Q","QU",
                   "EXTRA","EXTRAS","ONEUP","ONE_UP","1UP","1_UP","LIFE","LIVES","BONUS","BONUSES",
                   "Q_BALL","QBALL","KLAYMENQ","Q_MARK","QMARK","QSPIN","BLBQ","BLB_Q","ICON",
                   "ICON_Q","SWIRLY_ICON","SWIRL_ICON","KLAYQ","KLAY_Q","KLAYBONUS",
                   "SPINNYQ","SPINNY","SPIN_Q","SPINNY_Q","SPINQ","TWIRL","TWIRLY",
                   "BLBSWIRL","BLB_SWIRL","BLBO","BLB_O"]
    hamster_ents = ["HAMSTER","HAMSTERSHIELD","HAMSTER_SHIELD","HAMSTERS","HAMS","HAM",
                    "FURBALL","FUR","FURRY","FURRIES","FUZZY","FUZZIES","PET","PETS","BUDDY","BUDDIES",
                    "FRIEND","FRIENDS","CRITTER","CRITTERS","FURRY_PET","FURPET","ANIMAL",
                    "ROD","RODENT","RAT","MICE","MOUSE","GUINEA","GUINEAPIG","GP",
                    "SHIELD","SHIELDS","HALO","HALOS","HALOES","HAMSTERHALO","HAMSTER_HALO",
                    "ORBIT","ORBITER","SAT","SATELLITE","ORB","ORBS","RING","RINGS","AURA","AURAS"]
    prefixes = ["FX_PICKUP_","FX_GAIN_","FX_GET_","FX_GRAB_","FX_","SFX_PICKUP_","SFX_",
                "FX_NEW_","FX_TAKE_","FX_BLB_","FX_PWR_","FX_POWER_","FX_BONUS_","FX_HUD_",
                "FX_LIVE_","FX_LIFE_","FX_PWR_UP_","FX_PWRUP_","FX_PICKUP_LIFE_",
                "FX_KLAY_","FX_KLAY_PICKUP_",]
    sufs = ["","_01","_02","_03","_1","_2","_3","_SM","_LG","_S","_L","_FULL","_NORM","_REG",
            "_MAX","_HALF","_QTR","_PWR","_SOFT","_LOUD","_QUIET",
            "_GET","_GRAB","_TAKE","_PICKUP","_ON","_OFF","_END","_DONE",
            "_FX","_AUDIO","_SOUND","_SFX","_BLB","_CHIME","_DING","_BEEP","_TWEET"]
    
    for ent_list, target in [(swirly_ents, T1), (hamster_ents, T2)]:
        for pre in prefixes:
            for e in ent_list:
                for suf in sufs:
                    yield pre + e + suf, target, "swirly" if target == T1 else "hamster"
    
    # For 3rd hamster, also try _03 variants
    for ent in hamster_ents:
        for pre in prefixes:
            for suf in ["_03","_3","_3RD","_FULL","_MAX","_DONE","_END","_LAST","_FINAL","_TOP","_PEAK",
                        "_03_FULL","_FULL_03","_LAST_03","_MAX_03","_TOP_03","_PEAK_03"]:
                yield pre + ent + suf, T3, "hamster_3rd"

hits = {}
total = 0
for name, target, kind in gen_names():
    total += 1
    h = calcHash(name)
    if h == target:
        hits.setdefault((target, kind), []).append(name)

print(f"Tried {total} names")
for (t, kind), names in hits.items():
    print(f"\n0x{t:08x} ({kind}):")
    for n in sorted(set(names))[:10]:
        print(f"  {n}")
