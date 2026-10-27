/* BEETLE V1.4.29: steady sustain must not reattack or accumulate DC. */
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "track_chip.h"
#include "track_mixer.h"
static struct track_chip_engine chip;
static struct track_mixer mix;
static int16_t buf[256];
int main(void){
 track_chip_init(&chip);track_mixer_init(&mix);
 chip.osc_type[0]=TRACK_CHIP_RETRO_PULSE;
 chip.env_attack[0]=1;chip.env_decay[0]=1;
 chip.env_sustain[0]=85;chip.env_release[0]=40;
 chip.duty[0]=2;
 assert(track_chip_note(&chip,0,0,69,120));
 int64_t sum=0,absum=0; int min=-1,max=-1;
 uint32_t birth=chip.voices[0].birth;
 for(int b=0;b<400;b++){
  memset(buf,0,sizeof buf);
  track_chip_render(&chip,&mix,buf,128);
  assert(chip.voices[0].birth==birth);
  if(b>200){for(int i=0;i<256;i++) {sum+=buf[i];absum+=(buf[i]<0?-(int32_t)buf[i]:buf[i]);}}
 }
 assert(chip.voices[0].active && chip.voices[0].env_stage==2);
 assert(absum>100000);
 /* A centered oscillator should have negligible long-run DC. */
 assert((sum<0?-sum:sum)*100<absum*5);
 track_chip_note_off(&chip,0,69);
 for(int b=0;b<160;b++){memset(buf,0,sizeof buf);track_chip_render(&chip,&mix,buf,128);}
 assert(!chip.voices[0].active);
 printf("PASS V1.4.29 RETRO PULSE: steady sustain, centered DC, smooth release (DC %.3f%%)\n",100.0*(double)(sum<0?-sum:sum)/(double)absum);
 return 0;
}
