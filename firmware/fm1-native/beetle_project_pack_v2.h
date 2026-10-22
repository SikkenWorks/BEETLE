/* SPDX-License-Identifier: GPL-3.0-only
 * BEETLE's independently authored lossless project packer, version 1.
 * OFFLINE ONLY: not wired into native project load/save or DMA paths.
 * Caller owns 4 KiB hash scratch; never allocate in audio ISR.
 */
#ifndef BEETLE_PROJECT_PACK_V2_H
#define BEETLE_PROJECT_PACK_V2_H
#include <stdint.h>
#include <stddef.h>
#define BEETLE_PACK_V2_HEADER 16u
#define BEETLE_PACK_V2_WORK 2048u
#define BEETLE_PACK_V2_MAX_INPUT 12256u
#define BEETLE_PACK_V2_OK 0
#define BEETLE_PACK_V2_INVALID -1
#define BEETLE_PACK_V2_CAPACITY -2
#define BEETLE_PACK_V2_CHECKSUM -3
int beetle_pack_project_v2(const uint8_t *source, size_t size,
    uint8_t *out, size_t out_cap, size_t *written, uint16_t last_two[BEETLE_PACK_V2_WORK]);
int beetle_unpack_project_v2(const uint8_t *packed, size_t packed_len,
    uint8_t *out, size_t out_cap, size_t *written);
#endif
