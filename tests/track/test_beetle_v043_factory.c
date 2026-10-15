/* SPDX-License-Identifier: GPL-3.0-only
 * BEETLE V0.43 regression: usable full-band factory kit, envelope lengths,
 * independent instruments, master/lane safety, parameter reset and hat choke.
 */
#include "track_drum.h"
#include "track_mixer.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define N 44100
static int16_t out[N*2];
static uint32_t hash32(const int16_t* a,size_t n){uint32_t h=2166136261u;for(size_t k=0;k<n;k++){h^=(uint16_t)a[k];h*=16777619u;}return h;}
static uint64_t power(unsigned begin,unsigned end){uint64_t v=0;for(unsigned i=begin;i<end;i++){int64_t x=out[i*2];v+=(uint64_t)(x*x);}return v/(end-begin);}
static void render(unsigned key,struct track_drum_engine*d,struct track_mixer*m){memset(out,0,sizeof(out));track_drum_init(d);assert(track_drum_trigger(d,key));track_drum_render(d,m,out,N);}
int main(void){struct track_drum_engine d;struct track_mixer m;track_mixer_init(&m);uint32_t hashes[TRACK_DRUM_COUNT];
 for(unsigned i=0;i<TRACK_DRUM_COUNT;i++){
  render(i,&d,&m);hashes[i]=hash32(out,N*2);
  int peak=0;uint64_t acc=0;for(unsigned k=0;k<N*2;k++){int x=out[k];if(x<0)x=-x;if(x>peak)peak=x;acc+=(uint64_t)x;}
  assert(peak>200 && peak<32767);assert(acc>80000);
  struct track_drum_timbre p={0};track_drum_component_default(&p,(uint8_t)i);
  assert(memcmp(&p,&d.components[i],sizeof(p))==0);
  assert(d.components[i].gain>0u);
 }
 for(unsigned i=0;i<TRACK_DRUM_COUNT;i++)for(unsigned j=i+1;j<TRACK_DRUM_COUNT;j++)assert(hashes[i]!=hashes[j]);
 render(0,&d,&m);assert(power(0,1000)>power(5000,6000));/* ROM kick decays naturally */
 render(1,&d,&m);assert(power(0,1000)>power(5000,6000));/* ROM snare wire tail */
 render(2,&d,&m);const uint64_t hc_late=power(4500,7000);
 render(3,&d,&m);const uint64_t ho_late=power(4500,7000);assert(ho_late>hc_late*40u);
 render(7,&d,&m);assert(power(14000,17500)>50u); /* cymbal tail */
 render(8,&d,&m);assert(power(22000,26000)>25u); /* ride sustain */
 track_drum_init(&d);d.components[0].pitch=-36;assert(track_drum_trigger(&d,0));assert(d.voices[0].pitch_semitones==-36);
 track_drum_component_default(&d.components[0],0);assert(d.components[0].pitch==0);
 /* Global mute and lane volume must mute all drums as one instrument. */
 track_mixer_init(&m);m.ch[TRACK_DRUM_CHANNEL].volume=0u;render(0,&d,&m);
 for(unsigned k=0;k<N*2;k++)assert(out[k]==0);
 track_mixer_init(&m);m.ch[TRACK_DRUM_CHANNEL].mute=1u;render(1,&d,&m);
 for(unsigned k=0;k<N*2;k++)assert(out[k]==0);
 /* Six simultaneous instruments must leave headroom at factory settings. */
 track_mixer_init(&m);track_drum_init(&d);const uint8_t hits[]={0,1,2,4,7,9};
 for(unsigned i=0;i<sizeof(hits);i++)assert(track_drum_trigger(&d,hits[i]));
 memset(out,0,sizeof(out));track_drum_render(&d,&m,out,N);int peak=0;
 for(unsigned k=0;k<N*2;k++){int v=out[k];if(v<0)v=-v;if(v>peak)peak=v;}
 assert(peak<32767);
 track_drum_init(&d);track_drum_trigger(&d,3);track_drum_trigger(&d,2);
 unsigned open=0;for(unsigned i=0;i<TRACK_DRUM_POLY;i++)if(d.voices[i].active && d.voices[i].id==10 && d.voices[i].release_frames)open++;
 assert(open==1u);puts("BEETLE V0.43: 14 differentiated factory drums; kick/snare envelopes, open/closed hats, crash/ride tails, edit/reset, mute, headroom & choke PASS");return 0;}
