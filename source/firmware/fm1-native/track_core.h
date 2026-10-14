/* TRACK V0.1 – portable FM1 tracker model.
 * SPDX-License-Identifier: GPL-3.0-only
 * NOTE: hardware integration must use the original ETCH V1.28 Felucca HAL.
 * No flash writes, OTA protocol or audio interrupts in this model.
 */
#ifndef TRACK_CORE_H
#define TRACK_CORE_H
#include <stdint.h>
#define TRACK_CHANNELS 6u
#define TRACK_SYNTH_CHANNELS 5u
#define TRACK_PATTERNS 48u
#define TRACK_PROJECTS 12u
#define TRACK_STEPS 16u
#define TRACK_FX_COUNT 12u
#define TRACK_FX_DRUM_PITCH 11u
#define TRACK_NOTE_STOP 254u
#define TRACK_NOTE_EMPTY 255u
#define TRACK_FX_VOLUME 10u
#define TRACK_CHORD_COUNT 12u
#define TRACK_PUNCH_COUNT 8u
/* Sparse pattern backing store: an inactive pattern may be loaded on demand. */
struct track_step {uint8_t note;uint8_t instrument;uint8_t chord;uint8_t effect;uint8_t value;};
struct track_pattern {struct track_step ch[TRACK_CHANNELS][TRACK_STEPS];uint8_t length;};
struct track_harmony {uint8_t root,mode,keylock;};
struct track_transport {uint16_t bpm;uint8_t playing,play_step,edit_step,edit_column,edit_channel,stop_step;uint32_t tick_remainder;};
struct track_engine {struct track_pattern active;struct track_harmony harmony;struct track_transport transport;uint8_t project_slot,pattern_slot,record_armed,punch_mask,dirty;};
/* MIDI 0..127 notes, 254 STP and 255 ---; no sentinels ever reach DSP pitch arrays. */
uint8_t track_note_scroll(uint8_t value,int32_t detents,uint8_t drum);
void track_init(struct track_engine *e);
int track_enter_note(struct track_engine *e,uint8_t note);
int track_set_note(struct track_engine *e,uint8_t channel,uint8_t step,uint8_t note);
int track_clear_step(struct track_engine *e,uint8_t channel,uint8_t step);
int track_adjust_octave(struct track_engine *e,int8_t semitones);
void track_rotate_select(struct track_engine *e,int8_t direction);
void track_set_channel(struct track_engine *e,uint8_t channel);
void track_set_bpm(struct track_engine *e,uint16_t bpm);
void track_play(struct track_engine *e);
void track_stop(struct track_engine *e);
int track_tick(struct track_engine *e); /* 1 on next step, 0 if stopped */
uint8_t track_quantize(uint8_t note,const struct track_harmony *h);
uint8_t track_chord(uint8_t root,uint8_t chord,uint8_t result[5]);
void track_punch(struct track_engine *e,uint8_t effect,uint8_t held);
int track_is_lit(uint8_t button); /* FN indices 2..13; ARP=10 remains off */
uint32_t track_home_action(uint32_t held_ms,uint8_t released); /* 1 HOME short, 2 GUIDE long */
/* Pure endian-defined codec: fixed 324-byte pattern, no flash access. */
#define TRACK_PATTERN_BYTES (4u+TRACK_CHANNELS*TRACK_STEPS*5u)
int track_pattern_encode(const struct track_pattern *pattern,uint8_t *dst,uint32_t cap);
int track_pattern_decode(struct track_pattern *pattern,const uint8_t *src,uint32_t len);
#endif
