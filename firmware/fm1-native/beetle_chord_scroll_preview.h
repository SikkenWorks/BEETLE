/* BEETLE V1.4.33 CHD audition policy. SPDX-License-Identifier: GPL-3.0-only */
#ifndef BEETLE_CHORD_SCROLL_PREVIEW_H
#define BEETLE_CHORD_SCROLL_PREVIEW_H
#include "track_core.h"
/* Produces exactly the pitches that the tracker playback engine uses.
 * Return 0 for drum lane, playing transport, EMPTY/STP root or invalid shape. */
static inline uint8_t beetle_chord_scroll_pitches(const struct track_step *s,
    const struct track_harmony *h,uint8_t lane,uint8_t playing,uint8_t out[5]){
 if(!s || !h || !out || lane>=TRACK_SYNTH_CHANNELS || playing ||
    s->note>=TRACK_NOTE_STOP || s->chord>=TRACK_CHORD_COUNT)return 0u;
 return track_chord_keylocked(s->note,s->chord,h,out);
}
#endif
