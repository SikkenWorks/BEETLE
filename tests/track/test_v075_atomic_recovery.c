/* BEETLE V0.75: host-only NOR fault injection and nested project-header validation.
 * No hardware writes; GPL-3.0-only. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "track_journal.h"
#include "track_master_fx.h"
#include "beetle_instrument_library.h"
static uint8_t banks[2][TRACK_JOURNAL_BANK_BYTES];
static uint8_t baseline[2][TRACK_JOURNAL_BANK_BYTES];
static uint8_t scratch[TRACK_STORE_MAX];
static struct track_store_project project,loaded;
static unsigned steps, target_step, faults, attempts;
static int fail_mode; /* 0 normal, 1 read, 2 erase, 3 program */
static int should_fail(int mode){if(mode!=fail_mode)return 0;return steps++==target_step;}
static int rd(void *ctx,uint8_t s,uint8_t b,uint32_t at,uint8_t *p,uint32_t n){
 (void)ctx;if(s!=0||b>=2||at>TRACK_JOURNAL_BANK_BYTES||n>TRACK_JOURNAL_BANK_BYTES-at)return -1;
 if(should_fail(1)){memset(p,0,n/2u);faults++;return -1;}
 memcpy(p,banks[b]+at,n);return 0;
}
static int erase(void *ctx,uint8_t s,uint8_t b){
 (void)ctx;if(s!=0||b>=2)return -1;
 if(should_fail(2)){memset(banks[b],0xff,TRACK_JOURNAL_BANK_BYTES/2u);faults++;return -1;}
 memset(banks[b],0xff,TRACK_JOURNAL_BANK_BYTES);return 0;
}
static int prog(void *ctx,uint8_t s,uint8_t b,uint32_t at,const uint8_t *p,uint32_t n){
 (void)ctx;if(s!=0||b>=2||at>TRACK_JOURNAL_BANK_BYTES||n>TRACK_JOURNAL_BANK_BYTES-at)return -1;
 const int fails=should_fail(3);
 const uint32_t written=fails?(n+1u)/2u:n;
 for(uint32_t j=0;j<written;j++){
  if((banks[b][at+j]&p[j])!=p[j])return -1;
  banks[b][at+j]&=p[j];
 }
 if(fails){faults++;return -1;}return 0;
}
static const struct track_journal_io io={0,rd,erase,prog};
static void configure(void){
 struct track_engine e;track_init(&e);memset(&project,0,sizeof(project));
 track_song_init(&project.song,&e);track_mixer_init(&project.mixer);
 track_master_default(&project.master);beetle_library_init(&project.library);
 for(uint8_t i=0;i<4;i++){
  project.instruments[i].type=i;project.instruments[i].volume=65;
  project.instruments[i].duty=50;project.instruments[i].cutoff=7000;
  track_va_patch_default(&project.instruments[i].va);
 }
 for(uint8_t i=0;i<TRACK_DRUM_COUNT;i++)track_drum_timbre_default(&project.drum[i]);
 memcpy(project.title,"V075 RECOVER",12);
 project.slot=0;project.bpm=120;project.mixer.quantize_mutes=1;
 /* A dense, but flash-fitting, arrangement crosses many NOR write pages. */
 for(unsigned pat=0;pat<16u;pat++)for(unsigned ch=0;ch<TRACK_CHANNELS;ch++)
  for(unsigned step=0;step<TRACK_STEPS;step++){
   struct track_step *v=&project.song.patterns[pat].ch[ch][step];
   v->note=(uint8_t)(ch==TRACK_DRUM_CHANNEL?(step%TRACK_DRUM_COUNT):(36u+(pat+ch+step)%48u));
  }
}
static void save(uint16_t bpm){
 project.bpm=bpm;
 assert(track_journal_save(&io,0,&project,scratch,sizeof scratch,NULL)==TRACK_JOURNAL_OK);
}
static void restore(void){memcpy(banks,baseline,sizeof banks);fail_mode=0;steps=0;}
static uint16_t load(void){
 assert(track_journal_load(&io,0,&loaded,scratch,sizeof scratch,NULL)==TRACK_JOURNAL_OK);
 return loaded.bpm;
}
static void put32(uint8_t *dst,uint32_t x){
 for(unsigned k=0;k<4;k++)dst[k]=(uint8_t)(x>>(8u*k));
}
static uint32_t get32(const uint8_t *p){
 return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
int main(void){
 memset(banks,0xff,sizeof banks);configure();save(120);save(130);
 assert(load()==130);memcpy(baseline,banks,sizeof banks);
 /* Save 140: at every read/erase/program failpoint, the previous bank (130)
  * is never changed. A torn commit may finish successfully, yielding 140;
  * anything else must still load 130. */
 for(int mode=1;mode<=3;mode++){
  unsigned tested=0;
  for(unsigned k=0;k<512;k++){
   restore();project.bpm=140;fail_mode=mode;target_step=k;steps=0;
   const int ret=track_journal_save(&io,0,&project,scratch,sizeof scratch,NULL);
   const unsigned observed=steps;
   fail_mode=0;
   if(k>=observed){assert(ret==TRACK_JOURNAL_OK);break;}
   assert(ret==TRACK_JOURNAL_IO);
   /* The bank containing the last successfully committed generation is
    * immutable while trying to update the other bank. */
   assert(memcmp(banks[1],baseline[1],TRACK_JOURNAL_BANK_BYTES)==0);
   const int st=track_journal_load(&io,0,&loaded,scratch,sizeof scratch,NULL);
   assert(st==TRACK_JOURNAL_OK||st==TRACK_JOURNAL_FOREIGN);
   if(st==TRACK_JOURNAL_OK)assert(loaded.bpm==130||loaded.bpm==140);
   /* A partially erased/programmed header can look foreign. The current
    * fail-closed policy intentionally blocks writes rather than guessing
    * that the interrupted target is owned. */
   project.bpm=150;
   const int saved=track_journal_save(&io,0,&project,scratch,sizeof scratch,NULL);
   assert(saved==TRACK_JOURNAL_OK||saved==TRACK_JOURNAL_FOREIGN);
   if(saved==TRACK_JOURNAL_OK)assert(load()==150);
   else assert(!memcmp(banks[1],baseline[1],TRACK_JOURNAL_BANK_BYTES));
   tested++;attempts++;
  }
  assert(tested>0);
  printf("PASS fault injection mode %d: %u simulated cuts\n",mode,tested);
 }
 assert(faults==attempts);
 restore();
 /* A correct OUTER checksum with an invalid INNER project header must not
  * become the active bank for a subsequent SAVE. Simulate an older buggy
  * writer: corrupt the newer inner magic; make the outer CRC consistent. */
 const uint32_t n=get32(banks[1]+12u);
 banks[1][TRACK_JOURNAL_HEADER_BYTES]^=0x1u;
 put32(banks[1]+16u,track_store_crc32(banks[1]+TRACK_JOURNAL_HEADER_BYTES,n));
 put32(banks[1]+24u,track_store_crc32(banks[1],20u));
 assert(load()==120); /* newest bank is not a valid TRK1 project */
 uint8_t sole_valid[TRACK_JOURNAL_BANK_BYTES];
 memcpy(sole_valid,banks[0],sizeof sole_valid);
 save(140);
 assert(!memcmp(sole_valid,banks[0],sizeof sole_valid));
 assert(load()==140);
 /* Inner payload CRC mismatch is equally non-authoritative. */
 restore();banks[1][TRACK_JOURNAL_HEADER_BYTES+24u]^=1u;
 put32(banks[1]+16u,track_store_crc32(banks[1]+TRACK_JOURNAL_HEADER_BYTES,get32(banks[1]+12u)));
 put32(banks[1]+24u,track_store_crc32(banks[1],20u));
 assert(load()==120);save(141);
 assert(!memcmp(banks[0],baseline[0],TRACK_JOURNAL_BANK_BYTES));
 assert(load()==141);
 printf("PASS V0.75 atomic recovery: %u fault points, nested header/body CRC and surviving bank retention\n",attempts);
 return 0;
}
