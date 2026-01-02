#include "common.h"

/* Asset type constants for TOC entries */
#define ASSET_TYPE_258 0x258
#define ASSET_TYPE_259 0x259
#define ASSET_TYPE_25A 0x25A

/* Game mode constants */
#define GAME_MODE_LEVEL     3
#define GAME_MODE_SPECIAL   6

/* Forward declarations */
extern void func_8007B074(void *ctx, s32 arg1, s32 arg2, s32 type);

/* 
 * Table of Contents entry in level data
 * 12 bytes per entry
 */
typedef struct {
    u16 count;          /* 0x00: Number of entries (at start of TOC) */
    u16 pad02;          /* 0x02: Padding */
    u32 type;           /* 0x04: Asset type (0x258, 0x259, 0x25A) */
    u32 size;           /* 0x08: Size in bytes */
    u32 offset;         /* 0x0C: Offset from TOC start */
} TocEntry;             /* 0x10 total, but stride is 0x0C due to count being separate */

/*
 * Level metadata entry from BLB header
 * 0x70 bytes per entry, at start of header
 */
typedef struct {
    u16 sectorLo;       /* 0x00: Sector number (low) */
    u16 sectorHi;       /* 0x02: Sector number (high) */
    u32 unk04;          /* 0x04 */
    u32 tocOffset;      /* 0x08: Offset to TOC within loaded data */
    u8 pad0C[0x70-0xC]; /* Rest of entry */
} LevelMetadataEntry;   /* 0x70 total */

/*
 * BLB Header structure (partial)
 */
typedef struct {
    LevelMetadataEntry levels[26];  /* 0x000: Level metadata, 26 entries * 0x70 = 0xB60 */
    u8 padB60[0xECC - 0xB60];       /* Movie table, sector table, etc */
    u16 sectorTable[128][2];        /* 0xECC: Sector table entries (sectorLo, sectorHi) */
    u8 padF34[0xF36 - 0xF34];       /* 0xF34 */
    u8 gameMode;                    /* 0xF36: Game mode (3=level, 6=special) */
    u8 padF37[0xF92 - 0xF37];       /* Padding */
    u8 levelIndex;                  /* 0xF92: Current level index */
} BLBHeader;

/*
 * Level data loading context
 * Passed to func_8007A62C as first argument
 */
typedef struct {
    u32 unk00;              /* 0x00 */
    u32 unk04;              /* 0x04 */
    u32 unk08;              /* 0x08 */
    u32 unk0C;              /* 0x0C */
    u32 unk10;              /* 0x10 */
    u32 unk14;              /* 0x14 */
    u32 unk18;              /* 0x18 */
    u32 unk1C;              /* 0x1C */
    u32 unk20;              /* 0x20 */
    u32 unk24;              /* 0x24 */
    u32 unk28;              /* 0x28 */
    u32 unk2C;              /* 0x2C */
    u32 unk30;              /* 0x30 */
    u32 unk34;              /* 0x34 */
    u32 unk38;              /* 0x38 */
    u32 unk3C;              /* 0x3C */
    u32 unk40;              /* 0x40 */
    u32 unk44;              /* 0x44 */
    u32 unk48;              /* 0x48 */
    u32 unk4C;              /* 0x4C */
    u32 unk50;              /* 0x50 */
    u32 unk54;              /* 0x54 */
    u32 unk58;              /* 0x58 */
    BLBHeader *header;      /* 0x5C: Pointer to BLB header */
    u8 headerOffset;        /* 0x60: Offset within header state array (0xF34 region) */
    u8 pad61[3];            /* 0x61 */
    s32 (*loadCallback)(u16, u16, u16*, BLBHeader*); /* 0x64: CD load callback */
    u16 *tocPtr;            /* 0x68: Pointer to loaded TOC */
    u32 dataOffset;         /* 0x6C: Offset to actual data (after TOC) */
    void *asset258;         /* 0x70: Pointer to asset type 0x258 */
    void *asset259;         /* 0x74: Pointer to asset type 0x259 */
    u32 asset259Size;       /* 0x78: Size of asset type 0x259 */
    void *asset25A;         /* 0x7C: Pointer to asset type 0x25A */
} LevelDataContext;

/*
 * Parse level data from loaded buffer
 * Address: 0x8007A62C | Size: 0x254
 * 
 * Observed header accesses from trace (with headerOffset=12, i.e. 0x0C):
 *   - header[0xF40] at +0x98: state byte (0xF34 + 12 = 0xF40)
 *   - header[0xF9C] at +0xB0: level index (0xF92 + 10 = 0xF9C, slight offset?)
 *   - header[0x000] at +0xE4: Level[0].sector_offset (u16)
 *   - header[0x002] at +0xE8: Level[0].sector_count (u16)
 *   - header[0x008] at +0x148: Level[0].toc_offset (u32) for calculating dataOffset
 *
 * This function:
 * 1. Clears the context structure
 * 2. Reads state from header[0xF34 + headerOffset] to determine game mode
 * 3. Reads level index from header[0xF92 + headerOffset]
 * 4. Uses level index to look up sector offset/count from Level[index]
 * 5. Calls the load callback to read data from CD
 * 6. Parses the TOC and locates assets by type (0x258, 0x259, 0x25A)
 * 
 * @param ctx     Level data context structure (at 0x8009DCC4 in trace)
 * @param buffer  Buffer to load data into (at 0x800AF3E0 = header mirror)
 * @return        1 on success, 0 on failure
 */
#if 1
INCLUDE_ASM("asm/pal/nonmatchings/LevelDataParser", func_8007A62C);
#else
s32 func_8007A62C(LevelDataContext *ctx, u16 *buffer) {
    u8 *headerBase;  /* a3 in original - raw header pointer */
    u8 *header;      /* a2 in original - header + offset */
    u8 gameMode;
    u8 levelIndex;
    u16 sectorLo;
    u16 sectorHi;
    u16 *toc;
    u16 i;
    u32 type;
    u8 *entryPtr;
    u8 *assetPtr;
    u32 assetSize;
    
    /* Load header base pointer first, offset will be loaded during clears */
    headerBase = (u8 *)ctx->header;
    
    /* Clear context fields */
    ctx->tocPtr = NULL;
    ctx->dataOffset = 0;
    ctx->asset258 = NULL;
    ctx->asset259 = NULL;
    ctx->asset259Size = 0;
    ctx->asset25A = NULL;
    ctx->unk00 = 0;
    ctx->unk04 = 0;
    ctx->unk08 = 0;
    ctx->unk20 = 0;
    ctx->unk24 = 0;
    ctx->unk0C = 0;
    ctx->unk10 = 0;
    ctx->unk2C = 0;
    ctx->unk14 = 0;
    ctx->unk18 = 0;
    ctx->unk1C = 0;
    ctx->unk28 = 0;
    ctx->unk38 = 0;
    ctx->unk3C = 0;
    ctx->unk34 = 0;
    ctx->unk30 = 0;
    ctx->unk40 = 0;
    ctx->unk44 = 0;
    ctx->unk48 = 0;
    ctx->unk4C = 0;
    ctx->unk50 = 0;
    ctx->unk54 = 0;
    ctx->unk58 = 0;
    
    /* Compute effective header pointer: a2 = headerBase + headerOffset */
    header = headerBase + ctx->headerOffset;
    
    /* Check game mode at absolute offset 0xF36 from effective header */
    gameMode = *(u8 *)(header + 0xF36);
    if (gameMode < 3) {
        return 0;
    }
    
    /* Get level index at absolute offset 0xF92 */
    levelIndex = *(u8 *)(header + 0xF92);
    sectorLo = 0;
    sectorHi = 0;
    
    switch (gameMode) {
    case GAME_MODE_LEVEL:
        /* Use level metadata table - compute offset from headerBase */
        /* Entry = headerBase + levelIndex * 0x70, read u16 at +0 and +2 */
        sectorLo = *(u16 *)(headerBase + levelIndex * 0x70);
        sectorHi = *(u16 *)(headerBase + levelIndex * 0x70 + 2);
        break;
    case GAME_MODE_SPECIAL:
        /* Use sector table at offset 0xECC from headerBase */
        /* Entry = headerBase + levelIndex * 4 + 0xECC */
        sectorLo = *(u16 *)(headerBase + levelIndex * 4 + 0xECC);
        sectorHi = *(u16 *)(headerBase + levelIndex * 4 + 0xECE);
        break;
    }
    
    /* Call load callback - 4th arg is ctx->header (headerBase before offset) */
    if (!(ctx->loadCallback(sectorLo, sectorHi, buffer, (BLBHeader *)ctx->header) & 0xFF)) {
        return 0;
    }
    
    /* Store TOC pointer */
    ctx->tocPtr = buffer;
    
    /* Reload header for second check */
    header = (u8 *)ctx->header + ctx->headerOffset;
    
    /* Set data offset (only for level mode, mode 3) */
    if (*(u8 *)(header + 0xF36) == GAME_MODE_LEVEL) {
        /* dataOffset = buffer + levels[levelIndex].tocOffset (at +8) */
        ctx->dataOffset = (u32)buffer + *(u32 *)((u8 *)ctx->header + levelIndex * 0x70 + 8);
    } else {
        ctx->dataOffset = 0;
    }
    
    /* Parse TOC entries and locate assets by type */
    toc = ctx->tocPtr;
    for (i = 0; i < *toc; i++) {
        /* Entry pointer: toc + i * 12 bytes */
        entryPtr = (u8 *)toc + i * 12;
        
        /* Asset pointer: tocPtr + entry->offset (at +0xC) */
        assetPtr = (u8 *)ctx->tocPtr + *(u32 *)(entryPtr + 0xC);
        
        /* Type at +0x4, size at +0x8 */
        type = *(u32 *)(entryPtr + 0x4);
        assetSize = *(u32 *)(entryPtr + 0x8);
        
        /* Original uses cascading if-else, not switch */
        if (type == ASSET_TYPE_259) {
            ctx->asset259 = assetPtr;
            ctx->asset259Size = assetSize;
        } else if (type < ASSET_TYPE_25A) {
            if (type == ASSET_TYPE_258) {
                ctx->asset258 = assetPtr;
            }
        } else if (type == ASSET_TYPE_25A) {
            ctx->asset25A = assetPtr;
        }
    }
    
    /* For non-level modes, call func_8007B074 */
    if (*((u8 *)ctx->header + ctx->headerOffset + 0xF36) != GAME_MODE_LEVEL) {
        func_8007B074(ctx, 1, 1, ASSET_TYPE_25A);
    }
    
    return 1;
}
#endif

