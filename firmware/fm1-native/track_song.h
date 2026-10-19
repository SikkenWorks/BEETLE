/* TRACK V0.4: bounded native RAM pattern bank, song chain and recorded Punch-In.
 * Does not implement flash persistence. GPL-3.0-only.
 */
#ifndef TRACK_SONG_H
#define TRACK_SONG_H
#include <stdint.h>
#include "track_core.h"
#define TRACK_SONG_ENTRIES 64u
#define TRACK_SONG_FX_EVENTS 24u
struct track_song_event {uint8_t step:4,fx:3;uint8_t down:2;uint8_t param;}; /* 3 packed RAM bytes, down=2: ALGORITHM automation */
struct track_song_clip {
 struct track_song_event data[TRACK_SONG_FX_EVENTS];
 uint8_t count,overflow;
};
/* V19: legacy base pattern/repeats plus six independent lane references.
 * 255 means silence for a particular lane. */
#define TRACK_SONG_SILENT 255u
struct track_song_entry {uint8_t pattern,repeats,lane[TRACK_CHANNELS];};
struct track_song_bank {
 struct track_pattern patterns[TRACK_PATTERNS];
 struct track_song_clip clips[TRACK_PATTERNS];
 struct track_song_entry chain[TRACK_SONG_ENTRIES];
 uint8_t count,position,repeat_index,loop,enabled;
 uint8_t edit_row,edit_column,composite_active;
};
void track_song_init(struct track_song_bank *bank,const struct track_engine *engine);
/* Switches active pattern without touching persistent flash. On failure active pattern remains unchanged. */
int track_song_switch(struct track_song_bank *bank,struct track_engine *engine,uint8_t index);
void track_song_capture(struct track_song_bank *bank,const struct track_engine *engine);
void track_song_restore(struct track_song_bank *bank,struct track_engine *engine);
uint8_t track_song_punch_pattern(const struct track_song_bank *bank,const struct track_engine *engine);
void track_song_start(struct track_song_bank *bank,struct track_engine *engine);
/* Called only at wrap to step 00 in audio context. Returns 1 when a pattern changed. */
int track_song_on_wrap(struct track_song_bank *bank,struct track_engine *engine);
int track_song_insert(struct track_song_bank *bank,uint8_t after);
int track_song_delete(struct track_song_bank *bank,uint8_t at);
int track_song_change(struct track_song_bank *bank,uint8_t row,uint8_t col,int32_t delta);
int track_song_record(struct track_song_bank *bank,uint8_t pattern,uint8_t step,uint8_t fx,uint8_t pressed);
int track_song_record_value(struct track_song_bank *bank,uint8_t pattern,uint8_t step,uint8_t fx,uint8_t kind,uint8_t value);
uint8_t track_song_effect_parameter(const struct track_song_bank *bank,uint8_t pat,uint8_t step,uint8_t fx,uint8_t fallback);
/* Reconstruct active recorded effects at a step (applies on step boundaries). */
uint8_t track_song_effect_mask(const struct track_song_bank *bank,uint8_t pattern,uint8_t step);
#endif
