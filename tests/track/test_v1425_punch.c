/* BEETLE V1.4.25: 16 Punch-In IDs, TRK1 V25 compatibility and DSP smoke. */
#include "track_project_store.h"
#include "track_chip.h"
#include "track_master_fx.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static struct track_store_project before,after,legacy;
static struct track_engine engine;
static uint8_t bytes[TRACK_STORE_V4_MAX];
static int16_t frames[512];
static void init_project(void){
 memset(&before,0,sizeof(before));track_init(&engine);
 track_song_init(&before.song,&engine);track_mixer_init(&before.mixer);
 track_master_default(&before.master);
 before.slot=0;before.bpm=120;before.song.count=1;before.song.loop=1;
 for(unsigned i=0;i<8;i++){before.instruments[i].volume=65;before.instruments[i].cutoff=7000;track_va_patch_default(&before.instruments[i].va);}
 for(unsigned i=0;i<TRACK_DRUM_COUNT;i++)track_drum_timbre_default(&before.drum[i]);
 for(unsigned i=0;i<13;i++)before.punch_param[i]=(uint8_t)(13+i*6);
}
static void recalc_crc(uint32_t n){
 uint32_t crc=track_store_crc32(bytes+24,n-24);
 for(unsigned i=0;i<4;i++)bytes[12+i]=(uint8_t)(crc>>(8*i));
 for(unsigned i=0;i<4;i++)bytes[8+i]=(uint8_t)(n>>(8*i));
 crc=track_store_crc32(bytes,16);
 for(unsigned i=0;i<4;i++)bytes[16+i]=(uint8_t)(crc>>(8*i));
}
int main(void){
 init_project();
 struct track_song_clip *clip=&before.song.clips[0];
 clip->count=3;
 clip->data[0]=(struct track_song_event){.step=0,.fx=8,.down=1,.param=61};
 clip->data[1]=(struct track_song_event){.step=1,.fx=15,.down=2,.param=79};
 clip->data[2]=(struct track_song_event){.step=2,.fx=15,.down=0,.param=255};
 uint32_t n=0;
 assert(track_store_encode(&before,bytes,sizeof(bytes),&n)==0);
 assert(bytes[4]==25u&&n>80u);
 assert(track_store_decode(&after,bytes,n)==0);
 for(unsigned i=0;i<13;i++)assert(after.punch_param[i]==before.punch_param[i]);
 assert(after.song.clips[0].count==3);
 assert(after.song.clips[0].data[0].fx==8&&after.song.clips[0].data[0].step==0);
 assert(after.song.clips[0].data[1].fx==15&&after.song.clips[0].data[1].down==2&&after.song.clips[0].data[1].param==79);
 assert(after.song.clips[0].data[2].fx==15&&!after.song.clips[0].data[2].down);
 puts("PASS native V25 sixteen effect IDs and thirteen params encode/decode");
 /* Reconstruct a genuine V24 project with empty event storage and the
  * original five param bytes. This catches the V25 version-gate regression. */
 clip->count=0;assert(track_store_encode(&before,bytes,sizeof(bytes),&n)==0);
 const uint8_t needle[]={13,19,25,31,37,43,49,55,61,67,73,79,85};
 uint32_t at=n;
 for(uint32_t i=24;i+sizeof(needle)<=n;i++)if(!memcmp(bytes+i,needle,sizeof(needle))){at=i;break;}
 assert(at<n);
 memmove(bytes+at+5,bytes+at+13,n-(at+13));n-=8;
 bytes[4]=24;recalc_crc(n);
 assert(track_store_decode(&legacy,bytes,n)==0);
 for(unsigned i=0;i<5;i++)assert(legacy.punch_param[i]==before.punch_param[i]);
 for(unsigned i=5;i<13;i++)assert(legacy.punch_param[i]==50);
 puts("PASS native TRK1 V24 migration and default new FX parameters");
 /* One 256-frame PCM interval per effect; all user parameters remain bounded.
  * This does not substitute for the real FM1 DMA stress tests. */
 static struct track_chip_engine chip;
 for(unsigned effect=0;effect<16;effect++){
  track_chip_init(&chip);
  for(unsigned j=0;j<8;j++)chip.punch_param[j]=50;
  chip.punch_bpm=120;chip.punch_focus=(uint8_t)effect;
  for(unsigned i=0;i<256;i++){
   frames[i*2]=(int16_t)(((i*373u)&16383u)-8192);
   frames[i*2+1]=(int16_t)(((i*287u)&16383u)-8192);
  }
  track_chip_process_punch(&chip,(uint16_t)(1u<<effect),frames,256);
  assert(track_chip_health(&chip));
 }
 puts("PASS native FX 01-16 host audio smoke / bounded index and health checks");
}
