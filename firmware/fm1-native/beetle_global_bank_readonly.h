/* SPDX-License-Identifier: GPL-3.0-only
 * BEETLE global one-shot library v1: read-only index and validation contract.
 * STORAGE-AGNOSTIC LAB MODULE. Not enabled in FM1 firmware.
 * No flash writes, no dynamic allocation, no audio ISR integration.
 */
#ifndef BEETLE_GLOBAL_BANK_READONLY_H
#define BEETLE_GLOBAL_BANK_READONLY_H
#include <stdint.h>
#define BEETLE_GBANK_SLOTS 13u
#define BEETLE_GBANK_MAX_FILE 33554432u
/* Read exactly n bytes at offset, return 0 for success; n never exceeds 512. */
typedef int (*beetle_gbank_read_fn)(void *ctx,uint32_t offset,uint8_t *dst,uint32_t n);
struct beetle_gbank_slot {
 uint32_t wav_offset,wav_bytes,audio_bytes,frames,sample_rate;
 uint16_t channels,bits_per_sample;
 uint8_t format; /* 1 = PCM, 3 = IEEE float; 0 = empty */
 char name[13],filename[13];
};
struct beetle_gbank_index {
 struct beetle_gbank_slot slots[BEETLE_GBANK_SLOTS];
 uint32_t bytes_total,bytes_audio;
};
/* Nonzero error: -1 malformed/corrupt, -2 read failure, -3 unsupported format. */
int beetle_gbank_open_readonly(beetle_gbank_read_fn read,void *ctx,uint32_t total,
                              struct beetle_gbank_index *out);
#endif
