/* V0.37 DSP regressions: 14 distinct 16-bit drum sounds, tune, Punch-In and release. */
#include "track_chip.h"
#include "track_drum.h"
#include "track_mixer.h"
#include "track_core.h"
#include "track_song.h"
#include "track_project_store.h"
#include "track_master_fx.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint32_t crc(const int16_t *p,unsigned n){uint32_t h=2166136261u;for(unsigned j=0;j<n;j++){h^=(uint16_t)p[j];h*=16777619u;}return h;}
static void run_drum(void){
 struct track_drum_engine d;struct track_mixer m;track_mixer_init(&m);
 uint32_t hashes[TRACK_DRUM_COUNT]={0};
 for(unsigned drum=0;drum<TRACK_DRUM_COUNT;drum++){
  track_drum_init(&d);assert(track_drum_trigger(&d,drum)==1);
  int16_t out[4096*2]={0};track_drum_render(&d,&m,out,4096);
  uint32_t peak=0;int64_t energy=0;
  for(unsigned j=0;j<4096*2;j++){int32_t v=out[j];if(v<0)v=-v;if((uint32_t)v>peak)peak=(uint32_t)v;energy+=v;}
  assert(peak>10&&peak<32767&&energy>3000);
  hashes[drum]=crc(out,4096*2);
 }
 for(unsigned i=0;i<TRACK_DRUM_COUNT;i++)for(unsigned j=i+1;j<TRACK_DRUM_COUNT;j++)assert(hashes[i]!=hashes[j]);
 track_drum_init(&d);assert(track_drum_trigger_pair_tuned(&d,0,1,-12)==2);
 for(unsigned i=0;i<TRACK_DRUM_POLY;i++)if(d.voices[i].active){assert(d.voices[i].pitch_semitones==-12);}
 printf("V0.37 DRUM: 14 distinct rendered outputs, 16-bit peak headroom, -12 pitch PASS\n");
}
static void run_punch(void){
 struct track_chip_engine s;track_chip_init(&s);assert(s.punch_param[2]==0u);
 int16_t out[512*2];
 for(unsigned frame=0;frame<28;frame++){
  for(unsigned j=0;j<512;j++){int16_t v=(int16_t)((((j+frame*512u)*697u)%12000u)-6000);out[j*2]=v;out[j*2+1]=v/2;}
  track_chip_process_punch(&s,0u,out,512);
 }
 const uint8_t effects[]={1u,2u,4u,8u,16u};
 for(unsigned k=0;k<5;k++){
  int16_t reference[512*2];
  for(unsigned j=0;j<512;j++){int16_t v=(int16_t)((j*141u)%12000u-6000);reference[j*2]=v;reference[j*2+1]=v/2;}
  memcpy(out,reference,sizeof(out));
  track_chip_process_punch(&s,effects[k],out,512);
  assert(crc(reference,512*2)!=crc(out,512*2));
  for(unsigned i=0;i<512*2;i++)assert(out[i]>=-32768&&out[i]<=32767);
  s.punch_param[2]=87u;
  track_chip_process_punch(&s,0u,out,512);
  assert(s.punch_param[2]==87u);
 }
 printf("V0.37 PUNCH: repeat/reverse/filter/tape/stutter DSP and filter retention PASS\n");
}
static struct track_store_project timeline_a,timeline_b;
static uint8_t stored[TRACK_STORE_MAX];
static void test_persistence_and_wrap(void){
 struct track_engine e;track_init(&e);memset(&timeline_a,0,sizeof(timeline_a));
 track_song_init(&timeline_a.song,&e);track_mixer_init(&timeline_a.mixer);
 track_master_default(&timeline_a.master);for(unsigned i=0;i<TRACK_DRUM_COUNT;i++)track_drum_timbre_default(&timeline_a.drum[i]);
 timeline_a.bpm=123u;timeline_a.slot=7u;
 /* DRUM PITCH 0=-12 st, 12=unshifted, 24=+12: 3 distinct patterns. */
 for(unsigned idx=0;idx<3;idx++){
  struct track_step *step=&timeline_a.song.patterns[idx].ch[TRACK_DRUM_CHANNEL][4];
  step->note=2u;step->effect=TRACK_FX_DRUM_PITCH;step->value=(uint8_t)(idx*12u);
 }
 assert(track_song_record(&timeline_a.song,0,15,2,1)==0);
 assert(track_song_record(&timeline_a.song,0,15,2,0)==0);
 assert(timeline_a.song.clips[0].count==2);
 assert(timeline_a.song.clips[0].data[1].step==0);
 assert((track_song_effect_mask(&timeline_a.song,0,15)&4u)==4u);
 assert((track_song_effect_mask(&timeline_a.song,0,0)&4u)==0u);
 assert(track_song_record(&timeline_a.song,0,5,1,1)==0);
 assert(track_song_record(&timeline_a.song,0,5,1,0)==0);
 assert((track_song_effect_mask(&timeline_a.song,0,5)&2u)!=0);
 assert((track_song_effect_mask(&timeline_a.song,0,6)&2u)==0);
 uint32_t bytes=0;
 assert(track_store_encode(&timeline_a,stored,sizeof(stored),&bytes)==0);
 assert(bytes<12288u);
 assert(track_store_decode(&timeline_b,stored,bytes)==0);
 assert(timeline_b.bpm==123&&timeline_b.slot==7);
 for(unsigned idx=0;idx<3;idx++){
  const struct track_step *step=&timeline_b.song.patterns[idx].ch[TRACK_DRUM_CHANNEL][4];
  assert(step->note==2u&&step->effect==TRACK_FX_DRUM_PITCH);
  assert(step->value==idx*12u);
 }
 assert((track_song_effect_mask(&timeline_b.song,0,15)&4u)!=0);
 assert((track_song_effect_mask(&timeline_b.song,0,0)&4u)==0u);
 printf("V0.37 PROJECT: drum -12/0/+12 step FX persisted, Punch short-hit and step15 wrap PASS\n");
}
int main(void){run_drum();run_punch();test_persistence_and_wrap();return 0;}
