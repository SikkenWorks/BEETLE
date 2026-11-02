/* SPDX-License-Identifier: GPL-3.0-only
 * Compare optimized BEETLE MASTER with the pre-V1.8.1 64-bit reference.
 * Exhaustive PCM and boundary gains, plus long changing-gain ramps.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <limits.h>
#include "track_output_gain.h"

static int16_t reference_master(int16_t sample,uint16_t knob,int32_t *smoothed_q16){
 if(!smoothed_q16)return 0;
 if(knob>32767u)knob=32767u;
 const int32_t limit=(int32_t)((uint32_t)knob<<16u);
 int32_t value=*smoothed_q16;
 if(value<0)value=0;
 if(value>limit)value=limit;
 else if(value<limit){
  int32_t step=(limit-value)/96;
  if(step<1)step=1;
  value+=step;
  if(value>limit)value=limit;
 }
 *smoothed_q16=value;
 return (int16_t)(((int64_t)sample*(value>>16u))/32767);
}
static int16_t original_right(int16_t sample,int32_t gain_q16){
 return (int16_t)(((int64_t)sample*(gain_q16>>16u))/32767);
}
static int16_t optimized_right(int16_t sample,int32_t gain_q16){
 return (int16_t)(((int32_t)sample*(gain_q16>>16u))/32767);
}
static uint32_t rand_state=0x92156ABCu;
static uint32_t next_rand(void){
 rand_state^=rand_state<<13;
 rand_state^=rand_state>>17;
 rand_state^=rand_state<<5;
 return rand_state;
}
int main(void){
 const uint16_t gains[]={0,1,96,512,16384,32766,32767,65535};
 uint64_t cases=0;
 for(unsigned g=0;g<sizeof(gains)/sizeof(gains[0]);g++){
  for(int32_t sample=INT16_MIN;sample<=INT16_MAX;sample++){
   /* Steady gain, plus one ramp step from silence and from high gain. */
   for(unsigned mode=0;mode<3;mode++){
    int32_t oldval=mode==0?(int32_t)((uint32_t)(gains[g]>32767?32767:gains[g])<<16u)
                       :mode==1?0:INT32_MAX;
    int32_t newval=oldval;
    int16_t a=reference_master((int16_t)sample,gains[g],&oldval);
    int16_t b=track_master_apply((int16_t)sample,gains[g],&newval);
    assert(a==b && oldval==newval);
    assert(original_right((int16_t)sample,newval)==optimized_right((int16_t)sample,newval));
    cases++;
   }
  }
 }
 /* Moving master during playback: prevent hidden state-dependent divergence. */
 int32_t oldval=0,newval=0;
 for(uint32_t i=0;i<1000000u;i++){
  int16_t sample=(int16_t)next_rand();
  uint16_t knob=(uint16_t)next_rand();
  int16_t a=reference_master(sample,knob,&oldval);
  int16_t b=track_master_apply(sample,knob,&newval);
  assert(a==b && oldval==newval);
  assert(original_right(sample,newval)==optimized_right(sample,newval));
  cases++;
 }
 assert(track_master_apply(123,32767u,0)==0);
 printf("PASS V1.8.1 MASTER exactness: %llu frames, signed 32-bit range safe, no 64-bit helper required\n",(unsigned long long)cases);
 return 0;
}
