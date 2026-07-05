/* =============================================================================
 * DecodeRLESpriteCore.c  --  PC-port RLE sprite pixel decoder (core)
 * =============================================================================
 * Faithful transcription of export/SLES_010.90.c DecodeRLESpriteCore
 * (0x80010264). Decodes RLE-compressed 8-bit sprite pixels into an output
 * framebuffer, supporting both normal (left-to-right) and horizontally-flipped
 * (right-to-left) scanline emission. May suspend mid-decode and resume.
 *
 * rle_state[] (6 ints, produced by GetFrameMetadata):
 *   [0] rows_remaining     [1] row_stride_bytes   [2] row_base_ptr
 *   [3] rle_token_ptr      [4] pixel_data_ptr      [5] flip_flag
 *
 * RLE token (16-bit): bit15 = advance to next row, bits[14:8] = horizontal skip,
 * bits[7:0] = run length. Pixel copy is unrolled 8 / 4 / 1 bytes per pass.
 *
 * The original's stores of the incoming $s0-$s3 to g_dwScratchSave_* are MIPS
 * register-preservation artifacts with no effect on the decoded output, so they
 * are omitted here.
 * ---------------------------------------------------------------------------*/
#include "common.h"
#include <stdint.h>

int DecodeRLESpriteCore(void *ctx, int *rle_state, short max_rows) {
    int rows = rle_state[0];
    unsigned short *token = (unsigned short *)rle_state[3];
    unsigned char *pixel = (unsigned char *)rle_state[4];
    unsigned char *dst = (unsigned char *)rle_state[2];
    int stride = rle_state[1];
    unsigned char *rowBase = dst;
    int n;
    (void)ctx;

    if ((short)rle_state[5] == 0) {
        /* normal: emit pixels left-to-right */
        for (; rows != 0; rows = rows - 1) {
            unsigned short tok = *token;
            unsigned int runLen = tok & 0xff;
            if ((tok & 0x8000) != 0) {
                max_rows = max_rows - 1;
                if (max_rows == -1) {
                    goto suspend;
                }
                dst = rowBase + stride;
                rowBase = dst;
            }
            dst = dst + ((tok & 0x7f00) >> 8);
            if (((runLen - 1) & 0xffff) != 0xffff) {
                for (n = (int)runLen - 9; -1 < n; n = n - 8) {
                    dst[0] = pixel[0];
                    dst[1] = pixel[1];
                    dst[2] = pixel[2];
                    dst[3] = pixel[3];
                    dst[4] = pixel[4];
                    dst[5] = pixel[5];
                    dst[6] = pixel[6];
                    dst[7] = pixel[7];
                    pixel = pixel + 8;
                    dst = dst + 8;
                }
                for (n = n + 4; -1 < n; n = n - 4) {
                    dst[0] = pixel[0];
                    dst[1] = pixel[1];
                    dst[2] = pixel[2];
                    dst[3] = pixel[3];
                    pixel = pixel + 4;
                    dst = dst + 4;
                }
                n = n + 4;
                do {
                    *dst = *pixel;
                    pixel = pixel + 1;
                    n = n - 1;
                    dst = dst + 1;
                } while (-1 < n);
            }
            token = token + 1;
        }
    } else {
        /* flipped: emit pixels right-to-left */
        for (; rows != 0; rows = rows - 1) {
            unsigned short tok = *token;
            unsigned int runLen = tok & 0xff;
            if ((tok & 0x8000) != 0) {
                max_rows = max_rows - 1;
                if (max_rows == -1) {
                suspend:
                    if (rows != 0) {
                        rle_state[0] = rows;
                        rle_state[3] = (int)(intptr_t)token;
                        rle_state[4] = (int)(intptr_t)pixel;
                        rle_state[2] = (int)(intptr_t)rowBase;
                        return 0;
                    }
                    return 1;
                }
                dst = rowBase + stride;
                rowBase = dst;
            }
            dst = dst - ((tok & 0x7f00) >> 8);
            if (((runLen - 1) & 0xffff) != 0xffff) {
                for (n = (int)runLen - 9; -1 < n; n = n - 8) {
                    dst[-7] = pixel[7];
                    dst[-6] = pixel[6];
                    dst[-5] = pixel[5];
                    dst[-4] = pixel[4];
                    dst[-3] = pixel[3];
                    dst[-2] = pixel[2];
                    dst[-1] = pixel[1];
                    dst[0] = pixel[0];
                    pixel = pixel + 8;
                    dst = dst - 8;
                }
                for (n = n + 4; -1 < n; n = n - 4) {
                    dst[-3] = pixel[3];
                    dst[-2] = pixel[2];
                    dst[-1] = pixel[1];
                    dst[0] = pixel[0];
                    pixel = pixel + 4;
                    dst = dst - 4;
                }
                n = n + 4;
                do {
                    *dst = *pixel;
                    pixel = pixel + 1;
                    n = n - 1;
                    dst = dst - 1;
                } while (-1 < n);
            }
            token = token + 1;
        }
    }
    return 1;
}
