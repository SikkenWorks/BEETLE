/* TRACK V0.14: bounded, realtime-safe retro and bandlimited VA synth.
 * No malloc, libm, logging, or flash operations in the audio callback.
 * These are original chip-STYLE generators, not cycle-accurate emulators.
 * SPDX-License-Identifier: GPL-3.0-only
 */
#ifndef TRACK_CHIP_H
#define TRACK_CHIP_H
#include <stdint.h>
#include "track_core.h"
#define TRACK_CHIP_VOICES 8u
#define TRACK_CHIP_RATE 44100u
#define TRACK_CHIP_RETRO_PULSE 0u
#define TRACK_CHIP_RETRO_WAVE 1u
#define TRACK_CHIP_VA_OSC 2u
#define TRACK_CHIP_VA_OSC2 3u
#define TRACK_CHIP_RESID_LITE 4u
#define TRACK_CHIP_SID_6581 4u
#define TRACK_CHIP_SID_8580 5u
#define TRACK_CHIP_SID_DIRTY 6u
#define TRACK_CHIP_SID_BASS 7u
#define TRACK_CHIP_SID_NOISE 8u
#define TRACK_CHIP_TYPES 9u
/* VA shapes: harmonic-limited saw, triangle, square, sine, variable-width pulse. */
#define TRACK_VA_SHAPES 5u
struct track_va_patch {
 uint8_t wave_a,wave_b;
 int8_t coarse,fine; /* semitones +/-24, cents +/-100 */
 uint8_t detune,mix,pulse_width,morph; /* A width; DETUNE/MIX/MORPH are shared mix controls */
 int8_t coarse_b,fine_b; /* oscillator B independent tuning */
 uint8_t pulse_width_b; /* oscillator B independent pulse width 5..95 */
 uint8_t drive,sub,noise,fm_depth; /* 0..100, timbral VA controls */
};
void track_va_patch_default(struct track_va_patch *p);
int track_va_patch_valid(const struct track_va_patch *p);

/* Legacy aliases for old test fixtures, not exposed in TRACK instrument UI. */
#define TRACK_CHIP_PULSE TRACK_CHIP_RETRO_PULSE
#define TRACK_CHIP_SID TRACK_CHIP_RETRO_WAVE
#define TRACK_CHIP_GBWAVE TRACK_CHIP_VA_OSC
#define TRACK_CHIP_NOISE TRACK_CHIP_VA_OSC2
struct track_chip_voice {
 uint32_t phase,phase2,step,step2,lfsr,noise_clock,birth;
 int32_t lowpass,last_level,sid_bandpass;
 uint16_t release_left;
 uint32_t envelope,attack_rate,decay_rate,release_rate,envelope_sustain;
 uint8_t env_stage; /* 0 A, 1 D, 2 S, 3 R */
 uint8_t active,channel,instrument,type,velocity,attack,note,released;
};
struct track_chip_engine {
 struct track_chip_voice voices[TRACK_CHIP_VOICES];
 uint8_t voice_count,last_voice_index,duty[TRACK_CHANNELS],cutoff[TRACK_CHANNELS],drive[TRACK_CHANNELS],osc_type[4];
 uint8_t env_attack[4],env_decay[4],env_sustain[4],env_release[4];
 struct track_va_patch patches[4]; /* per instrument, not per channel */
 uint8_t fx_type[TRACK_CHANNELS],fx_value[TRACK_CHANNELS];
 uint8_t step_gain[TRACK_CHANNELS]; /* persistent per-channel 0..100% volume automation */
 uint8_t mono; /* 1: held last note priority; 0: up to 8 voices */
 uint32_t alloc_serial,steal_count,restart_count,health_cookie;
 uint16_t phrase_div_q8; /* Q8 mix divisor, smoothed per sample */
 uint8_t phrase_max_voices; /* preserve headroom until entire phrase ends */
 uint32_t step_samples,cut_after[TRACK_CHANNELS];
 uint32_t fx_samples[TRACK_SYNTH_CHANNELS]; /* FX age survives empty tracker steps */
 uint16_t release_left;
 int32_t prev_left,prev_right,tail_left,tail_right;
 /* Bounded, zero-allocation punch history; 512 stereo samples = 2 KiB. */
 int16_t punch_ring[512][2];
 uint16_t punch_write,punch_start,punch_tape_rate;
 int16_t punch_decimate_l,punch_decimate_r;uint8_t punch_decimate_phase;
 uint32_t punch_position,punch_age;
 uint8_t punch_previous,punch_wet,punch_filter_wet;
 uint8_t punch_param[8]; /* 0..100 per FX, retained across key-up / page changes */
 int32_t punch_hp_l,punch_hp_r,punch_bp_l,punch_bp_r;
 int32_t punch_filter_l,punch_filter_r;
};
struct track_mixer;
void track_chip_init(struct track_chip_engine *s);
void track_chip_fx_reset(void);
void track_chip_begin_step(struct track_chip_engine *s);
/* Apply FX to already-held voices without re-triggering the ADSR. */
void track_chip_repitch_channel(struct track_chip_engine *s,uint8_t channel);
/* Returns 1 when voice accepted, 0 when pool is at capacity. */
int track_chip_note(struct track_chip_engine *s,uint8_t channel,uint8_t instrument,uint8_t note,uint8_t velocity);
void track_chip_silence(struct track_chip_engine *s);
void track_chip_note_off(struct track_chip_engine *s,uint8_t channel,uint8_t note);
/* Called with audio IRQ disabled, recalculates active voices without re-triggering. */
void track_chip_refresh_instrument(struct track_chip_engine *s,uint8_t instrument);
void track_chip_release_channel(struct track_chip_engine *s,uint8_t channel);
/* A new note or STP ends only this lane, with a bounded click-safe fade. */
void track_chip_stop_channel(struct track_chip_engine *s,uint8_t channel);
void track_chip_set_mono(struct track_chip_engine *s,uint8_t mono);
/* Nonzero means sane. A damaged DSP state is reset without rebooting the FM1. */
int track_chip_health(const struct track_chip_engine *s);
void track_chip_recover(struct track_chip_engine *s);
void track_chip_render(struct track_chip_engine *s,const struct track_mixer *mix,int16_t *stereo,uint32_t frames);
/* Live global punch processing AFTER drums; octave controls only chip notes. */
void track_chip_process_punch(struct track_chip_engine *s,uint8_t mask,int16_t *stereo,uint32_t frames);
#endif
