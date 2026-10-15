/* V0.84 host DSP regression: drum-only stutter, ring modulation, vibrato. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "track_chip.h"
#include "track_mixer.h"

#define FRAMES 8192
static int16_t samples[FRAMES*2];
static void fill_drum_only(void){for(int i=0;i<FRAMES*2;i++)samples[i]=(int16_t)(i%2?11000:9000);}
int main(void){
 struct track_chip_engine s;track_chip_init(&s);
 assert(s.punch_param[1]==20u); /* new Ring Mod default */
 track_chip_init(&s);fill_drum_only();
 s.punch_param[4]=100u;s.punch_bpm=120u;
 assert(s.voice_count==0u); /* drum-only PCM, zero synth voices */
 track_chip_process_punch(&s,1u<<4,samples,FRAMES);
 long long on=0,off=0;int on_count=0,off_count=0;
 /* x8 @120 BPM: gate half=1378 samples. First 256 are wet crossfade. */
 for(int i=256;i<FRAMES;i++){
  int ph=i%2756;
  if(ph>=400 && ph<1000){on+=(samples[i*2]>0?samples[i*2]:-samples[i*2]);on_count++;}
  if(ph>=1600&&ph<2400){off+=(samples[i*2]>0?samples[i*2]:-samples[i*2]);off_count++;}
 }
 assert(on_count&&off_count);
 assert(on*off_count>off*on_count*4); /* audible ON/OFF on drum-only bus */
 track_chip_init(&s);fill_drum_only();
 s.punch_param[1]=0;track_chip_process_punch(&s,1u<<1,samples,FRAMES);
 for(int i=0;i<FRAMES*2;i++)assert(samples[i]==(int16_t)(i%2?11000:9000));
 track_chip_init(&s);fill_drum_only();
 s.punch_param[1]=20u;track_chip_process_punch(&s,1u<<1,samples,FRAMES);
 int changed=0;for(int i=900;i<FRAMES;i++)if(samples[i*2]!=9000)changed++;
 assert(changed>FRAMES/2); /* true modulation at default intensity */
 track_chip_init(&s);struct track_mixer mix;track_mixer_init(&mix);
 s.osc_type[0]=TRACK_CHIP_NES_PULSE;s.fx_type[0]=6u;s.fx_value[0]=70u;
 assert(track_chip_note(&s,0,0,69,127));
 uint32_t prev=s.voices[0].phase,lo=0xffffffffu,hi=0;
 int16_t frame[2];
 for(int i=0;i<22050;i++){
  track_chip_render(&s,&mix,frame,1);
  uint32_t now=s.voices[0].phase,step=now-prev;prev=now;
  if(i>2000&&step<lo)lo=step;
  if(i>2000&&step>hi)hi=step;
 }
 assert(lo>0 && hi>lo && (hi-lo)>lo/12u); /* >8% modulation, easily heard */
 /* The NES NOISE clock must also use modulated step2, not its raw oscillator period. */
 track_chip_init(&s);s.osc_type[0]=TRACK_CHIP_NES_NOISE;s.fx_type[0]=6u;s.fx_value[0]=100u;
 assert(track_chip_note(&s,0,0,69,127));
 uint32_t first=0,highest=0,lowest=0xffffffffu;
 for(int i=0;i<20000;i++){
  uint32_t old=s.voices[0].noise_clock;
  track_chip_render(&s,&mix,frame,1);
  uint32_t d=s.voices[0].noise_clock-old;
  if(i>1000){if(d<lowest)lowest=d;if(d>highest)highest=d;}
  if(i==1000)first=d;
 }
 assert(first&&lowest<highest&&highest-lowest>lowest/15u);
 puts("PASS V0.84: drum-only STUTTER, 20% ringmod, 0% dry, audible vibrato");
 return 0;
}
