/* BEETLE V1.3.4: sparse three-FX slots and V21 persistent compatibility. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "track_project_store.h"
#include "track_master_fx.h"
#include "beetle_pattern_edit.h"
static struct track_store_project a,b;
static struct track_engine engine;
static struct beetle_pattern_editor editor;
static unsigned char encoded[TRACK_STORE_V4_MAX];
int main(void){
 memset(&a,0,sizeof(a));track_init(&engine);track_song_init(&a.song,&engine);
 track_mixer_init(&a.mixer);track_master_default(&a.master);
 a.slot=0;a.bpm=120;a.song.count=1;a.song.loop=1;
 for(unsigned i=0;i<8;i++){a.instruments[i].volume=65;a.instruments[i].cutoff=7000;track_va_patch_default(&a.instruments[i].va);}
 for(unsigned i=0;i<TRACK_DRUM_COUNT;i++)track_drum_timbre_default(&a.drum[i]);
 for(unsigned i=0;i<5;i++)a.punch_param[i]=50;
 a.song.patterns[0].ch[0][0].note=60;
 a.song.patterns[0].ch[0][0].effect=1u;
 uint8_t key=track_extra_key(0,0,0);
 assert(track_extra_add(&a.song,0,key,0,2u,55)==1);
 assert(track_extra_add(&a.song,0,key,1,4u,60)==1);
 assert(track_extra_count(&a.song,0,key)==2);
 assert(track_extra_add(&a.song,0,key,2,4u,50)==-2);
 assert(track_extra_find(&a.song,0,key,0)->effect==2u);
 assert(track_extra_find(&a.song,0,key,1)->effect==4u);
 /* CH6 secondary drum hit has independent three-slot FX identity. */
 a.song.patterns[0].ch[5][3].note=0;
 a.song.patterns[0].note2_fx[3].effect=TRACK_FX_VOLUME;
 uint8_t dk=track_extra_key(5,3,1);
 assert(track_extra_add(&a.song,0,dk,0,1u,88)==1);
 uint32_t used=0;assert(track_store_encode(&a,encoded,sizeof(encoded),&used)==0);
 assert(encoded[4]==21 && used>40);
 assert(track_store_decode(&b,encoded,used)==0);
 assert(track_extra_count(&b.song,0,key)==2);
 assert(track_extra_count(&b.song,0,dk)==1);
 assert(track_extra_find(&b.song,0,dk,0)->value==88);
 /* Explicit full pool: no overwrite or partial modification. */
 for(unsigned s=1;s<=45;s++){
  unsigned row=s%16,lane=(s/16)%5,pat=1+s/80;
  uint8_t key2=track_extra_key(lane,row,0);
  a.song.patterns[pat].ch[lane][row].effect=1u;
  assert(track_extra_add(&a.song,pat,key2,0,2u,33)==1);
 }
 assert(a.song.extras.count==48);
 struct track_extra_fx last=a.song.extras.data[47];
 a.song.patterns[1].ch[5][1].effect=1u;
 assert(track_extra_add(&a.song,1,track_extra_key(5,1,0),0,5u,70)==-1);
 assert(memcmp(&last,&a.song.extras.data[47],sizeof(last))==0);
 assert(a.song.extras.count==48);
 /* Removing FX1 must also remove FX2/3; no ghost FX in saved project. */
 a.song.patterns[0].ch[0][0].effect=0;
 track_extra_prune(&a.song,0,&a.song.patterns[0]);
 assert(track_extra_count(&a.song,0,key)==0);
 puts("PASS V1.3.4: FX1+FX2+FX3, CH6 NOTE2, V21 roundtrip, cap48 overlay signal, orphan prune");
 return 0;
}
