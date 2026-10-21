/* SPDX-License-Identifier: GPL-3.0-only
 * V0.82 Punch-In: seven tempo-locked gate subdivisions.
 * Only the light control path uses division; sample code uses precomputed half.
 */
#ifndef BEETLE_PUNCH_TIMING_H
#define BEETLE_PUNCH_TIMING_H
#include <stdint.h>
static inline uint8_t beetle_stutter_index(uint8_t value){
 if(value>=100u)return 6u;
 return (uint8_t)(((uint32_t)value*6u+50u)/100u);
}
static inline uint8_t beetle_stutter_value(uint8_t index){
 static const uint8_t v[7]={0u,17u,33u,50u,67u,83u,100u};
 return v[index<7u?index:6u];
}
/* A full gate cycle is one quarter-note at x1. /8 ... x8, with
 * equal-length ON/OFF halves. 30..300 BPM; safe even outside bounds. */
static inline uint32_t beetle_stutter_half_frames(uint16_t bpm,uint8_t value){
 if(bpm<30u)bpm=30u;
 if(bpm>300u)bpm=300u;
 const uint32_t beat=2646000u/(uint32_t)bpm;
 const uint8_t index=beetle_stutter_index(value);
 uint32_t full=index<3u ? beat*(uint32_t)(1u<<(3u-index)) :
                 (index==3u?beat:beat/(uint32_t)(1u<<(index-3u)));
 return full/2u ? full/2u : 1u;
}
/* Reverse at 50 = 1x (tempo-preserving). Values under 50 slow
 * down toward 0.5x, above 50 speed up toward 2x. Q8 advances on
 * a 22050-Hz capture ring; 128 is the neutral per-44100-Hz step. */
static inline uint32_t beetle_reverse_rate_q8(uint8_t value){
 if(value<=50u)return 64u+(uint32_t)value*64u/50u;
 return 128u+((uint32_t)(value-50u)*128u)/50u;
}
#endif
