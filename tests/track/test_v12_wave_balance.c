/* GPL-3.0-only, real native oscillators across the 12 types and 5 VA shapes. */
#include "track_chip.h"
#include "track_mixer.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
static struct track_chip_engine synth;
static int16_t pcm[2048];
static double rms_for(unsigned type,int shape,int *peak){
 struct track_mixer mix;track_mixer_init(&mix);mix.ch[0].volume=100;
 track_chip_init(&synth);
 synth.osc_type[0]=(uint8_t)type;
 synth.env_attack[0]=0;synth.env_decay[0]=1;synth.env_sustain[0]=100;
 if(shape>=0){synth.patches[0].wave_a=shape;synth.patches[0].wave_b=shape;}
 assert(track_chip_note(&synth,0,0,60,110));
 uint64_t sq=0;int max=0;
 for(unsigned b=0;b<24;b++){
  track_chip_render(&synth,&mix,pcm,1024);
  if(b<4)continue;
  for(int j=0;j<1024;j++){
   int val=pcm[2*j];if(val>max)max=val;if(-val>max)max=-val;
   sq+=(uint64_t)((int64_t)val*val);
  }
 }
 *peak=max;
 /* compare power without libm, return mean square */
 return (double)sq/(20.0*1024.0);
}
int main(void){
 double min=1e15,max=0;
 for(unsigned t=0;t<12;t++){
  int peak;double power=rms_for(t,-1,&peak);
  fprintf(stderr,"type %u rms %.1f peak %d\n",t,__builtin_sqrt(power),peak);
  if(power<min)min=power;if(power>max)max=power;
  assert(peak<30000); /* clean single voice retains 16-bit headroom */
 }
 fprintf(stderr,"ratio %f\n",max/min);
 assert(max/min<1.30); /* < 1.14 in RMS, native reference is ~1.07 */
 for(unsigned t=2;t<=3;t++){
  double lo=1e15,hi=0;
  for(int w=0;w<5;w++){
   int peak;double power=rms_for(t,w,&peak);
   if(power<lo)lo=power;if(power>hi)hi=power;
   assert(peak<30000);
  }
  assert(hi/lo<1.30);
 }
 printf("PASS V1.2: 12 native types + 5 VA shapes × 2 engines, RMS spread %.3f, headroom preserved\n",max/min);
}
