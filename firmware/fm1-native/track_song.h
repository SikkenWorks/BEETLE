/* TRACK V0.4: bounded native RAM pattern bank, song chain and recorded Punch-In.
 * Does not implement flash persistence. GPL-3.0-only.
 */
#ifndef TRACK_SONG_H
#define TRACK_SONG_H
#include <stdint.h>
#include "track_core.h"
#define TRACK_SONG_ENTRIES 64u
#define TRACK_SONG_FX_EVENTS 24u
struct track_song_event {uint8_t step:4,fx:4;uint8_t down:2;uint8_t param;}; /* 3 packed RAM bytes, down=2: ALGORITHM automation */
struct track_song_clip {
 struct track_song_event data[TRACK_SONG_FX_EVENTS];
 uint8_t count,overflow;
};
/* V19: legacy base pattern/repeats plus six independent lane references.
 * 255 means silence for a particular lane. */
#define TRACK_SONG_SILENT 255u
/* 128..175: automatically bound to pattern 0..47, visible only with content.
 * 0..47: explicitly selected pattern, even when empty (allows sustain).
 * 255: explicit STOP; never resumes on later pattern edits. */
#define TRACK_SONG_AUTO_BASE 128u
static inline uint8_t track_song_source(uint8_t lane){
 return (lane>=TRACK_SONG_AUTO_BASE && lane<TRACK_SONG_AUTO_BASE+TRACK_PATTERNS)?
        (uint8_t)(lane-TRACK_SONG_AUTO_BASE):lane;
}
static inline uint8_t track_song_valid_lane(uint8_t lane){
 return (uint8_t)(lane<TRACK_PATTERNS || lane==TRACK_SONG_SILENT ||
         (lane>=TRACK_SONG_AUTO_BASE&&lane<TRACK_SONG_AUTO_BASE+TRACK_PATTERNS));
}
struct track_song_entry {uint8_t pattern,repeats,lane[TRACK_CHANNELS];};
struct track_song_bank {
 struct track_pattern patterns[TRACK_PATTERNS];
 struct track_song_clip clips[TRACK_PATTERNS];
 struct track_song_entry chain[TRACK_SONG_ENTRIES];
 uint8_t count,position,repeat_index,loop,enabled;
 uint8_t edit_row,edit_column,composite_active;
 /* Transient playback-only range (not serialized). Selection is not a song edit. */
 uint8_t play_range_active,play_range_first,play_range_last;
 /* Shared project-wide sparse FX pool: each record belongs to a pattern. */
 struct track_extra_bank {
  uint8_t count;
  struct track_extra_fx {uint8_t pattern,id,effect,value;} data[TRACK_EXTRA_FX_CAP];
 } extras;
};
/* V22: FX2 and FX3 are independent. The top bit of effect marks FX3;
 * lower seven bits remain the real effect ID. Existing V21 projects are
 * upgraded while decoding. No extra RAM bytes are required. */
static inline uint8_t track_extra_slot(const struct track_extra_fx *f){return f?(uint8_t)((f->effect>>7u)&1u):0u;}
static inline uint8_t track_extra_effect(const struct track_extra_fx *f){return f?(uint8_t)(f->effect&127u):0u;}
static inline uint8_t track_extra_count(const struct track_song_bank *b,uint8_t pattern,uint8_t key){
 uint8_t n=0u;if(!b)return 0u;
 for(uint8_t i=0u;i<b->extras.count&&i<TRACK_EXTRA_FX_CAP;i++)
  if(b->extras.data[i].pattern==pattern&&b->extras.data[i].id==key)n++;
 return n;
}
static inline struct track_extra_fx *track_extra_get(struct track_song_bank *b,uint8_t pattern,uint8_t key,uint8_t slot){
 if(!b||slot>=2u)return 0;
 for(uint8_t i=0u;i<b->extras.count&&i<TRACK_EXTRA_FX_CAP;i++){
  struct track_extra_fx *f=&b->extras.data[i];
  if(f->pattern==pattern&&f->id==key&&track_extra_slot(f)==slot)return f;
 }
 return 0;
}
static inline const struct track_extra_fx *track_extra_find(const struct track_song_bank *b,uint8_t pattern,uint8_t key,uint8_t slot){
 return track_extra_get((struct track_song_bank*)b,pattern,key,slot);
}
static inline void track_extra_remove(struct track_song_bank *b,uint8_t pattern,uint8_t key,uint8_t slot){
 if(!b||slot>=2u)return;
 for(uint8_t i=0u;i<b->extras.count;i++){
  struct track_extra_fx *f=&b->extras.data[i];
  if(f->pattern==pattern&&f->id==key&&track_extra_slot(f)==slot){
   for(uint8_t j=i;j+1u<b->extras.count;j++)b->extras.data[j]=b->extras.data[j+1u];
   b->extras.count--;return;
  }
 }
}
static inline void track_extra_clear(struct track_song_bank *b,uint8_t pattern,uint8_t key){
 track_extra_remove(b,pattern,key,0u);track_extra_remove(b,pattern,key,1u);
}
static inline int track_extra_add(struct track_song_bank *b,uint8_t pattern,uint8_t key,uint8_t slot,uint8_t fx,uint8_t value){
 if(!b||pattern>=TRACK_PATTERNS||slot>=2u||fx==0u||fx>=TRACK_FX_COUNT||value>100u)return -2;
 if(track_extra_get(b,pattern,key,slot))return 0;
 if(b->extras.count>=TRACK_EXTRA_FX_CAP)return -1;
 b->extras.data[b->extras.count++]=(struct track_extra_fx){pattern,key,(uint8_t)(fx|(slot<<7u)),value};return 1;
}
/* FX slots are independent, so removing FX1 must not discard FX2/FX3. */
static inline void track_extra_prune(struct track_song_bank *b,uint8_t pattern,const struct track_pattern *p){
 (void)b;(void)pattern;(void)p;
}
void track_song_init(struct track_song_bank *bank,const struct track_engine *engine);
/* Switches active pattern without touching persistent flash. On failure active pattern remains unchanged. */
int track_song_switch(struct track_song_bank *bank,struct track_engine *engine,uint8_t index);
void track_song_capture(struct track_song_bank *bank,const struct track_engine *engine);
void track_song_restore(struct track_song_bank *bank,struct track_engine *engine);
uint8_t track_song_punch_pattern(const struct track_song_bank *bank,const struct track_engine *engine);
void track_song_start(struct track_song_bank *bank,struct track_engine *engine);
/* Inclusive song row loop: always loops this range regardless of global song LOOP. */
void track_song_start_range(struct track_song_bank *bank,struct track_engine *engine,
                            uint8_t first,uint8_t last);
/* Called only at wrap to step 00 in audio context. Returns 1 when a pattern changed. */
int track_song_on_wrap(struct track_song_bank *bank,struct track_engine *engine);
int track_song_insert(struct track_song_bank *bank,uint8_t after);
int track_song_delete(struct track_song_bank *bank,uint8_t at);
int track_song_change(struct track_song_bank *bank,uint8_t row,uint8_t col,int32_t delta);
/* Show auto-referenced lanes only once a source pattern contains actual events.
 * A deliberate 255 SILENT assignment stays silent, including after edits. */
uint8_t track_song_lane_visible(const struct track_song_bank *bank,uint8_t source,uint8_t channel);
uint8_t track_song_stop_mask(const struct track_song_bank *bank);
int track_song_record(struct track_song_bank *bank,uint8_t pattern,uint8_t step,uint8_t fx,uint8_t pressed);
int track_song_record_value(struct track_song_bank *bank,uint8_t pattern,uint8_t step,uint8_t fx,uint8_t kind,uint8_t value);
uint8_t track_song_effect_parameter(const struct track_song_bank *bank,uint8_t pat,uint8_t step,uint8_t fx,uint8_t fallback);
/* Reconstruct active recorded effects at a step (applies on step boundaries). */
void track_song_clear_punch_fx(struct track_song_bank *bank,uint8_t pattern,uint8_t fx);
uint16_t track_song_effect_mask(const struct track_song_bank *bank,uint8_t pattern,uint8_t step);
/* At first transport step/new Song row, never infer a key pressed in a previous loop. */
uint16_t track_song_effect_mask_boundary(const struct track_song_bank *bank,uint8_t pattern,uint8_t step,uint8_t allow_wrap);
#endif
