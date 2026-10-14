/* SPDX-License-Identifier: GPL-3.0-only
 * BEETLE V0.60: bounded, quantized manual pattern change.
 * Called at step transitions only. Never interferes with Song Mode.
 */
#ifndef BEETLE_PATTERN_QUEUE_H
#define BEETLE_PATTERN_QUEUE_H
#include <stdint.h>
#include "track_song.h"
static inline int beetle_pattern_queue_commit(struct track_song_bank *bank,
 struct track_engine *engine,volatile uint8_t *pending){
 if(!bank||!engine||!pending||bank->enabled||!engine->transport.playing||
    engine->transport.play_step!=0u||*pending>=TRACK_PATTERNS)return 0;
 uint8_t next=*pending;
 if(track_song_switch(bank,engine,next)!=0)return -1;
 *pending=255u;
 return 1;
}
#endif
