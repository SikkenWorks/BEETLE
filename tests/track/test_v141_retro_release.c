/* Native host PCM regression: Retro Pulse held note and release; no FM1 cycle claim. */
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "track_chip.h"
#include "track_mixer.h"
static struct track_chip_engine chip;
static struct track_mixer mix;
static int16_t pcm[256];
static uint64_t heldhash=1469598103934665603ull,releasehash=1469598103934665603ull;
int main(void){
 track_chip_init(&chip);track_mixer_init(&mix);
 chip.osc_type[0]=TRACK_CHIP_RETRO_PULSE;
 chip.env_attack[0]=1;chip.env_decay[0]=2;chip.env_sustain[0]=100;chip.env_release[0]=16;
 chip.duty[0]=2;
 assert(track_chip_note(&chip,0,0,69,120));
 uint32_t nonzero=0;unsigned long long release_energy=0;
 for(unsigned block=0;block<160+100;block++){
  if(block==160)track_chip_note_off(&chip,0,69);
  memset(pcm,0,sizeof pcm);track_chip_render(&chip,&mix,pcm,128);
  for(unsigned i=0;i<256;i++){
   int32_t x=pcm[i];if(x)nonzero++;
   if(block<160){heldhash^=(uint16_t)x;heldhash*=1099511628211ull;}
   else{releasehash^=(uint16_t)x;releasehash*=1099511628211ull;release_energy+=(int64_t)x*x;}
  }
 }
 assert(nonzero>20000&&release_energy>100000000ull);
 printf("RETRO PULSE heldhash=%016llx releasehash=%016llx release_energy=%llu nonzero=%u\n",
 (unsigned long long)heldhash,(unsigned long long)releasehash,release_energy,nonzero);
 return 0;
}
