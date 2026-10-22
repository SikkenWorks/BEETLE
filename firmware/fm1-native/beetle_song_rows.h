/* SPDX-License-Identifier: GPL-3.0-only
 * BEETLE Song Chain row range clipboard. No allocation or flash I/O.
 * Independent from the tracker pattern/step clipboard.
 */
#ifndef BEETLE_SONG_ROWS_H
#define BEETLE_SONG_ROWS_H
#include "track_song.h"
struct beetle_song_rows_clip {
 struct track_song_entry rows[TRACK_SONG_ENTRIES];
 uint8_t count,valid;
};
static inline uint8_t beetle_song_rows_in_range(uint8_t row,uint8_t a,uint8_t b){
 return (uint8_t)(row>=(a<b?a:b)&&row<=(a>b?a:b));
}
static inline int beetle_song_rows_copy(struct beetle_song_rows_clip *clip,
 const struct track_song_bank *song,uint8_t first,uint8_t last){
 if(!clip||!song||first>=song->count||last>=song->count)return -1;
 uint8_t lo=first<last?first:last,hi=first>last?first:last;
 clip->valid=0u;clip->count=(uint8_t)(hi-lo+1u);
 for(uint8_t k=0u;k<clip->count;k++)clip->rows[k]=song->chain[(uint8_t)(lo+k)];
 clip->valid=1u;return clip->count;
}
static inline int beetle_song_rows_paste(const struct beetle_song_rows_clip *clip,
 struct track_song_bank *song,uint8_t at){
 if(!clip||!clip->valid||!clip->count||!song||at>=TRACK_SONG_ENTRIES ||
    (uint16_t)at+clip->count>TRACK_SONG_ENTRIES)return -1;
 /* Complete, atomic preflight: never partially paste a selection. */
 for(uint8_t k=0u;k<clip->count;k++)song->chain[(uint8_t)(at+k)]=clip->rows[k];
 if(song->count<(uint8_t)(at+clip->count))song->count=(uint8_t)(at+clip->count);
 return clip->count;
}
#endif
