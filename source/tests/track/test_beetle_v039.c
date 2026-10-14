/* V0.39 regression tests: REAL V0.37 project format, frozen snapshot,
 * reverse direction, live stutter gate, and decreasing tape read speed. */
#include "track_chip.h"
#include "track_project_store.h"
#include "track_journal.h"
#include "track_master_fx.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static struct track_chip_engine engine;
static struct track_store_project old_project;
static unsigned char legacy[TRACK_STORE_MAX];
static int16_t pcm[8192*2];
static void seed_monotone(void){
 track_chip_init(&engine);
 for(unsigned i=0;i<4096;i++){int v=(int)i-2048;pcm[i*2]=(int16_t)v;pcm[i*2+1]=(int16_t)-v;}
 track_chip_process_punch(&engine,0,pcm,4096);
}
static void repeat_reverse(void){
 seed_monotone(); engine.punch_param[0]=45;
 for(unsigned i=0;i<8192;i++)pcm[2*i]=pcm[2*i+1]=12000;
 track_chip_process_punch(&engine,1,pcm,8192);
 /* 256 samples of crossfade-in; last part must not reflect live 12000. */
 for(unsigned i=1024;i<3072;i++)assert(pcm[2*i]<10000);
 /* Repeat is periodic after the transition; one frozen source, NOT a ring
  * still being overwritten with the dry input. */
 unsigned len=2u*(512u+(55u*(2816u-512u))/100u);
 for(unsigned i=700;i<900;i++)assert(abs(pcm[2*i]-pcm[2*(i+len)])<100);
 seed_monotone();engine.punch_param[1]=40;
 for(unsigned i=0;i<8192;i++)pcm[2*i]=pcm[2*i+1]=12000;
 track_chip_process_punch(&engine,2,pcm,8192);
 unsigned backwards=0, forwards=0;
 for(unsigned i=300;i<650;i++){
  if(pcm[2*i]<pcm[2*(i-1)])backwards++;
  if(pcm[2*i]>pcm[2*(i-1)])forwards++;
 }
 assert(backwards>150 && forwards<30);
 puts("PASS: repeat frozen snapshot, reverse actual backwards playback");
}
static void tape_and_stutter(void){
 seed_monotone();engine.punch_param[3]=100;
 for(unsigned i=0;i<4096;i++)pcm[2*i]=pcm[2*i+1]=9000;
 track_chip_process_punch(&engine,8,pcm,4096);
 unsigned pos1=engine.punch_position;
 track_chip_process_punch(&engine,8,pcm,4096);
 unsigned pos2=engine.punch_position;
 assert(pos2-pos1<pos1); /* Tape head *slows down* rather than applying distortion. */
 seed_monotone();engine.punch_param[4]=100;
 for(unsigned i=0;i<4096;i++)pcm[2*i]=pcm[2*i+1]=9000;
 track_chip_process_punch(&engine,16,pcm,4096);
 unsigned silent=0,loud=0;
 for(unsigned i=600;i<3200;i++){
  int x=abs(pcm[2*i]);if(x<200)silent++;if(x>7000)loud++;
 }
 assert(silent>400&&loud>400); /* Alternating mute/unmute, no sample loop. */
 assert(engine.punch_param[2]==0); /* Filter starts at zero. */
 engine.punch_param[2]=92;
 track_chip_process_punch(&engine,4,pcm,128);
 track_chip_process_punch(&engine,0,pcm,128);
 assert(engine.punch_param[2]==92); /* FILTER retained after release. */
 puts("PASS: tape-stop decreases playback rate, stutter gates live audio, filter value retained");
}
static void migration(void){
 FILE *f=fopen("tests/track/fixtures/beetle_v037_six_lanes.trk","rb");assert(f);
 size_t n=fread(legacy,1,sizeof(legacy),f);fclose(f);assert(n>128);
 assert(legacy[4]==6u); /* V0.37 really saved as format V6. */
 assert(track_store_decode(&old_project,legacy,(uint32_t)n)==0);
 assert(old_project.slot==7u&&old_project.bpm==123u);
 assert(old_project.song.patterns[0].ch[TRACK_DRUM_CHANNEL][0].note==3u);
 assert(old_project.song.patterns[0].ch[4][0].note==64u);
 assert(old_project.mixer.ch[3].swing==50u); /* Old ON becomes 50% */
 /* Migrated project now saves and loads as V8, retaining all channels. */
 track_master_default(&old_project.master);
 for(unsigned i=0;i<TRACK_DRUM_COUNT;i++)track_drum_timbre_default(&old_project.drum[i]);
 uint32_t used=0;assert(track_store_encode(&old_project,legacy,sizeof(legacy),&used)==0);
 assert(legacy[4]==8u);
 static struct track_store_project roundtrip;
 assert(track_store_decode(&roundtrip,legacy,used)==0);
 assert(roundtrip.song.patterns[0].ch[TRACK_DRUM_CHANNEL][0].note==3u);
 puts("PASS: real V0.37 V6 file loads and migrates into V8");
}
/* Simulate a real old A/B NOR journal (V0.37 header + V6 payload),
 * rather than only calling the decoder on raw project bytes. */
static uint8_t legacy_flash[2][TRACK_JOURNAL_BANK_BYTES];
static int flash_read(void *ctx,uint8_t slot,uint8_t bank,uint32_t off,uint8_t *dst,uint32_t n){
 (void)ctx;if(slot!=7||bank>=2||off+n>TRACK_JOURNAL_BANK_BYTES)return -1;
 memcpy(dst,legacy_flash[bank]+off,n);return 0;
}
static int flash_erase(void *ctx,uint8_t slot,uint8_t bank){
 (void)ctx;if(slot!=7||bank>=2)return -1;memset(legacy_flash[bank],255,TRACK_JOURNAL_BANK_BYTES);return 0;
}
static int flash_program(void *ctx,uint8_t slot,uint8_t bank,uint32_t off,const uint8_t *src,uint32_t n){
 (void)ctx;if(slot!=7||bank>=2||off+n>TRACK_JOURNAL_BANK_BYTES)return -1;
 for(uint32_t i=0;i<n;i++)legacy_flash[bank][off+i]&=src[i];return 0;
}
static void wr32(uint8_t *p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(i*8));}
static void old_journal(void){
 FILE *f=fopen("tests/track/fixtures/beetle_v037_six_lanes.trk","rb");assert(f);
 uint8_t image[TRACK_STORE_MAX];size_t n=fread(image,1,sizeof(image),f);fclose(f);
 assert(n>0&&n+32<TRACK_JOURNAL_BANK_BYTES);
 for(unsigned i=0;i<2;i++)memset(legacy_flash[i],255,sizeof(legacy_flash[i]));
 uint8_t *h=legacy_flash[0];
 wr32(h,TRACK_JOURNAL_MAGIC);h[4]=1;h[5]=0;h[6]=7;h[7]=248;
 wr32(h+8,3);wr32(h+12,(uint32_t)n);wr32(h+16,track_store_crc32(image,(uint32_t)n));h[20]=0;
 wr32(h+24,track_store_crc32(h,20));
 memcpy(h+32,image,n);
 struct track_journal_io io={0,flash_read,flash_erase,flash_program};
 static struct track_store_project restored;
 static uint8_t scratch[TRACK_STORE_MAX];struct track_journal_status status;
 assert(track_journal_load(&io,7,&restored,scratch,sizeof(scratch),&status)==TRACK_JOURNAL_OK);
 assert(restored.bpm==123&&restored.slot==7);
 assert(restored.song.patterns[0].ch[TRACK_DRUM_CHANNEL][0].note==3);
 assert(status.generation==3&&status.active_bank==0);
 puts("PASS: full V0.37 A/B NOR journal LOAD into V0.39, no erase");
}
int main(void){repeat_reverse();tape_and_stutter();migration();old_journal();return 0;}
