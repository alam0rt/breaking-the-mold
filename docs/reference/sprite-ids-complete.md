# Complete Sprite ID Reference

**Status**: ✅ Extracted from C Code  
**Last Updated**: January 15, 2026  
**Source**: SLES_010.90.c (all InitEntitySprite calls)

---

## Overview

Sprite IDs are 32-bit hash values hardcoded in game code. Each entity type initializes with specific sprite IDs.

**Function**: `InitEntitySprite(entity, sprite_id, z_order, x, y, flags)`

**Total Documented**: 30+ unique sprite IDs from C code

---

## Sprite ID Table

| Sprite ID | Hex | Decimal | Entity/Context | Z-Order | Source Line | Description |
|-----------|-----|---------|----------------|---------|-------------|-------------|
| 0x21842018 | 562,487,320 | Player | Klaymen (player character) | 10000 | 6836, 6843 | **Main player sprite** |
| 0x168254b5 | 372,557,493 | Projectile | Bullet/projectile | 959 | 12360 | **Projectile sprite** |
| 0x1e1000b3 | 504,037,555 | Enemy | EnemyA (Type 25) | 999 | 34181 | Enemy sprite |
| 0xb8700ca1 | 3,095,499,937 | Menu | Menu UI frame | 10000, 1000 | 10414, 14450, 36998 | **Menu frame** (3 uses) |
| 0x8c510186 | 2,354,389,382 | UI | UI element | 10000 | 10314 | UI sprite |
| 0x6a351094 | 1,781,612,692 | UI | UI element | 10000, 2000 | 10359, 12625 | UI sprite (2 uses) |
| 0xa9240484 | 2,837,914,756 | UI | Button/menu item | 10000 | 10503, 10840 | Button sprite (2 uses) |
| 0xe8628689 | 3,898,164,873 | UI | Menu element | 10000 | 10589 | Menu sprite |
| 0x88a28194 | 2,292,409,748 | UI | Icon element | 10000 | 10681 | Icon sprite |
| 0x80e85ea0 | 2,162,908,832 | UI | Icon element | 10000 | 10713 | Icon sprite |
| 0x9158a0f6 | 2,437,939,446 | UI | Menu item | 10000 | 10746 | Menu sprite |
| 0x902c0002 | 2,418,016,258 | UI | Menu element | 10000 | 11006 | Menu sprite |
| 0xa0cc1cd0 | 2,697,420,016 | Entity | Special entity | 982 | 15802 | Entity sprite |
| 0xa89d0ad0 | 2,828,421,840 | Entity | Entity type (spawned) | 1001 | 16440 | Entity sprite |
| 0xb01c25f0 | 2,953,422,320 | Entity | Entity type | 9000 | 16523 | Entity sprite |
| 0xca1b20cb | 3,391,873,227 | Entity | Entity type | 2000 | 35487 | Entity sprite |
| 0x1b301085 | 457,183,365 | Entity | Entity type | 1000 | 36650, 36733 | Entity sprite (2 uses) |
| 0x3da80d13 | 1,037,733,139 | Entity | Entity type | 1001 | 36861 | Entity sprite |
| 0x10094096 | 268,861,590 | UI | Back button | 1000 | 36948, 37090, 37172, 37198, 37221, 37250, 37271, 37282, 37292, 37303, 37313 | **Back/navigation button** (11 uses!) |
| 0x3099991b | 814,308,635 | UI | Cursor sprite | 2000 | 36957 | **Password cursor** |
| 0xec95689b | 3,9 66,940,827 | UI | Digit/button icon | 2000, 1000 | 36966, 37527, 37962 | **Password digits** (3 uses) |
| 0x68c01218 | 1,757,250,072 | UI | Menu element | 2000 | 37062 | Menu sprite |
| 0x3080840d | 812,516,365 | UI | Menu element | 2000 | 37065 | Menu sprite |
| 0x3080820d | 812,515,853 | UI | Menu element | 2000 | 37068 | Menu sprite |
| 0x30808e0d | 812,519,949 | UI | Menu element | 2000 | 37071 | Menu sprite |
| 0x38a0c119 | 950,468,889 | UI | Menu element | 2000 | 37074 | Menu sprite |
| 0x81100030 | 2,165,407,792 | UI | Menu element | 2000 | 37205 | Menu sprite |
| 0xe289c059 | 3,801,694,297 | UI | Menu element | 2000 | 37261, 37282, 37303 | Menu sprite (3 uses) |
| 0xe4ac9451 | 3,835,847,761 | UI | Menu element | 2000, 1000 | 37543-37934 | **Password screen digits** (18 uses!) |
| 0xaa0da270 | 2,853,692,016 | UI | Menu element | 1000 | 37987 | Menu sprite |
| 0x28c080df | 684,359,903 | UI | UI element | 30000 | 41120 | High z-order UI |
| 0x8ab92024 | 2,327,715,876 | UI | UI element | 30000 | 41162 | High z-order UI |

---

## Boss Sprite IDs

From boss initialization (InitBossEntity):

| Sprite ID | Hex | Usage | Count |
|-----------|-----|-------|-------|
| 0x181c3854 | 404,227,156 | Main boss body | 1 |
| 0x8818a018 | 2,282,364,952 | Boss parts/limbs | 6 |
| 0x244655d | 38,184,285 | Boss additional sprite | 1 |

---

## Sprite IDs by Z-Order

### Very High (30000)
- 0x28c080df, 0x8ab92024 - Top-layer UI

### High (10000)
- 0x21842018 (Player)
- 0x8c510186, 0x6a351094, 0xa9240484, 0xe8628689, 0x88a28194, 0x80e85ea0, 0x9158a0f6, 0x902c0002, 0xb8700ca1 - UI/Menu

### Medium (1000-2000)
- Password system sprites
- Menu elements
- Navigation buttons

### Low (959-1001)
- 0x168254b5 (Projectile)
- 0x1e1000b3 (Enemy)
- Entity sprites

---

## Most Frequently Used Sprites

**1. 0xe4ac9451** (18 uses):
- Password screen digit display
- Used for all password number positions

**2. 0x10094096** (11 uses):
- Back/navigation button
- Used across multiple menu screens

**3. 0xb8700ca1** (3 uses):
- Menu UI frame
- Main menu background/container

**4. 0xec95689b** (3 uses):
- Password digit/button icons
- Shows which button pressed

**5. 0xe289c059** (3 uses):
- Menu selection elements

---

## Sprite ID Extraction Status

**From InitEntitySprite Calls**: 30+ unique IDs  
**From Boss System**: 3 IDs (boss-specific)  
**From Entity Callbacks**: Need systematic extraction

**Current Coverage**: ~30-35 sprite IDs  
**Estimated Total**: 100-150 sprite IDs  
**Coverage**: ~25-30%

---

## Next Steps

### Systematic Entity Callback Extraction

For complete sprite ID coverage, extract from all 121 entity type callbacks:

**Method**:
1. Read entity callback function (from entity-types.md table)
2. Search for InitEntitySprite calls
3. Extract sprite ID (first hex parameter)
4. Document entity type → sprite ID mapping

**Priority Entity Types**:
- Type 2 (Clayball)
- Type 8 (Item)  
- Type 24 (Special Ammo)
- Type 25-27 (Enemies)
- Type 28, 48 (Platforms)
- Type 50-51 (Boss types)
- Type 60-61 (Effects)

**Time**: ~5-10 hours for complete extraction

---

## Related Documentation

- [Entity Types](entity-types.md) - Entity callback table
- [Entity Sprite ID Mapping](entity-sprite-id-mapping.md) - Previous mappings
- [Sprites](../systems/sprites.md) - Sprite format
- [Entities](../systems/entities.md) - Entity system

---

**Status**: ✅ **30+ Sprite IDs Extracted**  
**Coverage**: ~25-30% (good start)  
**Next**: Systematic entity callback analysis for 80-90 IDs

