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
#define TRACK_EXTRA_FX_CAP 48u /* project-wide sparse FX2/FX3 capacity */
#define TRACK_INSTRUMENT_SLOTS 8u
#define TRACK_FX_DRUM_PITCH 11u
#define TRACK_NOTE_STOP 254u
#define TRACK_NOTE_EMPTY 255u
#define TRACK_FX_VOLUME 10u
#define TRACK_CHORD_COUNT 12u
#define TRACK_PUNCH_COUNT 16u
/* Sparse pattern backing store: an inactive pattern may be loaded on demand. */
/* A 5-byte packed step: 8 NOTE + 3 INST + 4 CHORD + 4 FX + 7 VALUE
 * + 8 dynamics bits. Keeps the 48-pattern bank the SAME RAM size.
 * V1.3.1 uses 6 bytes per step to address eight synth patches.
 * dyn=0 has legacy velocity 94 and legato gate. Nibbles are independent.
 * Fields never address-taken. Explicit wire serialization avoids ABI issues. */
struct __attribute__((packed)) track_step {
 uint8_t note;
 unsigned int instrument:3;
 unsigned int chord:4;
 unsigned int effect:4;
 unsigned int value:7;
 unsigned int dynamics:8;
 unsigned int groove:7; /* 3-bit micro-delay, 2-bit ratchet, 2-bit probability */
};
/* 7 bit groove 0=original timing, one hit, 100% probability.
 * Micro 0..7 = delayed by micro/64 of a step, sample-exact. All offsets
 * are non-negative to avoid pretriggering the NEXT pattern/song position. */
#define TRACK_GROOVE_MICRO(S) ((uint8_t)((S)->groove&7u))
#define TRACK_GROOVE_RATCHET(S) ((uint8_t)(1u+(((S)->groove>>3u)&3u)))
#define TRACK_GROOVE_PROB(S) ((uint8_t)(100u-25u*(((S)->groove>>5u)&3u)))
static inline void track_groove_set_micro(struct track_step *s,uint8_t v){s->groove=(s->groove&~7u)|(v&7u);}
static inline void track_groove_set_ratchet(struct track_step *s,uint8_t v){s->groove=(s->groove&~24u)|(((v-1u)&3u)<<3u);}
static inline void track_groove_set_probability(struct track_step *s,uint8_t v){s->groove=(s->groove&~96u)|((v&3u)<<5u);}
#define TRACK_DYN_VEL(D) (((D)>>4)&15u)
#define TRACK_DYN_GATE(D) ((D)&15u)
static inline uint8_t track_step_velocity(const struct track_step *s){
 uint8_t v=(uint8_t)TRACK_DYN_VEL(s->dynamics);return v?(uint8_t)(v*8u):94u;
}
static inline uint8_t track_step_gate(const struct track_step *s){return (uint8_t)TRACK_DYN_GATE(s->dynamics);}
static inline void track_step_set_vel(struct track_step *s,uint8_t v){s->dynamics=(uint8_t)(((v&15u)<<4)|(s->dynamics&15u));}
static inline void track_step_set_gate(struct track_step *s,uint8_t v){s->dynamics=(uint8_t)((s->dynamics&240u)|(v&15u));}
_Static_assert(sizeof(struct track_step)==6u,"BEETLE eight-patch step must stay 6 bytes");
/* NOTE2 has an independent FX/value pair; the 5-byte primary step stays ABI-stable. */
struct track_note2_fx {uint8_t effect,value;};
/* Global sparse FX pool is attached to song bank, not the 48 patterns.
 * Pool lives once per project rather than multiplying every undo snapshot. */
struct track_pattern {struct track_step ch[TRACK_CHANNELS][TRACK_STEPS];uint8_t length;
 struct track_note2_fx note2_fx[TRACK_STEPS];
 uint8_t note3[TRACK_STEPS]; /* CH6 independent third drum hit, 255=EMPTY 254=STP */};
static inline uint8_t track_extra_key(uint8_t lane,uint8_t row,uint8_t note2){
 return (uint8_t)(lane*TRACK_STEPS+row+(note2?128u:0u));
}
struct track_harmony {uint8_t root,mode,keylock;};
struct track_transport {uint16_t bpm;uint8_t playing,play_step,edit_step,edit_column,edit_channel,stop_step;uint32_t tick_remainder;};
struct track_engine {struct track_pattern active;struct track_harmony harmony;struct track_transport transport;uint8_t project_slot,pattern_slot,record_armed,dirty;uint16_t punch_mask;};
/* MIDI 0..127 notes, 254 STP and 255 ---; no sentinels ever reach DSP pitch arrays. */
uint8_t track_note_scroll(uint8_t value,int32_t detents,uint8_t drum);
/* LOCK scrolls only through the selected scale; OFF/GUIDE preserve chromatic editing. */
uint8_t track_note_scroll_keylocked(uint8_t value,int32_t detents,uint8_t drum,const struct track_harmony *h);
void track_init(struct track_engine *e);
int track_enter_note(struct track_engine *e,uint8_t note);
/* Drum NOTE2: 4-bit chord holds empty/STOP and pad 1..14. The normally
 * 0..3 drum KIT field has a reserved bit 2 (0x4): when set, NOTE2 chord
 * 1..13 addresses user samples U01..U13. No new project bytes required.
 * The lower KIT bits and NOTE1 are untouched by NOTE2 selection. */
static inline uint8_t track_note2_id(const struct track_step *s){
 if(!s||!s->chord||s->chord==15u)return TRACK_NOTE_EMPTY;
 if(s->instrument&4u)return s->chord<=13u?(uint8_t)(13u+s->chord):TRACK_NOTE_EMPTY;
 return (uint8_t)(s->chord-1u);
}
static inline void track_note2_set(struct track_step *s,uint8_t id){
 if(!s)return;
 if(id==TRACK_NOTE_STOP){s->chord=15u;s->instrument&=3u;}
 else if(id==TRACK_NOTE_EMPTY){s->chord=0u;s->instrument&=3u;}
 else if(id>=14u&&id<=26u){s->chord=(uint8_t)(id-13u);s->instrument|=4u;}
 else if(id<14u){s->chord=(uint8_t)(id+1u);s->instrument&=3u;}
}
/* Entry does not alter NOTE1, cursor, drum KIT (low 2 bits) or step FX. */
int track_enter_drum_note2(struct track_engine *e,uint8_t drum_note);
int track_enter_drum_note3(struct track_engine *e,uint8_t drum_note);
int track_set_note(struct track_engine *e,uint8_t channel,uint8_t step,uint8_t note);
int track_clear_step(struct track_engine *e,uint8_t channel,uint8_t step);
int track_clear_step_range(struct track_engine *e,uint8_t channel,uint8_t first,uint8_t last);
int track_set_instrument_range(struct track_engine *e,uint8_t channel,uint8_t first,uint8_t last,uint8_t instrument);
/* Clear only the currently active pattern lane, all 16 rows, including drum NOTE2 FX. */
int track_clear_lane(struct track_engine *e,uint8_t channel);
/* True if the current pattern contains any musical event in this lane.
 * Ignore the default instrument and step value: those exist on empty lanes. */
uint8_t track_lane_has_data(const struct track_pattern *pattern,uint8_t channel);
int track_transpose_note_range(struct track_engine *e,uint8_t channel,uint8_t first,uint8_t last,int8_t semitones);
int track_adjust_octave(struct track_engine *e,int8_t semitones);
void track_rotate_select(struct track_engine *e,int8_t direction);
void track_set_channel(struct track_engine *e,uint8_t channel);
void track_set_bpm(struct track_engine *e,uint16_t bpm);
int track_set_pattern_length(struct track_engine *e,uint8_t length);
void track_play(struct track_engine *e);
void track_stop(struct track_engine *e);
int track_tick(struct track_engine *e); /* 1 on next step, 0 if stopped */
uint8_t track_quantize(uint8_t note,const struct track_harmony *h);
uint8_t track_chord(uint8_t root,uint8_t chord,uint8_t result[5]);
/* Under LOCK, root and every generated chord tone remain within the scale. */
uint8_t track_chord_keylocked(uint8_t root,uint8_t chord,const struct track_harmony *h,uint8_t result[5]);
void track_punch(struct track_engine *e,uint8_t effect,uint8_t held);
int track_is_lit(uint8_t button); /* FN indices 2..13; ARP=10 remains off */
uint32_t track_home_action(uint32_t held_ms,uint8_t released); /* 1 HOME short, 2 GUIDE long */
/* Pure endian-defined codec: fixed 324-byte pattern, no flash access. */
#define TRACK_PATTERN_V3_BYTES (4u+TRACK_CHANNELS*TRACK_STEPS*7u)
#define TRACK_PATTERN_V4_BYTES (TRACK_PATTERN_V3_BYTES+TRACK_STEPS*2u)
#define TRACK_PATTERN_BYTES (TRACK_PATTERN_V4_BYTES+TRACK_STEPS)
#define TRACK_PATTERN_V2_BYTES (4u+TRACK_CHANNELS*TRACK_STEPS*6u)
#define TRACK_PATTERN_LEGACY_BYTES (4u+TRACK_CHANNELS*TRACK_STEPS*5u)
int track_pattern_encode(const struct track_pattern *pattern,uint8_t *dst,uint32_t cap);
int track_pattern_decode(struct track_pattern *pattern,const uint8_t *src,uint32_t len);
#endif
