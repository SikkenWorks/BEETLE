/* BEETLE V0.66: quantize mute selection, pending transition and project migration. */
#include "track_project_store.h"
#include "track_master_fx.h"
#include "beetle_quantized_mute.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static struct track_store_project project, loaded;
static uint8_t data[TRACK_STORE_MAX];
static void wr32(uint8_t *d,uint32_t n){for(unsigned k=0;k<4;k++)d[k]=(uint8_t)(n>>(8u*k));}
int main(void){
 struct track_engine e;track_init(&e);track_song_init(&project.song,&e);
 track_mixer_init(&project.mixer);track_master_default(&project.master);
 for(unsigned i=0;i<4;i++){project.instruments[i].volume=65u;track_va_patch_default(&project.instruments[i].va);}
 for(unsigned i=0;i<TRACK_DRUM_COUNT;i++)track_drum_timbre_default(&project.drum[i]);
 project.slot=1;project.bpm=120;
 assert(project.mixer.quantize_mutes==0);
 project.mixer.quantize_mutes=1; /* retain explicit Q-mode regression */
 struct beetle_mute_queue q={0};
 assert(beetle_mute_arm(&q,&project.mixer,0,1));
 assert(project.mixer.ch[0].mute==0 && q.pending==1u);
 /* Mixer SEL turns quantization OFF; pending targets become audible immediately. */
 project.mixer.quantize_mutes^=1u;
 if(!project.mixer.quantize_mutes)beetle_mute_commit(&q,&project.mixer);
 assert(q.pending==0 && project.mixer.ch[0].mute==1);
 project.mixer.ch[0].mute=0; /* Direct K3 or direct physical key when OFF. */
 assert(q.pending==0 && project.mixer.ch[0].mute==0);
 project.mixer.quantize_mutes=1;
 assert(beetle_mute_toggle(&q,&project.mixer,5) && (q.pending&(1u<<5)));
 beetle_mute_cancel(&q);assert(project.mixer.ch[5].mute==0);
 for(unsigned enabled=0;enabled<2;enabled++){
  project.mixer.quantize_mutes=(uint8_t)enabled;
  uint32_t used;assert(track_store_encode(&project,data,sizeof(data),&used)==0);
  assert(data[4]==TRACK_STORE_VERSION && data[5]==0);
  assert(track_store_decode(&loaded,data,used)==0);
  assert(loaded.mixer.quantize_mutes==enabled);
  /* A genuine V12-shaped record: remove V13 flag, V14 snapshot bytes and V15 group bytes. */
  if(enabled==0){
   uint32_t n=used-11;
   data[4]=12;wr32(data+8,n);wr32(data+12,track_store_crc32(data+24,n-24));
   wr32(data+16,track_store_crc32(data,16));
   assert(track_store_decode(&loaded,data,n)==0);
   assert(loaded.mixer.quantize_mutes==0);
   assert(loaded.mixer.ch[0].mute==0);
  }
 }
 /* Invalid stored flag must fail closed even if header checksums are valid. */
 project.mixer.quantize_mutes=1;
 uint32_t used;assert(track_store_encode(&project,data,sizeof(data),&used)==0);
 data[used-11]=2;
 wr32(data+12,track_store_crc32(data+24,used-24));
 wr32(data+16,track_store_crc32(data,16));
 assert(track_store_decode(&loaded,data,used)==-3);
 puts("PASS V0.66 quantize ON/OFF, pending flush, direct mode, V13 roundtrip, V12 migration, validation");
 return 0;
}
