/* SPDX-License-Identifier: GPL-3.0-only — host regression for startup LEDs. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../../firmware/fm1-native/beetle_led_ripple.h"
int main(void){
 for(uint8_t id=0;id<41;id++){
  int bright=0,complete=0;
  for(uint32_t t=0;t<BEETLE_RIPPLE_DURATION_MS;t+=8u){
   uint8_t level=beetle_ripple_level(t,id);
   assert(level<=2u);
   if(level==2u)bright=1;
  }
  assert(bright);
  for(uint32_t t=BEETLE_RIPPLE_DURATION_MS;t<30000u;t+=43u){
   if(beetle_ripple_level(t,id)==0u)complete++;
   else {fprintf(stderr,"LED %u still glowing at %u ms\n",id,t);return 1;}
  }
  assert(complete>10);
 }
 assert(beetle_ripple_level(0u,27u)==2u);
 assert(beetle_ripple_level(0u,28u)==2u);
 assert(beetle_ripple_level(0u,0u)==0u);
 puts("PASS: all 41 LEDs have a bright wavefront and are permanently off after one-shot ripple");
 return 0;
}
