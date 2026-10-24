/* SPDX-License-Identifier: GPL-3.0-only
 * Regression: V1.4.30 activated VA's default 12-cent detune on SID B.
 * The resulting slow amplitude beat was mistaken for a built-in wah.
 * Compare normalized RMS windows after the envelope settles, and preserve
 * independently tuned oscillator B when explicitly requested. */
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include "../../firmware/fm1-native/track_chip.h"
#include "../../firmware/fm1-native/track_mixer.h"
#include "../../firmware/fm1-native/beetle_sid_lite.h"
static struct track_chip_engine engine;
static struct track_mixer mixer;
static int16_t audio[256*2];
static void measure(uint8_t chip,uint8_t detune,uint64_t *mn,uint64_t *mx){
 track_chip_init(&engine);track_mixer_init(&mixer);
 engine.osc_type[0]=chip;engine.cutoff[0]=96u;
 engine.patches[0].wave_a=chip==TRACK_CHIP_SID_BASS?1u:0u;
 engine.patches[0].wave_b=1u;engine.patches[0].mix=50u;
 engine.patches[0].drive=0u;engine.patches[0].morph=0u;
 engine.patches[0].detune=detune;
 engine.env_attack[0]=1u;engine.env_decay[0]=5u;engine.env_sustain[0]=100u;
 assert(track_chip_note(&engine,0u,0u,60u,110u));
 uint64_t sum=0,lo=UINT64_MAX,hi=0;
 unsigned n=0,window=0;
 while(n<44100u*4u){
  uint32_t cnt=256u;
  if(cnt>44100u*4u-n)cnt=44100u*4u-n;
  track_chip_render(&engine,&mixer,audio,cnt);
  for(uint32_t i=0;i<cnt;i++,n++){
   int64_t s=audio[2u*i];sum+=(uint64_t)(s*s);
   if(n%4410u==4409u){
    if(++window>=5u){if(sum<lo)lo=sum;if(sum>hi)hi=sum;}
    sum=0;
   }
  }
 }
 *mn=lo;*mx=hi;
}
int main(void){
 struct beetle_sid_prepared normal=beetle_sid_prepare(4,0,1,50,50,50,96,0,0,0,0,0);
 assert(normal.filter_coefficient==37);
 assert(normal.damping==204);
 for(int c=0;c<=255;c++){
  struct beetle_sid_prepared p=beetle_sid_prepare(4,0,1,50,50,50,(uint8_t)c,99,0,0,0,0);
  int32_t expect=9+c*c*31/10000;if(expect>124)expect=124;
  assert(p.filter_coefficient==expect && p.damping>=74);
 }
 for(uint8_t chip=TRACK_CHIP_SID_6581;chip<=TRACK_CHIP_SID_BASS;chip++){
  if(chip==TRACK_CHIP_SID_8580)continue;
  uint64_t mn,mx;measure(chip,12,&mn,&mx);
  printf("SID %u default sustain RMS power min=%llu max=%llu ratio=%.3f\n",chip,
      (unsigned long long)mn,(unsigned long long)mx,(double)mx/(double)mn);
  assert(mx*100u<mn*116u); /* no significant periodic wah on neutral B */
 }
 uint64_t mn,mx;measure(TRACK_CHIP_SID_6581,35,&mn,&mx);
 printf("SID explicit B detune=35: RMS power ratio=%.3f\n",(double)mx/(double)mn);
 assert(mx*100u>mn*120u); /* independent B still responds to user detune */
 struct beetle_sid_prepared c5=beetle_sid_prepare(5,0,1,50,50,50,96,0,0,0,0,0);
 struct beetle_sid_prepared c8=beetle_sid_prepare(8,0,1,50,50,50,96,0,0,0,0,0);
 assert(c5.filter_coefficient==c8.filter_coefficient && c5.damping==c8.damping);
 puts("PASS SID regression: legacy 6581 stable; custom B remains independent; 8580/noise mapping unchanged");
 return 0;
}
