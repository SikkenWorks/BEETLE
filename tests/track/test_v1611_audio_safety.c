/* GPL-3.0-only: V1.6.11 hardware-master freshness and fail-closed output. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../../firmware/fm1-native/beetle_audio_safety.h"
#include "../../firmware/fm1-native/track_output_gain.h"
static int32_t abs32(int32_t v){return v<0?-v:v;}
int main(void){
 assert(beetle_master_from_adc(0)==0);
 assert(beetle_master_from_adc(1023)==32767);
 assert(beetle_master_from_adc(-1)==0);
 assert(beetle_master_from_adc(1024)==0); /* pre-fix ADC would cause full-scale */
 assert(beetle_master_from_adc(65536)==0);
 /* A corrupted single high 10-bit ADC sample cannot instantly force full gain. */
 assert(beetle_master_publish_step(0u,1023)==2048u);
 assert(beetle_master_publish_step(20000u,1023)==22048u);
 assert(beetle_master_publish_step(20000u,0)==0u);
 assert(beetle_master_publish_step(65535u,1023)==0u);
 assert(beetle_master_safe_gain(32767,1000,1000)==32767);
 assert(beetle_master_safe_gain(32767,1080,1000)==32767);
 assert(beetle_master_safe_gain(32767,1081,1000)==0); /* main loop stalls */
 assert(beetle_master_safe_gain(65535,1000,1000)==0);
 assert(beetle_master_safe_gain(10000,5,0xfffffffeu)==10000); /* ms wrap */
 assert(beetle_master_safe_gain(10000,100,0xfffffffeu)==0);
 /* Once stale, old high Q16 gain must be clamped immediately DOWN. */
 int32_t smooth=32767<<16;
 int16_t dry=32767;
 int16_t safe=track_master_apply(dry,beetle_master_safe_gain(32767,1081,1000),&smooth);
 assert(safe==0 && smooth==0);
 /* Validate physical control invariants for all 1024 ADC codes and
  * widely ranging signals, including clipping, mute, and master sweeps. */
 for(int adc=0;adc<=1023;adc++){
  uint16_t knob=beetle_master_from_adc(adc);
  for(int i=-32768;i<=32767;i+=73){
   int32_t last=32767<<16;
   int16_t result=track_master_apply((int16_t)i,knob,&last);
   int32_t upper=(int32_t)(((int64_t)abs32(i)*knob)/32767);
   assert(abs32(result)<=upper);
   assert(last>=0 && last<=(int32_t)((uint32_t)knob<<16));
  }
 }
 puts("PASS V1.6.11: physical MASTER bounded, invalid/stale ADC mutes, rollover and all ADC 10-bit states");
 return 0;
}
