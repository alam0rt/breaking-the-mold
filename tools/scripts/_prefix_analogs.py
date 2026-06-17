"""Test analog prefix candidates of varying lengths."""
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

analogs = [
    # Audio prefix: FXPICKUP -> 0x40804460 sh=12. So sprite prefix probably analogous.
    'SPICKUP','S_PICKUP','SP_PICKUP','SPPICKUP','SPRPICKUP','SPRTPICKUP','SPRITEPICKUP',
    'SBONUS','SBNS','SBNUS','SPRBONUS','SPRBNS','SPRBNUS','SPRPOWUP','SPRPWRUP','SPRPOWER',
    'BNUS','BONUS','BLBPICK','BLBPICKUP',
    'PICKUP','PICKUPS','PICKUPER','POWERUP','POWERUPS','PUP','PUPP','PUPS',
    'GAMEOBJ','GAMEOBJECT','OBJECT','OBJECTS','OBJTYPE','OBJ_PICKUP','OBJPICKUP','OBJPCK','OBJPUP','POBJ','POBJECT',
    'PROP','PROPS','PROPERTY',
    'PRIZE','PRIZES','PRIZ',
    'POWUP','POWER','POWERUP','POWUPS',
    'BBLBPICKUP','BLBPICKUP',
    'ITEM','ITEMS','ITEMTYPE','ITEMTYPES',
    'KMEN','KLAYM','KLAYMEN','KLAYBALL','CLAYBALL','CLAYBL','KLAY','CLAY','CLY','KLYM','KLY',
    'OBJ1','OBJ2','OBJ3','OBJ4','OBJ0','PCK1','PCK0','PUP1','PUP0',
    'BNS1','BNS0','POW1','POW0','PWR1','PWR0',
    'P_PICKUP','PPICKUP',
    'CAT0','CAT1','CAT2','CAT3','CAT4',
    'CRYSTAL','CRYS','GOODY','GOODIES','SWAG','PERK',
    'TOKEN','TKN','TKNS','BADGE','MEDAL',
    'PRPCK','PRPICK','BLBSPR','SPRBLB','MSP','MSPR',
    'CAPTURE','CATCH','HUNT','PURSE','SACK',
    'GREEN','BLUE','YELLOW','RED','BLACK','WHITE','GOLDEN',
    'S_PUP','SPUP','SP_PUP','SPRPUP',
    'M_PUP','MPUP','MPCK','M_PCK',
    'GLPICKUP','BLBPCKUP','GLOBPICKUP','GLOBPICK',
    'GLOB','GLOBS','GLOBL','GLOBAL','GLOBALS',
    'KLAYPICK','KLAYPICKUP','KLAYBNS','KLAYBONUS','KLAYGET',
    'KLAYMENPICK','KMENPICK',
    'POPPICK','POPPED','POPPCK','POPER',
    'HOPPICK','HOPPER',
    'MV','MV0','MV1','MV2','MV3','MV4','MV5',
    'SPRPCK','SPRPCKUP',
    'BANK','BANK0','BANK1',
    'CHAR','CHARS','CHRS','CHR',
    'SCN','SCENE','SCN0','SCN1',
    'TYPE0','TYPE1','TYPE','TYPES','TYP','TYPS',
    'P0','P1','P2','P3','P4','P5','P6','P7','P8','P9',
    'PERSON','PEOPLE','PERSONAL','PRSNAL',
    'KASKULL','SKULL','SKL','SKULLM','SKLMKY',
    'NEV','NVR','NEVER','NHOOD','NVH','NVHD',
    'SF','SX','SFX','SFXP','SFX_',
    # FX prefix variants by analogy
    'BG','BGS','BGM','GFX','GFXP','GFX_','SP_','M_','S_',
    # game/level/scene specific
    'LEVEL','LVL','STAGE','STG','SCN','SCENE','MAP','MAPS','ZONE','ZONES','AREA','AREAS','ROOM','ROOMS','LOC','LOCS',
    # what if name structure is X_OBJ_PICKUP_FARTHEAD?
    'XOBJPICKUP','OOBJPICKUP','POBJPICKUP',
    # extra long names
    'SPRITEPICKUP_','SPRITES_PICKUP','SPRITESPICKUP',
    'BACKGROUND','BG_PICKUP','BGPICKUP',
    'PROPS_PICKUP','PROPSPICKUP','PROPPICKUP',
    'TEX','TEXS','TEX_','TXTR','TEXTURE','TEXTURES',
    'TIM','TIMS','TIM_',
    # Numeric variants
    'OBJ_0','OBJ_1','OBJ_2','OBJ_3','OBJ_4','OBJ_5','OBJ_6','OBJ_7','OBJ_8','OBJ_9',
    'PCK_0','PCK_1','PCK_2','PCK_3','PCK_4','PCK_5','PCK_6','PCK_7','PCK_8','PCK_9',
    'PUP_0','PUP_1','PUP_2','PUP_3','PUP_4','PUP_5','PUP_6','PUP_7','PUP_8','PUP_9',
    'BNS_0','BNS_1','BNS_2','BNS_3','BNS_4','BNS_5','BNS_6','BNS_7','BNS_8','BNS_9',
    # File-name style
    'TYPE_PCK','TYPE_OBJ','TYPE_PROP','TYPE_BNS','TYPE_PUP',
    # what if just an abbreviation of "SPRITE_PICKUP_"
    'SPRITEPICKUP','SPRITES_PCK','SPRITESPCK','SPRITE_PCK','SPRITEPCK',
    # Hungarian-style or "abc"
    'OBJS_PICK','OBJSPICK','OBJSPCK','OBJSPUP','OBJSBNS','OBJSBONUS',
    # Shorter form with numbers as type prefix
    '10_PICKUP','01_PICKUP','00_PICKUP','PICKUP_01','PICKUP_10','PICKUP_00',
    # No bonus, but treasure
    'TREAS','TRESR','TRES','TREASURE','TRES_','TREAS_',
    # Word + number
    'PICK_01','PICK_02','PICK_03','PICK_04','PICK_05','PICK_06','PICK_07','PICK_08','PICK_09',
    'POW_01','POW_02','POW_03','POW_04','POW_05','POW_06','POW_07','POW_08','POW_09',
    'BNS_01','BNS_02','BNS_03','BNS_04','BNS_05','BNS_06','BNS_07','BNS_08','BNS_09',
    'OBJ_01','OBJ_02','OBJ_03','OBJ_04','OBJ_05','OBJ_06','OBJ_07','OBJ_08','OBJ_09',
    'OBJ_001','OBJ_002','OBJ_003','OBJ_004','OBJ_005','OBJ_006','OBJ_007','OBJ_008','OBJ_009',
    # Variants of FXPICKUP
    'GFXPICKUP','SFXPICKUP','GFXPCK','SFXPCK',
    # PSY-Q style
    'BLBOBJ','BLBPRP','BLBOBJECT',
    # Klay item naming
    'POOPS','PUPS','PUFF','PUFFS',
    # All single bytes / chars 
    # M_ S_ T_ P_ E_ at the start
    'S_OBJ','SOBJ','SOBJECT','S_OBJECT',
    'M_OBJ','MOBJ','MOBJECT','M_OBJECT',
    'T_OBJ','TOBJ','TOBJECT','T_OBJECT',
    'P_OBJ','POBJ','POBJECT','P_OBJECT',
    'E_OBJ','EOBJ','EOBJECT','E_OBJECT',
    # Type prefix
    'TYP_OBJ','TYPOBJ','TYP_PCK','TYPPCK',
    # Stream/disk-based
    'STR','STRS','STR_','STR_PICKUP','STRPICKUP',
    'STG_PICKUP','STGPICKUP','STG_PCK','STGPCK','STG_PUP','STGPUP','STG_BNS','STGBNS','STG_OBJ','STGOBJ',
    'LEVEL_PICKUP','LVLPICKUP','LVL_PICKUP','LVLPCK','LVL_PCK',
]

exact = []
near = []
for s in analogs:
    h, sh = calc(s)
    if h == T and sh == TS:
        exact.append(s)
    elif sh == TS:
        bd = bin(h ^ T).count('1')
        near.append((bd, s, h, len(s)))

print(f'Tested {len(analogs)} analog candidates')
print(f'Exact matches: {len(exact)}')
for s in exact:
    print(f'  *** EXACT: {s!r}')

near.sort()
print(f'\nNear matches (sh=27, lowest h-distance):')
for bd, s, h, L in near[:30]:
    print(f'  bd={bd:2d}  L={L:2d}  {s!r:<24s} h=0x{h:08x}')
