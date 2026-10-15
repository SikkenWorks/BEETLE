/* BEETLE V0.36: deterministic button and clock helpers, no hardware dependencies.
 * FM1 keyboard F3..G5 semitone indices 0..26; six left-most black keys.
 * SPDX-License-Identifier: GPL-3.0-only */
#ifndef BEETLE_TRACK_UI_CONTROLS_H
#define BEETLE_TRACK_UI_CONTROLS_H
#include <stdint.h>
#define BEETLE_STORAGE_HOLD_MS 5000u
static inline int8_t beetle_black_mute_lane(uint8_t note_index){
 static const uint8_t black_keys[6]={1u,3u,5u,8u,10u,13u};
 for(uint8_t i=0;i<6u;i++)if(black_keys[i]==note_index)return (int8_t)i;
 return -1;
}
static inline uint8_t beetle_hold_finished(uint32_t now,uint32_t start){
 return (uint8_t)(start!=0u && (uint32_t)(now-start)>=BEETLE_STORAGE_HOLD_MS);
}
static inline uint8_t beetle_list_move(uint8_t current,int32_t steps){
 int32_t next=(int32_t)current+steps;
 if(next<0)next=0;if(next>11)next=11;return (uint8_t)next;
}
#endif
