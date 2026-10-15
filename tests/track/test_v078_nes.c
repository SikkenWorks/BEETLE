/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "beetle_nes_apu.h"
#include "track_chip.h"
#include "track_mixer.h"
static void check_audio(uint8_t type, long *sum, unsigned *transitions) {
 struct track_chip_engine chip; struct track_mixer mx;
 track_chip_init(&chip);track_mixer_init(&mx);
 chip.osc_type[0]=type;chip.env_attack[0]=1;chip.env_sustain[0]=95;
 assert(track_chip_note(&chip,0,0,60,100));
 int16_t buf[512*2];*sum=0;*transitions=0;
 int16_t prev=0;
 for(int loop=0;loop<30;loop++){
   memset(buf,0,sizeof(buf));track_chip_render(&chip,&mx,buf,512);
   for(int i=0;i<512;i++){
    int16_t v=buf[2*i];*sum+=v>=0?v:-v;
    if(i&&v!=prev)(*transitions)++;
    prev=v;
   }
 }
 assert(track_chip_health(&chip));
 assert(*sum>150000);assert(*transitions>50);
}
int main(void){
 unsigned cnt=0;
 for(uint8_t duty=0;duty<4;duty++){
  unsigned n=0;for(unsigned i=0;i<8;i++)if(beetle_nes_pulse((uint32_t)i<<29,duty)>0)n++;
  assert(n==(duty==0?1u:duty==1?2u:duty==2?4u:6u));
 }
 assert(beetle_nes_triangle(0)>beetle_nes_triangle(15u<<27));
 assert(beetle_nes_triangle(16u<<27)==beetle_nes_triangle(15u<<27));
 for(uint8_t note=36;note<100;note++){
  uint32_t inc=beetle_nes_noise_increment(note);assert(inc>0u);
  if(note>36)assert(inc>=beetle_nes_noise_increment(note-1));
 }
 uint32_t r=1;int saw_hi=0,saw_lo=0;
 for(int k=0;k<200;k++){
  int16_t x=beetle_nes_noise(&r,0);
  assert((r&0x8000u)==0u);
  saw_hi|=x>0;saw_lo|=x<0;
 }assert(saw_hi&&saw_lo);
 long sums[3];unsigned transitions[3];
 for(int i=0;i<3;i++)check_audio((uint8_t)(TRACK_CHIP_NES_PULSE+i),&sums[i],&transitions[i]);
 assert(sums[0]!=sums[1]&&sums[0]!=sums[2]);
 printf("PASS NES three types, duty/triangle/noise + render: energy %ld/%ld/%ld transitions %u/%u/%u\n",sums[0],sums[1],sums[2],transitions[0],transitions[1],transitions[2]);
 return 0;
}
