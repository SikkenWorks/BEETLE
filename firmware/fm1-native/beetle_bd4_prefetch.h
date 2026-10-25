/* SPDX-License-Identifier: GPL-3.0-only
 * BEETLE BD4S streaming: active with Storage V2 native FM1 firmware.
 * Prefetch uses main-loop flash I/O; render uses RAM only.
 * Producer/consumer handoff must remain serialized by the caller.
 */
#ifndef BEETLE_BD4_PREFETCH_H
#define BEETLE_BD4_PREFETCH_H
#include <stdint.h>
#include "beetle_dpcm4.h"
#define BEETLE_BD4_STREAM_VOICES 6u
#define BEETLE_BD4_STREAM_SLOTS 13u
#define BEETLE_BD4_WINDOW_FRAMES 64u
#define BEETLE_BD4_WINDOW_COUNT 2u
#define BEETLE_BD4_SAMPLE_RATE 22050u
#define BEETLE_BD4_OUTPUT_RATE 44100u
/* Exact read of bytes from a separately verified sample blob. Return 1 on success.
 * This callback MUST be invoked only from beetle_bd4_prefetch_poll(). */
typedef int (*beetle_bd4_blob_reader)(void *context, uint8_t slot, uint32_t offset,
                                       uint8_t *destination, uint16_t size);
/* Pinned variant: the reader MUST verify voice+generation against an acquired
 * immutable flash record lease. Never resolve a currently mutable slot here.
 * Lease is obtained/released by MAIN CONTEXT, outside the audio callback. */
typedef int (*beetle_bd4_pinned_reader)(void *context, uint8_t voice, uint8_t slot,
                                        uint32_t generation, uint32_t offset,
                                        uint8_t *destination, uint16_t size);
struct beetle_bd4_window {
 uint32_t first_frame;
 uint16_t count;
 volatile uint8_t valid;
 int16_t pcm[BEETLE_BD4_WINDOW_FRAMES];
};
struct beetle_bd4_stream_voice {
 struct beetle_bd4_window windows[BEETLE_BD4_WINDOW_COUNT];
 uint8_t encoded[BEETLE_BD4_BLOCK_BYTES];
 uint32_t byte_length, frame_count, source_offset, decoded_frames, output_frames;
 uint32_t generation;
 uint32_t reads, underruns, invalid_blocks, skipped_frames;
 int32_t predictor,step,last_output;
 /* Cache the two 22.05 kHz source samples for the intervening 44.1 kHz
  * interpolation frame. Per-voice, RAM-only; no extra PCM windows/flash. */
 int16_t interp_a,interp_b;
 uint8_t interp_valid;
 uint16_t block_frames,block_cursor;
 uint8_t slot, configured, playing, verified_header, encoded_length, next_write;
 uint8_t pinned;
 uint8_t fade_in, in_gap, choke_frames;
};
struct beetle_bd4_stream {
 struct beetle_bd4_stream_voice voices[BEETLE_BD4_STREAM_VOICES];
 uint32_t background_reads, invalid_reads;
};
/* Initialisation and preparation are MAIN-CONTEXT operations, not audio IRQ.
 * Caller must prove blob integrity and storage ownership before preparing.
 * The sample's BD4S 16-byte header is checked again by the prefetch routine.
 */
void beetle_bd4_stream_reset(struct beetle_bd4_stream *stream);
int beetle_bd4_stream_prepare(struct beetle_bd4_stream *stream, unsigned voice,
                              uint8_t slot, uint32_t compressed_bytes);
/* Prefer this API for any mutable global bank: a voice retains exactly the
 * sample generation it was prepared with, even after slot replacement. */
int beetle_bd4_stream_prepare_pinned(struct beetle_bd4_stream *stream, unsigned voice,
                                     uint8_t slot, uint32_t compressed_bytes,
                                     uint32_t generation);
/* Poll a bounded number of 64-frame decode windows from background context.
 * Returns windows filled, or negative on invalid arguments. Never writes flash.
 */
int beetle_bd4_prefetch_poll(struct beetle_bd4_stream *stream,
                             beetle_bd4_blob_reader reader, void *context,
                             unsigned window_budget);
/* Separate generation-checked API; rejects non-pinned configured voices. */
int beetle_bd4_prefetch_poll_pinned(struct beetle_bd4_stream *stream,
                                    beetle_bd4_pinned_reader reader, void *context,
                                    unsigned window_budget);
/* Start only after at least two PCM windows are resident; otherwise refuse
 * without altering musical time. Actual sequencer must schedule lookahead.
 */
int beetle_bd4_stream_start(struct beetle_bd4_stream *stream,unsigned voice);
/* Scheduler-context finalisation, only after audio render has relinquished the
 * voice. Explicitly disarm BEFORE releasing the associated Storage V2 lease. */
void beetle_bd4_stream_disarm(struct beetle_bd4_stream *stream,unsigned voice);
/* Song Chain drum-lane -- is STP: fade any active user-sample voice in
 * 48 output frames. A deliberately selected empty pattern does not choke.
 * These functions operate on RAM-only stream state; no device writes.
 */
void beetle_bd4_stream_choke_voice(struct beetle_bd4_stream *stream,unsigned voice);
void beetle_bd4_stream_choke_all(struct beetle_bd4_stream *stream);
/* Realtime stage: reads PCM windows ONLY. No I/O, decoding, or allocation.
 * The supplied output is replaced, not mixed into BEETLE's native bus.
 * Main-loop producer must establish safe cross-context handoff.
 */
/* RAM-only availability probe. The active BD4S voice must not advance its
 * source cursor until BOTH samples needed for interpolation are resident;
 * first-window readiness alone cannot prevent an early step-1 truncation. */
int beetle_bd4_stream_ready_now(const struct beetle_bd4_stream *stream,unsigned voice);
void beetle_bd4_stream_render(struct beetle_bd4_stream *stream,unsigned voice,
                              int16_t *mono_output,uint32_t output_frames);
#endif
