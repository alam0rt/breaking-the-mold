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
 */

#include "common.h"

/**
 * GetMovieUnknown00 - Get reserved field from current movie entry
 * Address: 0x8007AE14 | Size: 0x44
 *
 * Returns the u16 at offset +0x00 in the movie entry. This field is always 0
 * in all known BLB versions and appears to be reserved/unused.
 *
 * Only returns a value when in movie mode (header[0xF36] == 1).
 */
INCLUDE_ASM("asm/pal/nonmatchings/BLBHeaderAccessors", GetMovieUnknown00);

/**
 * GetMovieSectorCount - Get sector count from current movie entry
 * Address: 0x8007AE58 | Size: 0x44
 *
 * Returns the u16 at offset +0x02 in the movie entry, which is the number
 * of CD sectors the movie occupies.
 *
 * Only returns a value when in movie mode (header[0xF36] == 1).
 */
INCLUDE_ASM("asm/pal/nonmatchings/BLBHeaderAccessors", GetMovieSectorCount);
