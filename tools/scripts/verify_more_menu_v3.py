"""Continue cracking - target 0x42906465 and 0x64221e61 with more creative names.
These are 'multi-purpose' sounds (hamster pickup + enemy hurt + password unlock for one;
dir-input + wrong-password for other).
"""
import sys, os
sys.path.insert(0, os.path.dirname(__file__))
from calchash import calcHash

T1 = 0x42906465  # hamster pickup / enemy hurt / unlock
T2 = 0x64221e61  # wrong-password / dir input / crawl-climb
T3 = 0xc2906565  # 3rd hamster / full hamster

# Most likely: short multi-purpose name
roots1 = ["HURT","HIT","HURTFLASH","HIT_OK","HIT_FX","HURT_FX","HIT_FLASH",
          "POW","POWER","UNLOCK","CHIME","PICKUP","GAIN","GET","WIN","OK",
          "DONE","COMPLETE","FINISH","END","SUCCESS","SUCCEED","PASS",
          "DAMAGE","FLASH","BLINK","BLINKED","BLINKING","HIT_BLINK","ENEMY_HIT","ENEMY_HURT",
          "BOI","BOING","DOINK","TWANG","CHIRP","DING","CHIME","DING_DONG",
          "WHACK","SMACK","SLAP","KICK","PUNCH","BUMP","KNOCK","CRACK",
          "BUSTED","WHACKED","KNOCKED","STRUCK","SMACKED","SLAPPED","KICKED","PUNCHED",
          "TAKE_DAMAGE","TAKE_HIT","KNOCK_BACK","HIT_BACK","HIT_HURT","HIT_HARD"]
roots2 = ["WRONG","BAD","FAIL","NO","INVALID","ERROR","CANCEL","BACK","BUZZ","BUZZER",
          "INPUT","DIR","DPAD","DPAD_PRESS","DPAD_INPUT","ARROW","PRESS","ARROW_PRESS",
          "MOVE","TICK","TAP","CLICK","SLIDE","SHIFT","CRAWL","CLIMB","CRAB",
          "PRESS_DIR","DIR_INPUT","DIR_TICK","DIR_BLIP","KLAYMOVE","KLAY_MOVE",
          "SCOOT","SHIMMY","SHUFFLE","SCRATCH","SCRABBLE","SCRAPE","STRETCH","DRAG",
          "TRY","ATTEMPT","INPUT_FAIL","INPUT_NO","INPUT_BAD","INPUT_OK","INPUT_GO",
          "PASSWORD_BAD","PASSWORD_NO","PASSWORD_WRONG","PASSWORD_FAIL","PASSWORD_INVALID",
          "PASSWORD_ERR","PASSWORD_ERROR","PASSWORD_TYPE","PASSWORD_PRESS",
          "PASSWORD_BUZZ","PASSWORD_BAD_BUZZ","PASSWORD_FAIL_BUZZ",
          "PWD_BAD","PWD_NO","PWD_WRONG","PWD_FAIL","PWD_INVALID","PWD_ERR","PWD_ERROR",
          "PWD_BUZZ","PWD_BAD_BUZZ","PWD_FAIL_BUZZ",
          "KLAY_CRAWL","KLAY_CLIMB","KLAY_CRAB","KLAY_SHIMMY","KLAY_SCOOT",
          "KLAY_SCRATCH","KLAY_SCRAPE","KLAY_STRETCH","KLAY_DRAG","KLAY_DRAG_SCRATCH"]

for tgt, label, roots in [(T1, "enemy hurt/hamster/unlock", roots1),
                          (T2, "wrong-password/dir-input/crawl", roots2),
                          (T3, "3rd hamster/full", ["HAMSTER_FULL","HAMSTER_3","HAMSTER_MAX",
                                                    "HAMSTER_03","HAM_03","HAM_FULL","HAM_3",
                                                    "FULL","MAX","3RD","COMPLETE","FINAL","DONE",
                                                    "PWR_FULL","POWER_FULL","SHIELD_FULL",
                                                    "HSHIELD_FULL","HAMSHIELD_FULL",
                                                    "TRIPLE","TRIPLE_HAMSTER","THREE_HAMSTER"])]:
    print(f"\n=== 0x{tgt:08x} ({label}) ===")
    found = False
    for r in roots:
        for pre in ["FX_","SFX_","FX_HUD_","SFX_HUD_","FX_BLB_","FX_KLAY_","FX_GAME_","SFX_GAME_",
                    "FX_LEVEL_","SFX_LEVEL_","FX_MENU_","SFX_MENU_","FX_PWD_","SFX_PWD_",
                    "FX_PWR_","SFX_PWR_","FX_PASSWORD_","SFX_PASSWORD_",
                    "FX_INPUT_","SFX_INPUT_","FX_HAM_","SFX_HAM_","FX_HAMSTER_","SFX_HAMSTER_"]:
            for suf in ["","_01","_02","_03","_1","_2","_3","_LO","_HI","_OK","_FX","_BLB","_BUZZ",
                        "_BEEP","_SOFT","_LOUD","_QUIET","_FAST","_SLOW","_FULL","_END","_DONE"]:
                n = pre + r + suf
                h = calcHash(n)
                if h == tgt:
                    print(f"  {n:40s} -> 0x{h:08x} <-- TARGET")
                    found = True
    if not found:
        print(f"  No hit")
