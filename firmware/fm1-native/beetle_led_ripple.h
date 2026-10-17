/* SPDX-License-Identifier: GPL-3.0-only
 * BEETLE RC1.0.3 – centre-outwards drop/ripple over all 41 physical LEDs.
 * Pure visual map, called from UI/boot task, never from the audio IRQ.
 */
#ifndef BEETLE_LED_RIPPLE_H
#define BEETLE_LED_RIPPLE_H
#include <stdint.h>
#define BEETLE_RIPPLE_DURATION_MS 1200u
static const uint8_t beetle_ripple_keys_x[27]={0,1,2,3,4,5,6,8,9,10,11,12,14,15,16,17,18,19,20,22,23,24,25,26,28,29,30};
/* Returns 0=off, 1=soft trailing glow, 2=bright wavefront. */
static uint8_t beetle_ripple_level(uint32_t elapsed_ms,uint8_t id){
 if(id>=41u)return 0u;
 /* One shot. Never repeat a partially completed startup animation. */
 if(elapsed_ms>=BEETLE_RIPPLE_DURATION_MS)return 0u;
 uint32_t phase=elapsed_ms;
 uint8_t distance=0u;
 if(id>=14u){
  uint8_t x=beetle_ripple_keys_x[id-14u];
  distance=x<15u?(uint8_t)(15u-x):x>16u?(uint8_t)(x-16u):0u;
 }else if(id<2u){distance=17u;} /* octave buttons at the two ends */
 else{
  uint8_t b=(uint8_t)(id-2u),column=(uint8_t)(b%4u),row=(uint8_t)(b/4u);
  uint8_t x=(uint8_t)(column*10u);
  distance=(uint8_t)((x<15u?15u-x:x-15u)+10u+row*4u);
 }
 if(phase<72u)return distance==0u?2u:0u; /* two centre keys flash first */
 if(phase>=1120u)return 0u;
 uint8_t radius=(uint8_t)((phase-72u)/24u);
 uint8_t gap=(uint8_t)(radius>distance?radius-distance:distance-radius);
 if(gap<=1u)return 2u;
 if(radius>distance&&gap<=5u)return 1u;
 return 0u;
}
#endif
