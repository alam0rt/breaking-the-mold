"""Test prefix candidates."""
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

target_h, target_sh = 0x88200080, 27

cands = [
    'OBJECT','OBJEC','OBJ','OBJS','OBJEKT','OBJTYP',
    'ITEM','ITEMS','PROP','PROPS','THING','THINGS','GAIN','GIFT','TREAT',
    'PICKUP','PICK','PICKS','PCKUP','PICKER','PCKER','PICKD',
    'BNS','BONUS','BONUSS','BNUS','BONUSE','GRAB','TAKE','TAKES',
    'POWER','POWERS','POWUP','POW','PWR','PWRUP',
    'KLAY','KLAYS','KLAYM','KLAYMN','KMEN','KLAYMS',
    'SPRITE','SPRT','SPRTS','SPR','SPRS','GFX','SFX','FX','SFXS',
    'MAIN','MAINS','MAINY','MAINP','MAINPK','TYPE','TYPES','MISC',
    'GLOB','GLOBS','GLOBL','GLBL','GLOBE','GLOBAL',
    'BASE','BASES','BASEP','BASEPK','BASEPU',
    'MENU','MENUS','MNU','MNUS','MENUE','MENUEX',
    'PROT','PROTS','PROTO','PROTON',
    'HERO','HEROES','HEROS','PLAYER','PLAYR','PLAY','PLAYS',
    'WORLD','WORLDS','WRLD','LEVEL','LVLS','LVL','LEVELS',
    'STAGE','STG','STGS','SCENE','SCN','SCNS',
    'MOVIE','MV','MVS','SCREEN','SCRN','SCRNS',
    'GAME','GAMES','GM','GMS','GMENU','GAMENU',
    'BLB','BLBS','BLBP','BLBPK','BLBPKP',
    'DROP','DROPS','DRP','DRPS',
    'CATCH','LOOT','LOOTS',
    'WAND','WANDS','SWORD','SWORDS','WEAPON','WEAP','WEAPS',
    'CLAY','CLAYS','CLAYM','CLAYB','BALL','BALLS','CLAYBL',
    'GET','GETS','GRAB','GRABS','HAVE','HAVES',
    'CARRY','HOLDS','HOLD','HOLDER',
    'KEEP','KEEPS','SHARP','SHARPE','SPARE','SPARES','SQUARE','SQUAR',
    'STAR','STARS','START','STARTS',
    'EGGS','EGG',
    'POWBOX','POWERB','POWERX',
    'POO','POOP','POOPS','PUP','PUPS','PUPP','PUPPY',
    'COIN','COINS','GOLD','GOLDS','GEM','GEMS',
    'KEY','KEYS','LOCK','LOCKS',
    'POTION','POT','POTS','VIAL','VIALS',
    'CHARM','CHARMS','RING','RINGS','TOKEN','TOKENS',
    'NEW','OLD','BIG','SMALL','RARE','COMMON',
    'KLY','KLYM','KLYMN','KLAYM','KLAYMN',
    'SKULL','SKULLS','SKLL','MONK','MONKS','MONKEY','MONKE',
    'CRYS','CRYST','CRYSTAL',
    'BLB','SPR','MAP','TIM','XA','VAB','MOV','STR',
    'GAMEOBJ','OBJGAME','TYPEOBJ','OBJTYPE','ITEMOBJ','OBJITEM',
    'MAINGM','MAINGAME','GAMEMA','GMMAIN',
    'POWPCK','PCKPOW','POWPICK',
    'OBJPCK','OBJPICK','PCKOBJ','PICKOBJ',
    'PROP','PROPS','PRPS','PROPSPR',
    # 4 chars: lots of common 4-letter words
    'TIME','TIDE','THAT','THEN','THEY','THIS','THUD','THUG',
    'GOLD','GOOD','GROW','GULP',
    'GIRL','GIVE','GUSH','GUST',
    'GUMP','GUMS','HUGE','HUFF',
    'INKS','LACE','LAME','LANE','LATE',
    'MAID','MAKE','MAKER',
    'MATE','MATH','MATS','MEAL','MEAN','MEAT',
    'MICE','MILE','MIME','MIND','MINI','MINK','MIST','MITE',
    'MUTE','NAIL','NAME',
    'NEAR','NEED','NEON','NEST','NEWT',
    'NICE','NINE','NOPE','NORM',
    'NUMB','OATH','OBOE','OPEN','ORAL',
    'ORCA','OUCH','OURS','OUST','OVAL',
    'PACE','PAGE','PAIN','PAIR',
    'PALM','PAPA','PAPE',
    'PARK','PART','PASS','PAST',
    'PEAR','PEAS','PEAT','PEEL','PEER',
    'PEST','PIES','PIKE','PILE','PILL','PIMP','PINE','PINK','PINT',
    'PIPE','PISH','PLAN','PLAY','PLEA','PLOD','PLOP','PLOT','PLOW',
    'PLUG','PLUM','PLUS','POEM','POET','POKE','POLE','POLL','POLO',
    'POND','PONY','POOL','POOR','POPE','POPS','PORE','PORK','PORT',
    'POSE','POSH','POST','POUR','POUT',
    'PRAM','PRAY','PREP','PREY','PRIM','PROD','PROM','PROP','PROS',
    'PUMP','PUNK','PUNS','PUNY','PURE','PURL','PURR','PUSH','PUSS',
    'QUAD','QUAY','QUID','QUIP','QUIT','QUIZ',
    'RACE','RACK','RAGE','RAID',
    'RANG','RANK','RANT','RAPS','RARE','RASH',
    'RATE','RAVE','RAYS','READ','REAL','REAM','REAP','REAR',
    'RIDE','RIFE','RIFT','RILE','RIMS','RING','RINK','RIPE','RIPS',
    'ROAD','ROAM','ROBE','ROCK','RODE','RODS','ROLE','ROLL','ROMP',
    'ROOM','ROOT','ROPE','ROSE','ROSY','ROUT','ROVE','ROWS','RUBE',
    'RUBY','RUDE','RUGS','RUIN','RULE','RUMP','RUNG','RUNS','RUSE',
    'RUSH','RUST','SACK','SAFE','SAGA','SAGE','SAID','SAIL','SALE',
    'SALT','SAME','SAND','SANE','SANG','SANK','SAPS','SARI','SASH',
    'SAVE','SAWS','SAYS','SCAB','SCAN','SCAR','SCAT','SCUD','SCUM',
    'SEAL','SEAM','SEAR','SEAS','SEAT','SECT','SEED','SEEK','SEEM',
    'SEEN','SEER','SEES','SELF','SELL','SEMI','SEND','SENT','SEPT',
    'SERF','SHAM','SHED','SHEW','SHIN','SHIP','SHOD','SHOE','SHOO',
    'SHOP','SHOT','SHOW','SHUN','SHUT','SHWA','SICK','SIDE','SIFT',
    'SIGH','SIGN','SILK','SILL','SILO','SILT','SING','SINK','SINS',
    'SIPS','SIRE','SITE','SITS','SIZE','SKID','SKIM','SKIN','SKIP',
    'SLAB','SLAG','SLAM','SLAP','SLED','SLEW','SLID','SLIM','SLIP',
    'SLOG','SLOP','SLOT','SLOW','SLUG','SLUM','SLUR','SMOG','SMUG',
    'SNAP','SNIP','SNOB','SNOT','SNOW','SNUB','SNUG','SOAK','SOAP',
    'SOAR','SOBS','SOCK','SODA','SOFA','SOFT','SOIL','SOLD','SOLE',
    'SOLO','SOME','SONG','SONS','SOON','SOOT','SOPS','SORE','SORT',
    'SOUL','SOUP','SOUR','SOWN','SOWS','SPAN','SPAR','SPAT','SPED',
    'SPIN','SPIT','SPRY','SPUD','SPUN','SPUR','STAB','STAG','STAR',
    'STAY','STEM','STEP','STEW','STIR','STOP','STOW','STUB','STUD',
    'STUN','STYE','SUCH','SUCK','SUDS','SUED','SUES','SUET','SULK',
    'SUMP','SUMS','SUNG','SUNK','SUNS','SUPS','SURE','SURF','SWAB',
    'SWAG','SWAM','SWAN','SWAP','SWAT','SWAY','SWIG','SWIM','SWUM',
    'TACK','TACO','TADS','TAGS','TAIL','TAKE','TALE','TALK','TALL',
    'TAME','TAMP','TANG','TANK','TAPE','TAPS','TARN','TARO','TARP',
    'TART','TASK','TAUT','TAXI','TEAL','TEAM','TEAR','TEAS','TEAT',
    'TECH','TEDS','TEED','TEEM','TEEN','TEES','TELL','TEND','TENT',
    'TERM','TERN','TEST','TEXT','THAN','THAT','THAW','THEE','THEM',
    'THEN','THEW','THEY','THIN','THIS','THUD','THUG','TICK','TIDE',
    'TIDY','TIED','TIER','TIES','TIFF','TIKE','TILE','TILL','TILT',
    'TIME','TINE','TINS','TINT','TINY','TIPS','TIRE','TITS','TOAD',
    'TODS','TODY','TOED','TOES','TOFF','TOFU','TOGA','TOGS','TOIL',
    'TOKE','TOLD','TOLL','TOMB','TOMS','TONE','TONG','TONS','TOOK',
    'TOOL','TOOT','TOPI','TOPS','TORE','TORN','TORS','TORY','TOSS',
    'TOTE','TOTS','TOUR','TOUT','TOWN','TOWS','TOYS','TRAM','TRAP',
    'TRAY','TREE','TREK','TRET','TREY','TRIM','TRIO','TRIP','TROD',
    'TROT','TRUE','TUBA','TUBE','TUBS','TUCK','TUFA','TUFF','TUFT',
    'TUGS','TUNA','TUNE','TURF','TURN','TUSH','TUSK','TUTS','TWAS',
    'TWIG','TWIN','TWIT','TWOS','TYKE','TYPE','UGLY','ULNA','UNDO',
    'UNIT','UNTO','UPON','URGE','URNS','USED','USER','USES','VAIN',
    'VALE','VAMP','VANE','VANS','VARY','VASE','VAST','VATS','VEAL',
    'VEEP','VEER','VEIL','VEIN','VEND','VENT','VERB','VERY','VEST',
    'VETO','VIAL','VICE','VIED','VIES','VIEW','VIGS','VIKING','VILE',
    'VINE','VISA','VISE','VISTA','VITAL','VIVID','VOCAL','VOGUE',
    'VOICE','VOID','VOTE','VOWS','WACK','WADE','WADI','WAFT','WAGE',
    'WAGS','WAIF','WAIL','WAIT','WAKE','WALE','WALK','WALL','WALT',
    'WAND','WANE','WANS','WANT','WARD','WARE','WARM','WARN','WARP',
    'WARS','WART','WARY','WASH','WASP','WAST','WAVE','WAVY','WAXY',
    'WAYS','WEAK','WEAL','WEAN','WEAR','WEBS','WEED','WEEK','WEEP',
    'WEFT','WEIR','WELD','WELL','WELT','WEND','WENT','WEPT','WERE',
    'WEST','WHAM','WHAT','WHEN','WHET','WHEW','WHEY','WHIM','WHIP',
    'WHIR','WHIT','WHIZ','WHOA','WHOM','WHOP','WHYS','WICK','WIDE',
    'WIFE','WIGS','WILD','WILE','WILL','WILT','WILY','WIMP','WIND',
    'WINE','WING','WINK','WINO','WINS','WINY','WIPE','WIRE','WIRY',
    'WISE','WISH','WISP','WIST','WITH','WIVE','WOAD','WOES','WOKE',
    'WOLD','WOLF','WOMB','WOMP','WONK','WONT','WOOD','WOOF','WOOL',
    'WOOS','WORD','WORE','WORK','WORM','WORN','WOVE','WOWS','WRAP',
    'WREN','WRIT','WRYE','XRAY','YACK','YAGI','YAKS','YAMS','YANG',
    'YANK','YARD','YARE','YARN','YAWL','YAWN','YAWS','YEAR','YEAS',
    'YELL','YELP','YIPE','YIPS','YOGA','YOGI','YOKE','YOLK','YORE',
    'YOUR','YOWL','YULE','YURT','ZANY','ZAPS','ZEAL','ZEBU','ZERO',
    'ZEST','ZINC','ZINE','ZING','ZIPS','ZITI','ZITS','ZONE','ZONK',
    'ZOOM','ZOOS','ZORI','ZOUK','ZOUR',
    # Additional 6 char words
    'POPPER','POPPED','HOPPER','HOLDER','GETTER','POOPED','POOPER','BOOPED','BOOPER',
    'HUNTED','GIFTED','TAKEN','GIVEN','MARKED','GRABBED',
    'PICKER','PICKED','PUFFED','PUFFER','GLIDED','GLIDER',
    'TARGET','TARGTS','TOUCHED','TOUCH',
    'OBJET','OBJTS','PROPER','PROPRS','PROPLM','POPPY',
    'KEEPER','HOLDER','BLBPCK','BLBPUP','BLBOBJ',
    'GAMER1','GAMER2','MAIN01','MAIN02',
    'PROTOS','PROTOT','PROTON','PROTOZ','PROXY',
    'KLAY01','KLAY02','KLAY03','KLAYP1','KLAYS1',
    'KLYMAN','KLYMNS','KLYMS1','KMENS1',
    'HEROIC','HERONS','HEROIC',
    'PLATER','MARKED','MARKER',
    'NIRMAL','NORMAL','NORMS1',
    'CLAYED','CLAYER','CLAYME','CLAYM1','CLAYM2','CLAYMA','CLAYMN','CLAYMS',
    'BLBPCK','BLBGET','BLBPRO','BLBPUP','BLBBAS','BLBMAI',
    'PCKBLB','PCKMAI','PCKMAIN','PCKGET',
]
print(f'Testing {len(cands)} candidates against h={target_h:#010x} sh={target_sh}...')
matches = []
near = []  # same sh, low h-dist
for c in cands:
    h, sh = calc(c)
    if h == target_h and sh == target_sh:
        matches.append(c)
    elif sh == target_sh:
        bd = bin(h ^ target_h).count('1')
        if bd <= 6:
            near.append((bd, c, h))

print()
print(f'{len(matches)} EXACT matches:')
for m in matches:
    print(f'  *** {m!r}')

near.sort()
print(f'\n{len(near)} candidates with same sh, low h-distance:')
for bd, c, h in near[:30]:
    print(f'  bd={bd:2d} {c!r:<12s} h=0x{h:08x}')
