/* FM1 native oscillator level characterization, mono note C4, fixed settings. */
#include "track_chip.h"
#include "track_mixer.h"
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
static struct track_chip_engine eng;
static struct track_mixer mix;
static int16_t pcm[1024*2];
static void run(const char *name,int typ,int wav){
 track_chip_init(&eng);track_mixer_init(&mix);mix.ch[0].volume=100;
 eng.osc_type[0]=(uint8_t)typ;eng.env_attack[0]=0;eng.env_decay[0]=1;eng.env_sustain[0]=100;
 eng.cutoff[0]=96;eng.patches[0].wave_a=wav>=0?wav:0;eng.patches[0].wave_b=wav>=0?wav:0;
 eng.patches[0].mix=50;eng.patches[0].morph=0;eng.patches[0].drive=0;
 if(!track_chip_note(&eng,0,0,60,110)){fprintf(stderr,"note rejected\n");return;}
 long double sq=0;int peak=0;unsigned long n=0;
 for(int blk=0;blk<24;blk++){
  track_chip_render(&eng,&mix,pcm,1024);
  if(blk<4)continue;
  for(int i=0;i<1024;i++){
   int v=pcm[2*i];if(v<0 && -v>peak)peak=-v;if(v>peak)peak=v;
   sq+=(long double)v*v;n++;
  }
 }
 printf("%-19s type%2d wave%2d rms %8.1Lf peak%6d rms_dbFS %7.2f\n",name,typ,wav,sqrtl(sq/n),peak,20*log10l(sqrtl(sq/n)/32768));
}
int main(void){static const char *name[]={"DMG PULSE","DMG WAVE","VA OSC","VA OSC2","SID 6581","SID 8580","SID DIRTY","SID BASS","SID NOISE","NES PULSE","NES TRIANGLE","NES NOISE"};for(int i=0;i<12;i++)run(name[i],i,-1);for(int i=0;i<5;i++){run("VA OSC WAVE",2,i);run("VA OSC2 WAVE",3,i);}return 0;}
