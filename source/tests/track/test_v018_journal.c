/* Host-only fault-injection tests for TRACK V0.18 A/B journal. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "track_journal.h"
static uint8_t disk[4][2][TRACK_JOURNAL_BANK_BYTES];
static int errors=0, operations=0, fail_operation=-1, erases=0;
#define REQUIRE(X) do { if(!(X)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#X);errors++;} } while(0)
static int fail(void){return fail_operation>=0&&operations++==fail_operation;}
static int mread(void *ctx,uint8_t s,uint8_t b,uint32_t pos,uint8_t *dst,uint32_t n){
 (void)ctx;if(s>=4||b>=2||pos>TRACK_JOURNAL_BANK_BYTES||n>TRACK_JOURNAL_BANK_BYTES-pos)return -1;
 memcpy(dst,disk[s][b]+pos,n);return 0;
}
static int merase(void *ctx,uint8_t s,uint8_t b){
 (void)ctx;if(s>=4||b>=2||fail())return -1; erases++;memset(disk[s][b],0xFF,TRACK_JOURNAL_BANK_BYTES);return 0;
}
static int mprogram(void *ctx,uint8_t s,uint8_t b,uint32_t pos,const uint8_t *src,uint32_t n){
 (void)ctx;if(s>=4||b>=2||pos>TRACK_JOURNAL_BANK_BYTES||n>TRACK_JOURNAL_BANK_BYTES-pos||fail())return -1;
 for(uint32_t k=0;k<n;k++)if((disk[s][b][pos+k]&src[k])!=src[k])return -1;
 for(uint32_t k=0;k<n;k++){disk[s][b][pos+k]&=src[k];}
 return 0;
}
static const struct track_journal_io io={0,mread,merase,mprogram};
static struct track_store_project project,readback;
static uint8_t scratch[TRACK_STORE_MAX];
static void init_project(uint8_t slot){
 struct track_engine e;track_init(&e); memset(&project,0,sizeof(project));
 track_song_init(&project.song,&e);track_mixer_init(&project.mixer);
 project.slot=slot;project.bpm=120;memcpy(project.title,"TRACK JOURNAL",12);
}
int main(void){
 memset(disk,0xFF,sizeof(disk));
 for(uint8_t slot=0;slot<4;slot++){
  init_project(slot);
  project.song.patterns[47].ch[3][15].note=2;
  project.song.patterns[47].ch[0][0].note=(uint8_t)(61u+slot);
  project.song.clips[47].data[0]=(struct track_song_event){15u,2u,1u};
  project.song.clips[47].count=1;
  project.song.chain[0].pattern=47u;project.song.chain[0].repeats=3u;
  struct track_journal_status st={0};
  REQUIRE(track_journal_load(&io,slot,&readback,scratch,sizeof(scratch),&st)==TRACK_JOURNAL_NO_PROJECT);
  REQUIRE(track_journal_save(&io,slot,&project,scratch,sizeof(scratch),&st)==TRACK_JOURNAL_OK);
  REQUIRE(st.generation==1u&&st.active_bank==0u&&st.payload_bytes>15000u);
  REQUIRE(track_journal_load(&io,slot,&readback,scratch,sizeof(scratch),&st)==TRACK_JOURNAL_OK);
  REQUIRE(readback.slot==slot&&readback.song.patterns[47].ch[3][15].note==2u);
  REQUIRE(readback.song.patterns[47].ch[0][0].note==(uint8_t)(61u+slot));
  REQUIRE(readback.song.clips[47].data[0].step==15u);
  REQUIRE(readback.song.chain[0].pattern==47u);
 }
 init_project(0);project.bpm=137;
 /* Every interruption point in the second-bank write must leave generation 1 readable. */
 uint32_t wire_size=0;REQUIRE(track_store_encode(&project,scratch,sizeof(scratch),&wire_size)==0);
 int failpoints=(int)(3u+(wire_size+255u)/256u);
 for(int stop=0;stop<failpoints;stop++){
  memset(disk[0][1],0xFF,TRACK_JOURNAL_BANK_BYTES);
  fail_operation=stop;operations=0;
  int r=track_journal_save(&io,0,&project,scratch,sizeof(scratch),0);
  fail_operation=-1;
  REQUIRE(r==TRACK_JOURNAL_IO);
  struct track_journal_status st;
  REQUIRE(track_journal_load(&io,0,&readback,scratch,sizeof(scratch),&st)==TRACK_JOURNAL_OK);
  REQUIRE(readback.bpm==120u && st.generation==1u && st.active_bank==0u);
 }
 /* Successful journal commit, automatic fallback after bit rot. */
 memset(disk[0][1],0xFF,TRACK_JOURNAL_BANK_BYTES);
 struct track_journal_status st;
 REQUIRE(track_journal_save(&io,0,&project,scratch,sizeof(scratch),&st)==TRACK_JOURNAL_OK);
 REQUIRE(st.generation==2u&&st.active_bank==1u);
 REQUIRE(track_journal_load(&io,0,&readback,scratch,sizeof(scratch),&st)==TRACK_JOURNAL_OK&&readback.bpm==137u);
 disk[0][1][TRACK_JOURNAL_HEADER_BYTES+40]^=0x01u;
 REQUIRE(track_journal_load(&io,0,&readback,scratch,sizeof(scratch),&st)==TRACK_JOURNAL_OK);
 REQUIRE(st.generation==1u&&readback.bpm==120u);
 /* A foreign bank must not be erased or written under any circumstances. */
 disk[0][1][0]='F';disk[0][1][1]='E';disk[0][1][2]='L';disk[0][1][3]='U';
 int before=erases;
 REQUIRE(track_journal_save(&io,0,&project,scratch,sizeof(scratch),0)==TRACK_JOURNAL_FOREIGN);
 REQUIRE(erases==before);
 REQUIRE(track_journal_load(&io,0,&readback,scratch,sizeof(scratch),0)==TRACK_JOURNAL_FOREIGN);
 if(errors){fprintf(stderr,"FAILED: %d assertions\n",errors);return 1;}
 printf("PASS: four slots, 48 patterns, double drum notes, Punch-In events, A/B rollback, %d power-loss injection points, CRC fallback, foreign flash refusal\n",failpoints);
 return 0;
}
