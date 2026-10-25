/* SPDX-License-Identifier: GPL-3.0-only
 * BEETLE V1.4.22 DPCM4/FM1 runtime. ISR never reads flash.
 * CAVEAT: real FM1 audio/deadline validation still mandatory.
 */
#ifndef BEETLE_BD4_LIVE_H
#define BEETLE_BD4_LIVE_H
#include <stdint.h>
#include "beetle_storage_v2_reader.h"
void beetle_bd4_live_init(void);
void beetle_bd4_live_bind(void *workspace,uint32_t bytes);
void beetle_bd4_live_poll(beetle_v2_read read,void *ctx,void (*lock)(void),void (*unlock)(void));
void beetle_bd4_live_invalidate(void);
int beetle_bd4_live_available(uint8_t slot);
/* Three-character label, read from verified V2 sample footer during main-loop refresh. */
const char *beetle_bd4_live_label(uint8_t slot);
/* Called from existing drum IRQ; no flash access, no decoding. */
void beetle_bd4_live_request(uint8_t voice,uint8_t slot,uint32_t serial);
/* Returns 1 for an output frame, 0 if still waiting, -1 when finished. */
int beetle_bd4_live_next(uint8_t voice,uint32_t serial,int16_t *sample);
void beetle_bd4_live_stop(uint8_t voice);
void beetle_bd4_live_choke_all(void);
/* IRQ-safe, RAM-only: are triggered global samples still preparing their
 * initial decoded window? Used ONLY to hold the very first transport DMA
 * frames until NOTE1/NOTE2/NOTE3 samples are ready together. */
uint8_t beetle_bd4_live_start_pending(void);
uint32_t beetle_bd4_live_underruns(void);
#endif
