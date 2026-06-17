"""Test prefix conventions observed in guesses.html against pickup constraint.
calcHash(PREFIX) = 0x88200080, end-shift = 27
Observed prefixes used in this engine: FX_, BG_, SPR_, SPRT_, SM_, SP_, UI_,
ANM_, MENU_, OBJ_, GFX_, A_, AS_, MS_, MM_, SC_
"""
CS = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789'
STEP = [0]*36
for d, c in enumerate(CS):
    o = ord(c)
    if c.isdigit(): o += 22
    STEP[d] = (o - 64) & 31

def calc(s):
    h, sh = 0, 0
    for c in s.upper():
        if c in CS:
            d = CS.index(c)
            sh = (sh + STEP[d]) & 31
            h ^= 1 << sh
    return h & 0xFFFFFFFF, sh

T, TS = 0x88200080, 27

# Prefix conventions observed in actual engine assets (from guesses.html)
PREFIXES = [
    'FX_','BG_','SPR_','SPRT_','SM_','SP_','UI_','ANM_','MENU_','OBJ_',
    'GFX_','A_','AS_','MS_','MM_','SC_','SS_','S_','M_','P_','T_','G_',
    'AN_','MO_','MC_','MS_','MMS_','ICN_','ICON_','IT_','ITEM_','ENT_',
    'HUD_','MSC_','MISC_','PROP_','BOSS_','EX_','ID_','TYP_','TYPE_',
    'CAT_','GRP_','PIC_','PIX_','TX_','TXT_','TEX_','TIM_','VAB_',
    # Letter abbreviations
    'BC_','BL_','BLB_','BPK_',
    # Neverhood-style two-letter prefixes
    'AA_','AB_','AC_','AD_','AE_','AF_','AG_','AH_','AI_','AJ_','AK_','AL_',
    'AM_','AN_','AO_','AP_','AQ_','AR_','AS_','AT_','AU_','AV_','AW_','AX_','AY_','AZ_',
    'BA_','BB_','BC_','BD_','BE_','BF_','BG_','BH_','BI_','BJ_','BK_','BL_',
    'BM_','BN_','BO_','BP_','BQ_','BR_','BS_','BT_','BU_','BV_','BW_','BX_','BY_','BZ_',
    'CA_','CB_','CC_','CD_','CE_','CF_','CG_','CH_','CI_','CJ_','CK_','CL_',
    'CM_','CN_','CO_','CP_','CQ_','CR_','CS_','CT_','CU_','CV_','CW_','CX_','CY_','CZ_',
    'KM_','KP_','KS_','KK_','KC_',
    # 3-letter
    'SPR','GFX','SFX','PCK','OBJ','BNS','PUP','POW','BLB','HUD','UIS',
    'SPP','SPT','PRP','BNS','MKR','ANM','ANI','MOV','SND','SEQ','MIB',
]

ROOTS = [
    'PICKUP', 'PICKUPS', 'PICK', 'PICKED',
    'OBJ', 'OBJS', 'OBJECT', 'OBJECTS',
    'POW', 'POWUP', 'POWER', 'POWERUP', 'POWERUPS',
    'BNS', 'BONUS', 'BONUSES',
    'ITEM', 'ITEMS',
    'PROP', 'PROPS',
    'GIFT', 'GIFTS',
    'GET', 'GRAB', 'TAKE',
    'COLLECT', 'COLLECTABLE', 'COLLECTIBLE',
    'PRIZE', 'PRIZES', 'GOODY', 'GOODIES',
    'PUP', 'PUPS',
    'DROP', 'DROPS',
    'TOKEN', 'TOKENS',
    # also single-letter
    'P', 'X',
    # Just blank, prefix alone
    '',
]

# Also try suffix categories like _BONUS, _OBJ, _PCK
SUFFIXES = ['', '_BONUS', '_PCK', '_OBJ', '_PUP', '_GIFT', '_NEW', '_ITEM', '_PROP']

exact = []
all_tested = 0
for pre in PREFIXES:
    for root in ROOTS:
        for suf in SUFFIXES:
            name = pre + root + suf
            h, sh = calc(name)
            all_tested += 1
            if h == T and sh == TS:
                exact.append(name)

print(f'Tested {all_tested} candidates')
print(f'Exact matches: {len(exact)}')
for s in exact[:50]:
    print(f'  *** {s!r}')
