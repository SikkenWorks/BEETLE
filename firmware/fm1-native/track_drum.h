/* BEETLE V0.31: native 44.1kHz, 16-bit procedurally synthesized percussion.
 * Independent of legacy 8-bit WAV assets. GPL-3.0-only. */
#ifndef TRACK_DRUM_H
#define TRACK_DRUM_H
#include <stdint.h>
#include "beetle_oneshots.h"
#define TRACK_DRUM_COUNT 14u /* factory kit: unchanged */
#define TRACK_DRUM_NOTE_COUNT BEETLE_ONESHOT_LAST
#define TRACK_DRUM_POLY 6u
#define TRACK_DRUM_CHANNEL 5u
struct track_drum_voice {
 uint32_t sequence,phase,phase_step,noise_state,age,length;
 int32_t noise_lp,noise_lp2;
 uint32_t env_q24;
 uint16_t oneshot_offset;
 uint16_t gain_q8,env_loss_q16;
 uint8_t id,active,release_frames,component;
 int8_t pitch_semitones;
};
struct track_drum_timbre {uint8_t attack,decay,sustain,release,eq_low,eq_mid,eq_high,gain;int8_t pitch;};
struct track_drum_engine {
 struct track_drum_voice voices[TRACK_DRUM_POLY];
 uint32_t rng,trigger_serial;
 uint8_t sn_last,hh_last,step_gain;
 struct track_drum_timbre components[TRACK_DRUM_COUNT];
 const struct beetle_oneshot_bank *oneshots; /* project workspace; never allocated in ISR */
};
struct track_mixer;
void track_drum_init(struct track_drum_engine *d);
void track_drum_bind_oneshots(struct track_drum_engine *d,const struct beetle_oneshot_bank *b);
void track_drum_timbre_default(struct track_drum_timbre *p);
void track_drum_component_default(struct track_drum_timbre *p,uint8_t id);
int track_drum_trigger(struct track_drum_engine *d,uint8_t index);
int track_drum_trigger_pair(struct track_drum_engine *d,uint8_t first,uint8_t second);
int track_drum_trigger_pair_percent(struct track_drum_engine *d,uint8_t first,uint8_t second,uint8_t percent);
int track_drum_trigger_pair_tuned(struct track_drum_engine *d,uint8_t first,uint8_t second,int8_t semitones);
/* Update an already-triggered voice after per-NOTE pitch FX (ISR-safe). */
void track_drum_retune_voice(struct track_drum_engine *d,struct track_drum_voice *v);
void track_drum_stop(struct track_drum_engine *d);
/* Fast, bounded 48-frame fade for -- Song Chain transitions. */
void track_drum_choke_all(struct track_drum_engine *d);
void track_drum_render(struct track_drum_engine *d,const struct track_mixer *mix,int16_t *stereo,uint32_t frames);
const char *track_drum_name(uint8_t index);
#endif
