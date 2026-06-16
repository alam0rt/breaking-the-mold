"""Brute the FINN baby player sprite at 0x21842018."""
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

target = 0x21842018
ACTORS = ['FINN','FINNY','BABY','BABYKLAY','BABYKLAYMEN','BABYMONKEY','BABYBOY',
          'KLAYBABY','KLAYMENBABY','KMANBABY','KIDKLAY','KID',
          'MINI','TINY','SMALL','LITTLE','LIL','MICRO','TINYBABY',
          'BIT','BITTY','MITE','BABYBOSS','BOSSBABY','BBOY','BGIRL',
          'PLAYER','BABYPLAYER','PLAYERBABY','FINNTHE','THEFINN',
          'BABYFINN','FINNBABY','BABYJR','JUNIOR','JR']
ACTIONS = ['','IDLE','STAND','WALK','RUN','JUMP','FALL','DUCK','SLIDE','SIT','LOOK',
           'TURN','BLINK','HURT','DIE','DEATH','GRAB','HOLD','PICKUP',
           'CRAWL','TODDLE','WADDLE','WIGGLE','CRY','LAUGH','GIGGLE',
           'SLEEP','SUCK','EAT','BUBBLE','BLOW','BURP','FART','POOP',
           'SCARED','SMILE','HAPPY','SAD','MAD','PROUD','SHY','BABYJ','BABY1']
PFXS = ['','SPR_','SPRITE_','IMG_','IMAGE_','PIC_','PICTURE_','ANI_','ANIM_','ANIMATION_',
        'STATE_','POSE_','FRAME_','FR_','SEQ_','SEQUENCE_',
        'OBJ_','OBJECT_','ENT_','ENTITY_','ACTOR_','PLAYER_','MDL_','MODEL_','GFX_','GRAPHIC_',
        'CHAR_','CHARACTER_','BIRDIE_','PLAY_','PLAYER1_','PLAYER2_']
SUFS = ['','_1','_2','_3','_4','_5','_6','_7','_8','_9',
        '_01','_02','_03','_04','_05',
        '_A','_B','_C','_D',
        '_BIG','_SM','_LG','_HI','_LO',
        '_LEFT','_RIGHT','_UP','_DN','_DOWN',
        '_LOOP','_END','_START','_MAIN','_ALT']

count = 0
matches = []
for pfx in PFXS:
    for a in ACTORS:
        for v in ACTIONS:
            for sfx in SUFS:
                for sep1, sep2 in [('_','_'),('','_'),('_',''),('','')]:
                    if v:
                        s = pfx + a + sep1 + v + sep2 + sfx.lstrip('_')
                    else:
                        s = pfx + a + sfx
                    count += 1
                    if calcHash(s) == target:
                        matches.append(s)

print(f'Tested {count:,} strings')
for m in matches:
    print(f'  *** {m}')
print(f'Found {len(matches)} matches')

# Also try Klaymen/Skullmonkeys terms
extra = ['SKULLBALL','SKULLMONKEY','KMANGOST','HEADJOE','JOEHEAD','MIKE','SHISH',
         'BIGFAT','FATTY','BIGGUY','TINYGUY','TINYHERO',
         'KLAYMEN_BABY','KLAYMAN_BABY','BABY_KLAYMEN','BABY_KLAYMAN']
for e in extra:
    h = calcHash(e)
    if h == target:
        print(f'  *** {e}')
