"""Try many naming patterns for 0x62000441 — collectible/pickup sound.

Source context: TriggerCollectible100CTickCallback dispatches event 0x100C 
(collectible-touched), then PlayEntityPositionSound(entity, 0x62000441).
The "100C" hex is the event-id. Could also be Skullmonkeys-internal collectible
code names like 'CHEKMARK' or 'SCROLL'.
"""
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

target = 0x62000441

# Brute many words
WORDS = []
for r in ['PICKUP','GET','GRAB','TAKE','GOT','COLLECT','TOUCH','HIT','TRIGGER',
          'PRIZE','BONUS','REWARD','AWARD','TROPHY','SCORE','POINT','PT','PTS',
          'GEM','COIN','GOLD','SILVER','BRONZE','TOKEN','CHIP','STAR','HEART',
          'LIFE','LIVES','1UP','UP','EXTRA','POW','POWERUP','POWER',
          'CHIME','DING','BING','BLIP','BEEP','BOOP','TWINKLE','SPARKLE','SHIMMER',
          'BANANA','FRUIT','APPLE','BERRY','CANDY','CHOCOLATE','CHERRY',
          'KEY','LOCK','SCROLL','BOOK','MAP','POTION','HEALTH',
          'ITEM','THING','OBJECT','ENTITY','THING','THINGY','GIZMO',
          'SLEEP','WIN','WINK','PURR','POOF','BANG','BOOM','BLAST',
          'GIVE','GIFT','PRESENT','PASS','FOUND','FIND',
          'CHECK','CHECKMARK','CK','CHK','CKPT','CHECKPOINT',
          'BIG','LITTLE','TINY','SMALL','LARGE','HUGE',
          'BASIC','BASIC_1','BASIC_2','SIMPLE','GENERIC','SOUND','SFX','BEEP_BOOP',
          'BIT','BITS','PIECE','SHARD','FRAGMENT','CHUNK','CRUMB',
          'SKULLMONKEY','SKULL','MONKEY','CLAY','CLAYMAN','CLAYMEN','CLAYBALL',
          'COLLECTIBLE','COLLECT_100C','C100','C100C','100','HUNDRED',
          'BUBBLE','POOPSCOOP','SNATCH','GRAB_IT','GET_IT','GOTCHA',
          'BLING','SHOUT','HOORAY','YAY','YEAH','WOO','WOOHOO',
          'BAUBLE','TRINKET','GADGET','WIDGET','DOOHICKEY','THINGAMAJIG',
          'BOIL','BRG1','CAVE','CLOU','CRYS','CSTL','EGGS','EVIL','FINN','FOOD',
          'GLEN','GLID','HEAD','KLOG','MEGA','MENU','MOSS','PHRO','RUNN','SCIE',
          'SEVN','SNOW','SOAR','TMPL','WEED','WIZZ',
          'BRG','SOAR']:
    WORDS.append(r)

PREFIXES = ['FX_','SFX_','SND_','MUS_','BGM_','UI_',
            'FX_KLAY_','FX_PLAYER_','FX_BONUS_','FX_PRIZE_','FX_PICKUP_',
            'FX_COLLECT_','FX_GET_','FX_GRAB_','FX_HIT_','FX_TRIGGER_',
            'FX_GAME_','FX_LEVEL_','FX_SCORE_','FX_REWARD_','FX_BIG_',
            'FX_ITEM_','FX_GEM_','FX_TOKEN_','FX_BLING_',
            'FX_POW_','FX_POWERUP_','FX_LIFE_','FX_HEART_','FX_STAR_',
            'FX_BLINK_','FX_TINKLE_','FX_SHIMMER_','FX_CHIME_',
            'FX_LAND_','FX_AIR_','FX_BIRD_','FX_SOAR_','FX_BRG1_','FX_BRG_',
            'FX_DECOR_','FX_PIRA_','FX_CRED_','FX_LEGL_',
            'FX_BONUS_GET_','FX_PRIZE_GET_',
            'FX_BIRD_GET_','FX_BIRD_PICKUP_',
            # bare
            '']

SUFFIXES = ['','_1','_2','_3','_4','_5','_01','_02','_03',
            '_LO','_HI','_BIG','_SM','_LG',
            '_LOOP','_END','_START','_MAIN','_ALT',
            '_A','_B','_C']

count = 0
matches = []
seen = set()
for w in WORDS:
    for pfx in PREFIXES:
        for sfx in SUFFIXES:
            s = pfx + w + sfx
            if s in seen: continue
            seen.add(s)
            count += 1
            if calcHash(s) == target:
                matches.append(s)

print(f'Tested {count:,} unique strings')
for m in sorted(matches, key=lambda s: (len(s), s)):
    print(f'  *** {m}')
print(f'Found {len(matches)} total')
