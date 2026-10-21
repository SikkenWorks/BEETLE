/* SPDX-License-Identifier: GPL-3.0-only
 * DMG/RETRO PULSE regression, independent of NES PULSE and all other types. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "track_chip.h"
#include "track_mixer.h"
static uint64_t audition(uint8_t type,uint8_t duty,uint32_t *step,int32_t *last) {
 struct track_chip_engine s;
 struct track_mixer m;
 int16_t frames[256*2];
 track_chip_init(&s);track_mixer_init(&m);
 s.osc_type[0]=type;s.duty[0]=duty;
 s.env_attack[0]=0;s.env_decay[0]=0;s.env_sustain[0]=100;
 assert(track_chip_note(&s,0,0,69,110));
 *step=s.voices[s.last_voice_index].step;
 uint64_t energy=0;
 for(unsigned b=0;b<60;b++){
  memset(frames,0,sizeof(frames));track_chip_render(&s,&m,frames,256);
  for(unsigned i=0;i<256;i++){
   int32_t x=frames[i*2];energy+=(uint32_t)(x<0?-x:x);
   if(b==59u&&i==255u)*last=x;
  }
 }
 assert(track_chip_health(&s));
 assert(energy>100000u);
 return energy;
}
int main(void){
 uint32_t dmgStep,nesStep;int32_t out;
 uint64_t dmg=audition(TRACK_CHIP_RETRO_PULSE,2u,&dmgStep,&out);
 uint64_t nes=audition(TRACK_CHIP_NES_PULSE,2u,&nesStep,&out);
 /* DMG 11-bit frequency divider quantizes the usual equal-tempered pitch. */
 assert(dmgStep!=nesStep);
 /* Distinct amplitude/DAC/highpass path: not identical to NES APU pulse. */
 assert(dmg!=nes);
 for(uint8_t duty=0;duty<4u;duty++){
  uint32_t step;uint64_t e=audition(TRACK_CHIP_RETRO_PULSE,duty,&step,&out);
  assert(e>100000u);assert(step==dmgStep);
 }
 printf("PASS V0.80 DMG pulse: quantized phase inc %u vs NES %u; amplitudes %llu/%llu\n",dmgStep,nesStep,(unsigned long long)dmg,(unsigned long long)nes);
 return 0;
}
