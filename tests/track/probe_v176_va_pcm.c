/* GPL-3.0-only: deterministic 44.1 kHz PCM fingerprint of native VA engines.
 * Executed against the original V1.6.10 and current production track_chip.c.
 */
#include "track_chip.h"
#include "track_mixer.h"
#include <stdio.h>
#include <stdint.h>
static struct track_chip_engine synth;
static int16_t pcm[512*2];
int main(void){
 for(unsigned type=2;type<=3;type++)for(unsigned shape=0;shape<5;shape++)
  for(unsigned scene=0;scene<4;scene++){
   track_chip_init(&synth);
   struct track_mixer mix;track_mixer_init(&mix);
   mix.ch[0].volume=85;
   synth.osc_type[0]=(uint8_t)type;
   synth.patches[0].wave_a=(uint8_t)shape;
   synth.patches[0].wave_b=(uint8_t)((shape+scene+1)%5);
   synth.patches[0].morph=(uint8_t)(scene*20);
   synth.patches[0].drive=(uint8_t)(scene*9);
   synth.patches[0].sub=(uint8_t)(scene*4);
   synth.patches[0].fine_b=(int8_t)(scene*13);
   synth.patches[0].detune=(uint8_t)(12+scene*9);
   synth.cutoff[0]=(uint8_t)(96-scene*12);
   synth.env_attack[0]=(uint8_t)(scene*4);
   synth.env_decay[0]=12;synth.env_sustain[0]=78;synth.env_release[0]=24;
   unsigned note=(unsigned)(48+scene*12),vel=(unsigned)(80+scene*5);
   if(!track_chip_note(&synth,0,0,(uint8_t)note,(uint8_t)vel))return 2;
   uint64_t hash=14695981039346656037ULL;
   unsigned peak=0;
   for(unsigned b=0;b<12;b++){
    if(b==8)track_chip_note_off(&synth,0,(uint8_t)note);
    track_chip_render(&synth,&mix,pcm,512);
    for(unsigned i=0;i<1024;i++){
     uint16_t u=(uint16_t)pcm[i];
     unsigned m=(pcm[i]<0)?(unsigned)(-(int32_t)pcm[i]):(unsigned)pcm[i];
     if(m>peak)peak=m;
     hash=(hash^(uint8_t)u)*1099511628211ULL;
     hash=(hash^(uint8_t)(u>>8))*1099511628211ULL;
    }
   }
   printf("%u,%u,%u,%016llx,%u\n",type,shape,scene,(unsigned long long)hash,peak);
  }
 return 0;
}
