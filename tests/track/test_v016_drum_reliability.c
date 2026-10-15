/* TRACK V0.16: rhythm and HHC stress tests on native mixer/PCM. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "track_drum.h"
#include "track_mixer.h"
static struct track_drum_engine e;
static struct track_mixer m;
static int16_t stereo[4410*2];
static unsigned count_live_hats(void){unsigned n=0;for(unsigned i=0;i<TRACK_DRUM_POLY;i++)if(e.voices[i].active&&!e.voices[i].release_frames&&(e.voices[i].id==6||e.voices[i].id==8||e.voices[i].id==10))n++;return n;}
static uint64_t signal_energy(void){uint64_t x=0;for(unsigned i=0;i<4410*2;i++){int32_t v=stereo[i];x+=(uint32_t)(v<0?-v:v);}return x;}
int main(void){
 track_mixer_init(&m);track_drum_init(&e);
 assert(track_drum_trigger_pair(&e,2,3)==1);assert(count_live_hats()==1);assert(e.hh_last==6||e.hh_last==8);
 track_drum_stop(&e);assert(track_drum_trigger_pair(&e,3,2)==1);assert(count_live_hats()==1);
 track_drum_stop(&e);assert(track_drum_trigger_pair(&e,2,2)==1);assert(count_live_hats()==1);
 track_drum_stop(&e);assert(track_drum_trigger_pair(&e,0,2)==2);assert(count_live_hats()==1);
 int kick=0,hat=0;for(unsigned i=0;i<TRACK_DRUM_POLY;i++)if(e.voices[i].active){kick+=e.voices[i].id==0;hat+=e.voices[i].id==6||e.voices[i].id==8;}assert(kick==1&&hat==1);
 /* 64 uninterrupted 16th HHC steps at fast tempo; every trigger is audible,
    alternating both sample variants. Output buffer is pre-zeroed, as in DMA. */
 track_drum_stop(&e);track_drum_init(&e);
 uint64_t level[2]={0,0};uint8_t previous=255;
 for(unsigned step=0;step<64;step++){
  assert(track_drum_trigger_pair(&e,2,255)==1);
  assert(count_live_hats()==1);
  uint8_t newest=255;for(unsigned i=0;i<TRACK_DRUM_POLY;i++)if(e.voices[i].active&&!e.voices[i].release_frames&&(e.voices[i].id==6||e.voices[i].id==8))newest=e.voices[i].id;
  assert(newest==6||newest==8);if(step)assert(newest!=previous);previous=newest;
  memset(stereo,0,sizeof(stereo));track_drum_render(&e,&m,stereo,4410);
  uint64_t energy=signal_energy();assert(energy>50000);level[(newest==8)?1:0]+=energy;
 }
 /* Both alternate hats must have comparable integrated playback levels. */
 assert(level[0]>0&&level[1]>0);
 double ratio=(double)level[1]/(double)level[0];
 printf("HHC level ratio %.3f (HH2/HH1); tested 64 dense hits\n",ratio);
 assert(ratio>0.55&&ratio<1.65);
 /* All 14 logical drum selectors stay audible in isolation. */
 for(unsigned instrument=0;instrument<TRACK_DRUM_COUNT;instrument++){
  track_drum_init(&e);assert(track_drum_trigger(&e,(uint8_t)instrument));
  memset(stereo,0,sizeof(stereo));track_drum_render(&e,&m,stereo,4410);
  assert(signal_energy()>1000);
 }
 puts("TRACK V0.16: all 14 drums audible, two-lane priority, 64 rapid HHC, no silent steps PASS");
 return 0;
}
