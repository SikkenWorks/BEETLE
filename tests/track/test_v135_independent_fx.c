#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "track_project_store.h"
#include "track_master_fx.h"
#include "beetle_pattern_edit.h"
#include "track_sustain_fx.h"
static struct track_store_project a,b;
static struct track_engine e;
static unsigned char bytes[TRACK_STORE_V4_MAX];
int main(void){
 memset(&a,0,sizeof(a));track_init(&e);track_song_init(&a.song,&e);
 track_mixer_init(&a.mixer);track_master_default(&a.master);
 a.slot=0;a.bpm=120;a.song.count=1;a.song.loop=1;
 for(unsigned i=0;i<8;i++){a.instruments[i].volume=65;a.instruments[i].cutoff=7000;track_va_patch_default(&a.instruments[i].va);}
 for(unsigned i=0;i<TRACK_DRUM_COUNT;i++)track_drum_timbre_default(&a.drum[i]);
 for(unsigned i=0;i<5;i++)a.punch_param[i]=50;
 const uint8_t key=track_extra_key(0u,0u,0u);
 struct track_step *st=&a.song.patterns[0].ch[0][0];st->note=60;st->effect=0;
 // FX3 must be usable before FX1/FX2.
 assert(track_extra_add(&a.song,0,key,1,TRACK_FX_VOLUME,77)==1);
 assert(track_extra_find(&a.song,0,key,0)==NULL);
 assert(track_extra_effect(track_extra_find(&a.song,0,key,1))==TRACK_FX_VOLUME);
 assert(track_extra_count(&a.song,0,key)==1u);
 uint32_t n=0;assert(track_store_encode(&a,bytes,sizeof(bytes),&n)==0);
 assert(bytes[4]==TRACK_STORE_VERSION&&n>40u);
 assert(track_store_decode(&b,bytes,n)==0);
 assert(track_extra_find(&b.song,0,key,0)==NULL);
 assert(track_extra_effect(track_extra_find(&b.song,0,key,1))==TRACK_FX_VOLUME);
 assert(b.song.patterns[0].ch[0][0].effect==0);
 // FX2 also works independently and can be removed without changing FX3.
 assert(track_extra_add(&b.song,0,key,0u,2u,40u)==1);
 track_extra_remove(&b.song,0,key,0u);
 assert(track_extra_find(&b.song,0,key,0u)==NULL);
 assert(track_extra_effect(track_extra_find(&b.song,0,key,1u))==TRACK_FX_VOLUME);
 // Clearing FX1 never discards independent extras.
 track_extra_prune(&b.song,0,&b.song.patterns[0]);
 assert(track_extra_find(&b.song,0,key,1u));
 uint32_t m=0;assert(track_store_encode(&b,bytes,sizeof(bytes),&m)==0);
 assert(track_store_decode(&a,bytes,m)==0);
 assert(track_extra_find(&a.song,0,key,1u));
 // Processor must accept FX3 without FX1.
 struct track_chip_engine chip; memset(&chip,0,sizeof(chip));
 beetle_step_fx_event(&chip,0u,&a.song.patterns[0].ch[0][0],2u,96u,120u);
 beetle_step_fx_multi(&chip,0u,track_extra_find(&a.song,0,key,0u),track_extra_find(&a.song,0,key,1u),120u);
 assert(chip.fx_extra_type[0][0]==0u);
 assert(chip.fx_extra_type[0][1]==TRACK_FX_VOLUME);
 assert(chip.step_gain[0]==77u);
 // Entire sparse pool and duplicate slot guard.
 for(uint8_t i=0;i<47u;i++){
   uint8_t pat=(uint8_t)(1u+i/32u),row=(uint8_t)(i%16u),lane=(uint8_t)((i%32u)/16u);
   assert(track_extra_add(&a.song,pat,track_extra_key(lane,row,0u),0u,2u,50u)==1);
 }
 assert(a.song.extras.count==48);
 assert(track_extra_add(&a.song,3u,track_extra_key(0,0,0),1,2,50)==-1);
 assert(track_extra_add(&a.song,0,key,1,2,50)==0);
 puts("PASS V1.3.5 native: independent FX slots, current-schema encode/decode, DSP FX3 without FX1, pool limit");
}
