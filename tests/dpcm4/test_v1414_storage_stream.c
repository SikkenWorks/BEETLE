/* SPDX-License-Identifier: GPL-3.0-only
 * Host-only ABI connecting an immutable flash-bank reader to BEETLE BD4S
 * prefetch. Audio render never invokes reader callback. */
#include "beetle_bd4_prefetch.h"
#include <stdint.h>
#include <string.h>
static struct beetle_bd4_stream integration_stream;
void beetle_test_reset(void) { beetle_bd4_stream_reset(&integration_stream); }
int beetle_test_prepare(unsigned voice, uint8_t slot, uint32_t bytes) {
    return beetle_bd4_stream_prepare(&integration_stream,voice,slot,bytes);
}
int beetle_test_poll(beetle_bd4_blob_reader cb,void *ctx,unsigned windows) {
    return beetle_bd4_prefetch_poll(&integration_stream,cb,ctx,windows);
}
int beetle_test_start(unsigned voice) {
    return beetle_bd4_stream_start(&integration_stream,voice);
}
void beetle_test_render(unsigned voice,int16_t *out,uint32_t n) {
    beetle_bd4_stream_render(&integration_stream,voice,out,n);
}
uint32_t beetle_test_underruns(unsigned voice) {
    return voice<BEETLE_BD4_STREAM_VOICES ? integration_stream.voices[voice].underruns : UINT32_MAX;
}
