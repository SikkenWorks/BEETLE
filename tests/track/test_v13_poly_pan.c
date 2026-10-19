/* SPDX-License-Identifier: GPL-3.0-only. Native PCM voice+stereo regression. */
#include "track_chip.h"
#include "track_drum.h"
#include "track_mixer.h"
#include "beetle_stereo_pan.h"
#include "beetle_voice_headroom.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
static struct track_chip_engine synth;
static struct track_drum_engine drum;
static int16_t pcm[1024*2];
static double power(unsigned count,int pan,int *peak,int *separate){
 struct track_mixer m;track_mixer_init(&m);m.ch[0].volume=100;m.ch[0].pan=pan;
 track_chip_init(&synth);synth.osc_type[0]=TRACK_CHIP_VA_OSC;
 synth.env_attack[0]=0u;synth.env_decay[0]=1u;synth.env_sustain[0]=100u;
 assert(track_chip_note(&synth,0,0,60,100));
 if(count>1)assert(track_chip_note(&synth,0,0,64,100));
 if(count>2)assert(track_chip_note(&synth,0,0,67,100));
 double square=0.0;*peak=0;*separate=0;
 for(unsigned j=0;j<20;j++){
  track_chip_render(&synth,&m,pcm,1024u);
  if(j<4)continue;
  for(unsigned i=0;i<1024;i++){
   const int32_t l=pcm[i*2],r=pcm[i*2+1];
   if(l>*peak)*peak=l;if(-l>*peak)*peak=-l;
   if(r>*peak)*peak=r;if(-r>*peak)*peak=-r;
   if(l!=r)(*separate)++;
   square+=(double)l*l+(double)r*r;
  }
 }
 return square/(2.0*16*1024);
}
int main(void){
 uint16_t l,r;beetle_stereo_pan_gain(0,&l,&r);assert(l==256u&&r==256u);
 beetle_stereo_pan_gain(-100,&l,&r);assert(l>=360u&&r==0u);
 beetle_stereo_pan_gain(100,&l,&r);assert(l==0u&&r>=360u);
 assert(beetle_voice_divisor(1u)==256u&&beetle_voice_divisor(8u)==768u);
 int pk,s=0;double single=power(1,0,&pk,&s);assert(s==0 && pk<31000);
 double chord=power(3,0,&pk,&s);assert(s==0 && pk<32767);
 printf("1 voice RMS %.2f, 3-note chord RMS %.2f\n",sqrt(single),sqrt(chord));
 assert(chord>single*0.60); /* chord no longer collapses behind solo note */
 power(1,-100,&pk,&s);assert(s>10000&&pk<32767);
 for(unsigned i=0;i<1024;i++)assert(pcm[i*2+1]==0);
 power(1,100,&pk,&s);assert(s>10000&&pk<32767);
 for(unsigned i=0;i<1024;i++)assert(pcm[i*2]==0);
 struct track_mixer m;track_mixer_init(&m);m.ch[TRACK_DRUM_CHANNEL].pan=-100;
 track_drum_init(&drum);assert(track_drum_trigger(&drum,0u));
 for(unsigned i=0;i<2048;i+=1024){
  for(int j=0;j<2048;j++)pcm[j]=0;
  track_drum_render(&drum,&m,pcm,1024);
  for(unsigned j=0;j<1024;j++)assert(pcm[j*2+1]==0);
 }
 printf("PASS V1.3: native mono/chord power and L/R mixer PAN for synth + drums\n");
}
