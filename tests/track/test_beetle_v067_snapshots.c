/* V0.67 four per-project performance mute snapshots and V13->V14 migration. */
#include "track_project_store.h"
#include "track_master_fx.h"
#include "beetle_performance_snapshots.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static struct track_store_project project, loaded;
static uint8_t encoded[TRACK_STORE_MAX];
static void wr32(uint8_t *d,uint32_t n){for(unsigned k=0;k<4;k++)d[k]=(uint8_t)(n>>(8u*k));}
static void rehash(uint32_t n){wr32(encoded+8,n);wr32(encoded+12,track_store_crc32(encoded+24,n-24));wr32(encoded+16,track_store_crc32(encoded,16));}
int main(void){
 struct track_engine e;track_init(&e);track_song_init(&project.song,&e);
 track_mixer_init(&project.mixer);track_master_default(&project.master);
 for(unsigned i=0;i<4;i++){project.instruments[i].volume=65u;track_va_patch_default(&project.instruments[i].va);}
 for(unsigned i=0;i<TRACK_DRUM_COUNT;i++)track_drum_timbre_default(&project.drum[i]);
 project.slot=2u;project.bpm=120u;
 beetle_snapshots_init(&project.snapshots);
 assert(sizeof(project.snapshots)==5u);
 project.mixer.ch[0].mute=1u;project.mixer.ch[2].mute=1u;project.mixer.ch[5].mute=1u;
 assert(beetle_snapshot_store(&project.snapshots,0u,&project.mixer));
 assert(!beetle_snapshot_store(&project.snapshots,0u,&project.mixer));
 assert(project.snapshots.masks[0]==(1u|4u|32u));
 struct beetle_mute_queue q={0u,0u};
 project.mixer.ch[0].mute=0u;project.mixer.ch[4].mute=1u;
 assert(beetle_mute_arm(&q,&project.mixer,3u,1u));
 assert(beetle_snapshot_recall(&project.snapshots,0u,&q,&project.mixer,1u));
 assert(q.pending==(1u|16u)); /* CH1 arm; CH4 cancelled; stale CH4 removed */
 assert(q.values==1u);
 assert(project.mixer.ch[4].mute==1u && project.mixer.ch[0].mute==0u);
 assert(beetle_mute_commit(&q,&project.mixer));
 assert(beetle_snapshot_mask(&project.mixer)==(1u|4u|32u));
 project.mixer.quantize_mutes=0u;
 for(unsigned i=0;i<6;i++)project.mixer.ch[i].mute=0u;
 assert(beetle_snapshot_recall(&project.snapshots,0u,&q,&project.mixer,1u));
 assert(beetle_snapshot_mask(&project.mixer)==(1u|4u|32u) && !q.pending);
 assert(!beetle_snapshot_recall(&project.snapshots,1u,&q,&project.mixer,0u));
 project.snapshots.masks[2]=0x3Fu;project.snapshots.valid|=4u;
 uint32_t n;assert(track_store_encode(&project,encoded,sizeof(encoded),&n)==0);
 assert(encoded[4]==TRACK_STORE_VERSION&&encoded[5]==0);
 assert(track_store_decode(&loaded,encoded,n)==0);
 assert(loaded.snapshots.valid==5u&&loaded.snapshots.masks[0]==37u&&loaded.snapshots.masks[2]==63u);
 assert(loaded.mixer.quantize_mutes==0u);
 /* Simulate a valid V13 end record without added snapshot and group bytes. */
 uint32_t old=n-10u;encoded[4]=13u;encoded[5]=0u;rehash(old);
 assert(track_store_decode(&loaded,encoded,old)==0);
 assert(loaded.snapshots.valid==0u);
 for(unsigned i=0;i<4;i++)assert(loaded.snapshots.masks[i]==0u);
 /* Bad masks are refused even with valid CRCs; no silent corrupted recall. */
 project.snapshots.masks[2]=0x40u;
 assert(track_store_encode(&project,encoded,sizeof(encoded),&n)==-1);
 project.snapshots.masks[2]=0x3Fu;
 assert(track_store_encode(&project,encoded,sizeof(encoded),&n)==0);
 encoded[n-7]=0x80u;rehash(n);
 assert(track_store_decode(&loaded,encoded,n)==-3);
 puts("PASS V0.67 4 slots, quantized/direct recall, pending cancellation, V14 codec, V13 migration, CRC validation");
 return 0;
}
