/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH V1.23 Organic Engine: bounded deterministic per-voice micro-variation.
 * Four 0..100 controls. Zeroed project settings are a true, bit-exact bypass.
 */
#pragma once
#include <stdint.h>
struct etch_organic_settings {
 uint8_t drift, rate, phase, variance;
};
static inline uint8_t etch_organic_valid(const struct etch_organic_settings *o){
 return o && o->drift<=100u && o->rate<=100u && o->phase<=100u && o->variance<=100u;
}
static inline uint32_t etch_organic_hash(uint32_t x){
 x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;x^=x>>16;return x;
}
/* +/-12 cents maximum = approximately +/-454 Q16 pitch units.
 * STATIC variance adds up to +/-5 cents. Drifts are independent by voice. */
static inline int32_t etch_organic_target(uint32_t seed,uint8_t depth){
 uint32_t span=(uint32_t)depth*454u/100u;
 return (int32_t)((uint64_t)etch_organic_hash(seed)*(2u*span+1u)>>32)-(int32_t)span;
}
static inline int32_t etch_organic_static(uint32_t seed,uint8_t depth){
 uint32_t span=(uint32_t)depth*190u/100u;
 return (int32_t)((uint64_t)etch_organic_hash(seed)*(2u*span+1u)>>32)-(int32_t)span;
}
static inline uint32_t etch_organic_initial_phase(uint32_t seed,uint8_t depth){
 return (uint32_t)(((uint64_t)etch_organic_hash(seed)*depth)/100u);
}
static inline uint32_t etch_organic_interval(uint8_t rate){
 return 40000u-(uint32_t)rate*360u; /* 0.9-9.0 Hz on 44.1kHz audio */
}
static inline uint32_t etch_organic_pitch_inc(uint32_t inc,int32_t delta_q16){
 int64_t result=(int64_t)inc+(((int64_t)inc*delta_q16)>>16);
 return (uint32_t)(result<1?1:result>0xffffffffLL?0xffffffffLL:result);
}
