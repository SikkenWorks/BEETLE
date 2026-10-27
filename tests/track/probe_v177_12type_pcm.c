/* Host-only regression: 12 oscillators x 4 scenes; never used in FM1 runtime. */
#include "track_chip.h"
#include "track_mixer.h"
#include <stdint.h>
#include <stdio.h>
static struct track_chip_engine ch;
static int16_t pcm[1024];
int main(void){
 for(unsigned type=0;type<12;type++)for(unsigned scene=0;scene<4;scene++){
  track_chip_init(&ch);
  struct track_mixer mix;track_mixer_init(&mix);mix.ch[0].volume=(uint8_t)(50+scene*8);
  ch.osc_type[0]=(uint8_t)type;
  ch.patches[0].wave_a=(uint8_t)(scene%5);ch.patches[0].wave_b=(uint8_t)((scene+2)%5);
  ch.patches[0].drive=(uint8_t)(scene*8);ch.patches[0].sub=(uint8_t)(scene*5);
  ch.patches[0].morph=(uint8_t)(scene*14);
  ch.cutoff[0]=(uint8_t)(85+scene*15);
  ch.env_attack[0]=(uint8_t)(scene*5);ch.env_decay[0]=19;ch.env_sustain[0]=72;ch.env_release[0]=25;
  unsigned note=50+scene*9;unsigned velocity=84+scene*6;
  if(!track_chip_note(&ch,0,0,(uint8_t)note,(uint8_t)velocity))return 2;
  unsigned peak=0;uint64_t h=14695981039346656037ull;
  for(unsigned b=0;b<16;b++){
   if(b==11)track_chip_note_off(&ch,0,(uint8_t)note);
   track_chip_render(&ch,&mix,pcm,512);
   for(unsigned i=0;i<1024;i++){
    int32_t amp=pcm[i]<0?-(int32_t)pcm[i]:(int32_t)pcm[i];if((unsigned)amp>peak)peak=(unsigned)amp;
    uint16_t u=(uint16_t)pcm[i];h=(h^(uint8_t)u)*1099511628211ull;h=(h^(uint8_t)(u>>8))*1099511628211ull;
   }
  }
  printf("%u,%u,%016llx,%u\n",type,scene,(unsigned long long)h,peak);
 }
 return 0;
}
