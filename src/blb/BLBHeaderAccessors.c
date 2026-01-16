/**
 * BLBHeaderAccessors.c - BLB Header Movie Table Accessor Functions
 *
 * These functions provide access to movie table entries in the BLB header.
 * The movie table is located at header offset 0xB60, with each entry being
 * 0x1C (28) bytes.
 *
 * Movie Entry Layout:
 *   +0x00: u16 - Reserved (always 0)
 *   +0x02: u16 - Sector count
 *   +0x04: 5 bytes - Movie ID
 *   +0x09: 3 bytes - Short name
 *   +0x0C: 16 bytes - Filename
 *
 * State/Index Data (0xF34-0xFFF region):
 * The accessor functions use a sliding window pattern where:
 *   - header[0xF36 + stateOffset] contains the game mode/state
 *   - header[0xF92 + stateOffset] contains the current entry index
 *
 * For movie accessors (stateOffset=2):
 *   - header[0xF38]: Movie state byte (checked against 1 for movie mode)
 *   - header[0xF94]: Current movie index (0-12, used as index * 0x1C into movie table)
 *
 * Observed access patterns from trace:
 *   GetMovieUnknown00:    reads 0xF38, 0xF94, then 0xB60 + index*0x1C
 *   GetMovieSectorCount:  reads 0xF38, 0xF94, then 0xB62 + index*0x1C
 *   GetMovieFilename:     reads 0xF38, 0xF94 (then likely 0xB6C + index*0x1C)
 */

#include "common.h"

/**
 * GetMovieUnknown00 - Get reserved field from current movie entry
 * Address: 0x8007AE14 | Size: 0x44
 *
 * Algorithm (from trace):
 *   1. Read header[0xF38] (movie state byte at +0x10 into function)
 *   2. Read header[0xF94] (movie index at +0x20 into function)
 *   3. Read header[0xB60 + index * 0x1C] as u16 (at +0x38 into function)
 *
 * Returns the u16 at offset +0x00 in the movie entry. This field is always 0
 * in all known BLB versions and appears to be reserved/unused.
 *
 * Only returns a value when in movie mode (header[0xF38] == 1).
 */
INCLUDE_ASM("asm/pal/nonmatchings/blb/BLBHeaderAccessors", GetMovieUnknown00);

/**
 * GetMovieSectorCount - Get sector count from current movie entry
 * Address: 0x8007AE58 | Size: 0x44
 *
 * Algorithm (from trace):
 *   1. Read header[0xF38] (movie state byte at +0x10 into function)
 *   2. Read header[0xF94] (movie index at +0x20 into function)
 *   3. Read header[0xB62 + index * 0x1C] as u16 (at +0x38 into function)
 *
 * Returns the u16 at offset +0x02 in the movie entry, which is the number
 * of CD sectors the movie occupies.
 *
 * Only returns a value when in movie mode (header[0xF38] == 1).
 */
INCLUDE_ASM("asm/pal/nonmatchings/blb/BLBHeaderAccessors", GetMovieSectorCount);
