/* SPDX-License-Identifier: GPL-3.0-only
 * SikkenWorks BEETLE BD4S experimental 4-bit adaptive sample decoder.
 * Original design: NOT imported from IMA ADPCM or other firmware.
 * HOST/LAB ONLY. Do not read SPI NOR or call storage from audio IRQ.
 */
#ifndef BEETLE_DPCM4_H
#define BEETLE_DPCM4_H
#include <stdint.h>
/* Fixed independent blocks, 1..256 mono PCM frames; little endian on disk.
 * Header: first PCM sample (s16), initial quantizer step (u16), frames (u16).
 * Encoded data: signed four-bit deltas, low nibble first. No padding audio.
 * Maximum encoded block is 134 bytes. */
#define BEETLE_BD4_BLOCK_FRAMES 256u
#define BEETLE_BD4_BLOCK_BYTES 134u
/* A pure bounded decode on ALREADY PREFETCHED RAM. Never accesses flash.
 * Returns frame count or negative error. Scratch output supplied by caller.
 */
int beetle_bd4_decode_block(const uint8_t *src, uint32_t src_bytes,
                            int16_t *dst, uint32_t dst_frames);
#endif
