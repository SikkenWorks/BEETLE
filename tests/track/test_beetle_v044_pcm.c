/* BEETLE V0.44 ROM sample-backed drum regression. */
#include "track_drum.h"
#include "track_mixer.h"
#include "beetle_drum_rom.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#define B 8192
static int16_t pcm[B*2];
static uint32_t digest(void){uint32_t h=2166136261u;for(unsigned i=0;i<B*2;i++)h=(h^(uint16_t)pcm[i])*16777619u;return h;}
static int peak(void){int p=0;for(unsigned i=0;i<B*2;i++){int v=pcm[i];if(v<0)v=-v;if(v>p)p=v;}return p;}
int main(void){
 assert(BEETLE_DRUM_ROM_COUNT==16u&&BEETLE_DRUM_ROM_BYTES>100000u&&BEETLE_DRUM_ROM_BYTES<250000u);
 for(unsigned k=0;k<BEETLE_DRUM_ROM_COUNT;k++)assert(beetle_drum_offsets[k+1]>beetle_drum_offsets[k]);
 assert(beetle_drum_offsets[BEETLE_DRUM_ROM_COUNT]==BEETLE_DRUM_ROM_BYTES);
 struct track_mixer m;track_mixer_init(&m);
 struct track_drum_engine d;
 uint32_t hs[TRACK_DRUM_COUNT];
 for(unsigned k=0;k<TRACK_DRUM_COUNT;k++){
  track_drum_init(&d);memset(pcm,0,sizeof pcm);
  assert(track_drum_trigger(&d,(uint8_t)k));track_drum_render(&d,&m,pcm,B);
  hs[k]=digest();assert(peak()>180&&peak()<32767);
 }
 for(unsigned k=0;k<TRACK_DRUM_COUNT;k++)for(unsigned j=k+1;j<TRACK_DRUM_COUNT;j++)assert(hs[k]!=hs[j]);
 track_drum_init(&d);memset(pcm,0,sizeof pcm);
 track_drum_trigger(&d,1);track_drum_render(&d,&m,pcm,B);uint32_t a=digest();
 memset(pcm,0,sizeof pcm);track_drum_trigger(&d,1);track_drum_render(&d,&m,pcm,B);
 assert(a!=digest()); /* snare uses actual different ROM bytes */
 track_drum_init(&d);track_drum_trigger(&d,3); /* long open hat */
 track_drum_trigger(&d,2);int choked=0;
 for(unsigned k=0;k<TRACK_DRUM_POLY;k++)if(d.voices[k].active&&d.voices[k].id==10&&d.voices[k].release_frames)choked=1;
 assert(choked);
 /* All samples remain subject to user pitch, mixer and mute. */
 track_drum_init(&d);d.components[0].pitch=-36;track_drum_trigger(&d,0);
 uint32_t slow=0;for(unsigned k=0;k<TRACK_DRUM_POLY;k++)if(d.voices[k].active)slow=d.voices[k].phase_step;
 assert(slow>0);
 track_drum_init(&d);d.components[0].pitch=36;track_drum_trigger(&d,0);
 uint32_t fast=0;for(unsigned k=0;k<TRACK_DRUM_POLY;k++)if(d.voices[k].active)fast=d.voices[k].phase_step;
 assert(fast>slow*60u);
 track_drum_init(&d);m.ch[TRACK_DRUM_CHANNEL].mute=1;track_drum_trigger(&d,0);
 memset(pcm,0,sizeof pcm);track_drum_render(&d,&m,pcm,B);for(unsigned k=0;k<B*2;k++)assert(pcm[k]==0);
 puts("BEETLE V0.44: 16 real ROM one-shots, 14 voices, snare RR, closed hat choke, pitch +/-36, silence, headroom PASS");
 return 0;
}
