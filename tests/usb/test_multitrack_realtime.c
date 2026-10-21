/* SPDX-License-Identifier: GPL-3.0-only
 * BEETLE pre-reSID USB multitrack native math + common-clock regression.
 * Host test; NOT physical USB / DMA hardware certification.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "beetle_usb_multitrack.h"
static uint32_t rng=0x1251a930u;
static uint32_t rnd(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
int main(void){
 uint64_t tested=0;
 /* Check exactly against former int64 multiply for any PCM16 sign and
  * fractional-position pattern, including extremes/zero/full scale. */
 const int16_t corner[]={-32768,-32767,-32700,-16384,-1,0,1,16384,32700,32767};
 const uint16_t phases[]={0,1,255,256,257,32767,32768,32769,65280,65535};
 for(unsigned a=0;a<sizeof(corner)/sizeof(corner[0]);a++)
  for(unsigned b=0;b<sizeof(corner)/sizeof(corner[0]);b++)
   for(unsigned f=0;f<sizeof(phases)/sizeof(phases[0]);f++){
    int32_t diff=(int32_t)corner[b]-(int32_t)corner[a];
    int16_t reference=(int16_t)((int32_t)corner[a]+(int32_t)(((int64_t)diff*phases[f])>>16));
    assert(beetle_uac_mt_lerp(corner[a],corner[b],phases[f])==reference);tested++;
   }
 for(uint32_t i=0;i<10000000u;i++){
  int16_t a=(int16_t)rnd(),b=(int16_t)rnd();uint16_t frac=(uint16_t)rnd();
  int32_t diff=(int32_t)b-(int32_t)a;
  int16_t ref=(int16_t)((int32_t)a+(int32_t)(((int64_t)diff*frac)>>16));
  assert(beetle_uac_mt_lerp(a,b,frac)==ref);tested++;
 }
 /* Six simultaneous lanes stay aligned: one fractional position per USB
  * frame, no per-lane phase or sample-clock state is allowed. */
 for(uint32_t t=0;t<150000u;t++){
  uint16_t frac=(uint16_t)rnd();
  int16_t first=(int16_t)(rnd()&0x7fffu),second=(int16_t)(rnd()&0x7fffu);
  int16_t frame[6];
  for(uint8_t ch=0;ch<6;ch++)frame[ch]=beetle_uac_mt_lerp(first,second,frac);
  for(uint8_t ch=1;ch<6;ch++)assert(frame[ch]==frame[0]);
 }
 uint32_t step=BEETLE_UAC_MT_ONE;int32_t fill=(int32_t)104*256;
 uint32_t highest=step,lowest=step,changes=0;
 /* +/-20 sample observation jitter (USB frame/render boundary phase),
  * with constant +17 frames/s producer drift and sporadic 4-frame shocks. */
 for(uint32_t i=0;i<50000;i++){
  int32_t perturb=(int32_t)(rnd()%41)-20;
  int32_t f=104+perturb+(int32_t)(i/5000);
  if((i%613)==0)f+=4;
  fill=beetle_uac_mt_filter_fill(fill,(uint32_t)f);
  uint32_t newer=beetle_uac_mt_next_step(step,fill,104);
  assert(newer>=BEETLE_UAC_MT_ONE-64 && newer<=BEETLE_UAC_MT_ONE+64);
  if(newer!=step)changes++;
  step=newer;
  if(step>highest)highest=step;
  if(step<lowest)lowest=step;
 }
 printf("PASS USB six-stem exact interpolation comparisons=%llu; shared phase=150000; bounded SRC=[%u,%u], updates=%u\n",(unsigned long long)tested,lowest,highest,changes);
 return 0;
}
