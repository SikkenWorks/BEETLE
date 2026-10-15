/* BEETLE V0.71 A/B project journal power-loss and ownership qualification.
 * Host simulation only: no real FM1 flash hardware is accessed. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "track_journal.h"
#include "track_master_fx.h"
#include "beetle_instrument_library.h"
static uint8_t disk[TRACK_JOURNAL_SLOTS][2][TRACK_JOURNAL_BANK_BYTES];
static struct track_store_project project,loaded;
static uint8_t scratch[TRACK_STORE_MAX];
static int fail_step=-1,operation,erases;
static int io_fail(void){return fail_step>=0&&operation++==fail_step;}
static int mread(void *unused,uint8_t s,uint8_t b,uint32_t pos,uint8_t *to,uint32_t n){
 (void)unused;if(s>=TRACK_JOURNAL_SLOTS||b>=2||pos>TRACK_JOURNAL_BANK_BYTES||n>TRACK_JOURNAL_BANK_BYTES-pos)return -1;
 memcpy(to,disk[s][b]+pos,n);return 0;
}
static int merase(void *unused,uint8_t s,uint8_t b){
 (void)unused;if(s>=TRACK_JOURNAL_SLOTS||b>=2||io_fail())return -1;
 erases++;memset(disk[s][b],0xff,TRACK_JOURNAL_BANK_BYTES);return 0;
}
static int mprogram(void *unused,uint8_t s,uint8_t b,uint32_t pos,const uint8_t *src,uint32_t n){
 (void)unused;if(s>=TRACK_JOURNAL_SLOTS||b>=2||pos>TRACK_JOURNAL_BANK_BYTES||n>TRACK_JOURNAL_BANK_BYTES-pos||io_fail())return -1;
 for(uint32_t j=0;j<n;j++)if((disk[s][b][pos+j]&src[j])!=src[j])return -1;
 for(uint32_t j=0;j<n;j++)disk[s][b][pos+j]&=src[j];return 0;
}
static const struct track_journal_io io={NULL,mread,merase,mprogram};
static void setup(uint8_t slot){
 memset(&project,0,sizeof(project));
 struct track_engine engine;track_init(&engine);
 track_song_init(&project.song,&engine);
 track_mixer_init(&project.mixer);track_master_default(&project.master);
 for(uint8_t k=0;k<4;k++){
  project.instruments[k].volume=65;project.instruments[k].duty=50;
  project.instruments[k].cutoff=7000;project.instruments[k].type=k;
  track_va_patch_default(&project.instruments[k].va);
 }
 for(uint8_t k=0;k<TRACK_DRUM_COUNT;k++)track_drum_timbre_default(&project.drum[k]);
 beetle_library_init(&project.library);
 project.slot=slot;project.bpm=120;project.mixer.quantize_mutes=1;
 memcpy(project.title,"BEETLE TEST ",12);
 project.song.patterns[47].ch[0][15].note=(uint8_t)(60+slot);
 project.song.patterns[47].ch[TRACK_DRUM_CHANNEL][7].note=2;
 project.song.chain[0].pattern=47;project.song.chain[0].repeats=3;
}
int main(void){
 memset(disk,0xff,sizeof(disk));
 for(uint8_t slot=0;slot<TRACK_JOURNAL_SLOTS;slot++){
  struct track_journal_status st={0};setup(slot);
  assert(track_journal_load(&io,slot,&loaded,scratch,sizeof scratch,&st)==TRACK_JOURNAL_NO_PROJECT);
  assert(track_journal_save(&io,slot,&project,scratch,sizeof scratch,&st)==TRACK_JOURNAL_OK);
  assert(st.generation==1 && st.active_bank==0 && st.payload_bytes<=TRACK_STORE_V4_MAX);
  assert(track_journal_load(&io,slot,&loaded,scratch,sizeof scratch,&st)==TRACK_JOURNAL_OK);
  assert(loaded.slot==slot&&loaded.bpm==120);
  assert(loaded.song.patterns[47].ch[0][15].note==60+slot);
  const int before=erases;
  assert(track_journal_save(&io,slot,&project,scratch,sizeof scratch,&st)==TRACK_JOURNAL_OK);
  assert(erases==before); /* unchanged save must not wear NOR. */
 }
 setup(0);project.bpm=137;
 uint32_t used=0;
 assert(track_store_encode(&project,scratch,sizeof scratch,&used)==0);
 const int failpoints=3+(int)((used+255)/256);
 for(int stop=0;stop<failpoints;stop++){
  memset(disk[0][1],0xff,TRACK_JOURNAL_BANK_BYTES);
  operation=0;fail_step=stop;
  assert(track_journal_save(&io,0,&project,scratch,sizeof scratch,NULL)==TRACK_JOURNAL_IO);
  fail_step=-1;
  struct track_journal_status st={0};
  assert(track_journal_load(&io,0,&loaded,scratch,sizeof scratch,&st)==TRACK_JOURNAL_OK);
  assert(loaded.bpm==120 && st.generation==1 && st.active_bank==0);
 }
 memset(disk[0][1],0xff,TRACK_JOURNAL_BANK_BYTES);
 assert(track_journal_save(&io,0,&project,scratch,sizeof scratch,NULL)==TRACK_JOURNAL_OK);
 struct track_journal_status st;
 assert(track_journal_load(&io,0,&loaded,scratch,sizeof scratch,&st)==TRACK_JOURNAL_OK);
 assert(st.generation==2 && st.active_bank==1 && loaded.bpm==137);
 disk[0][1][TRACK_JOURNAL_HEADER_BYTES+45]^=0x01;
 assert(track_journal_load(&io,0,&loaded,scratch,sizeof scratch,&st)==TRACK_JOURNAL_OK);
 assert(st.generation==1 && st.active_bank==0 && loaded.bpm==120);
 const int before=erases;
 disk[0][1][0]='F';disk[0][1][1]='E';disk[0][1][2]='L';disk[0][1][3]='U';
 assert(track_journal_save(&io,0,&project,scratch,sizeof scratch,NULL)==TRACK_JOURNAL_FOREIGN);
 assert(erases==before);
 assert(track_journal_load(&io,0,&loaded,scratch,sizeof scratch,NULL)==TRACK_JOURNAL_FOREIGN);
 for(uint8_t slot=1;slot<TRACK_JOURNAL_SLOTS;slot++){
  assert(track_journal_load(&io,slot,&loaded,scratch,sizeof scratch,NULL)==TRACK_JOURNAL_OK);
  assert(loaded.bpm==120 && loaded.slot==slot);
 }
 printf("PASS V0.71: 12 slots, no-op saves, %d interruption points, CRC fallback, foreign protection\n",failpoints);
 return 0;
}
