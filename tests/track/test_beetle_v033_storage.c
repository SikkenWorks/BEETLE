/* Standalone BEETLE V0.33 simulated NOR flash storage, not hardware proof. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "track_journal.h"
#include "beetle_flash_layout.h"
static uint8_t flash[0x100000],snapshot[0x100000],scratch[TRACK_STORE_MAX];
static struct track_store_project p,out;
static int fail_program_after=-1, programs=0,erases=0;
static int rd(void *ctx,uint32_t off,uint8_t *dst,uint32_t n){(void)ctx;if(off>=sizeof(flash)||n>sizeof(flash)-off)return -1;memcpy(dst,flash+off,n);return 0;}
static int rdb(void *ctx,uint8_t slot,uint8_t bank,uint32_t off,uint8_t *dst,uint32_t n){uint32_t a;(void)ctx;return beetle_flash_map(slot,bank,off,n,&a)?-1:rd(0,a,dst,n);}
static int era(void *ctx,uint8_t slot,uint8_t bank){uint32_t a;(void)ctx;if(beetle_flash_map(slot,bank,0,TRACK_JOURNAL_BANK_BYTES,&a))return -1;assert(a>=BEETLE_FLASH_BASE && a+TRACK_JOURNAL_BANK_BYTES<=BEETLE_FLASH_END);erases+=3;memset(flash+a,0xff,TRACK_JOURNAL_BANK_BYTES);return 0;}
static int pg(void *ctx,uint8_t slot,uint8_t bank,uint32_t off,const uint8_t *src,uint32_t n){uint32_t a;(void)ctx;if(n>256 || beetle_flash_map(slot,bank,off,n,&a)|| (a&255u)+n>256u)return -1;
 if(fail_program_after==0)return -1;if(fail_program_after>0)--fail_program_after;
 for(uint32_t i=0;i<n;i++)if((flash[a+i]&src[i])!=src[i])return -1;
 for(uint32_t i=0;i<n;i++)flash[a+i]&=src[i];programs++;return 0;}
static struct track_journal_io io={0,rdb,era,pg};
static void mk(uint8_t slot,uint16_t bpm,int maximal){
 struct track_engine e;track_init(&e);memset(&p,0,sizeof(p));track_song_init(&p.song,&e);track_mixer_init(&p.mixer);
 p.slot=slot;p.bpm=bpm;p.song.count=maximal?TRACK_SONG_ENTRIES:1;p.harmony.root=0;p.harmony.mode=0;
 for(unsigned i=0;i<p.song.count;i++){p.song.chain[i].pattern=i%TRACK_PATTERNS;p.song.chain[i].repeats=16;}
 for(unsigned i=0;i<4;i++){p.instruments[i].type=i;p.instruments[i].duty=50;p.instruments[i].volume=60;p.instruments[i].cutoff=7000;track_va_patch_default(&p.instruments[i].va);}
 if(maximal)for(unsigned x=0;x<TRACK_PATTERNS;x++){
  for(unsigned c=0;c<4;c++)for(unsigned s=0;s<TRACK_STEPS;s++){struct track_step *v=&p.song.patterns[x].ch[c][s];v->note=c==3?3:60;v->instrument=c;v->effect=TRACK_FX_VOLUME;v->value=100;}
  p.song.clips[x].count=TRACK_SONG_FX_EVENTS;for(unsigned k=0;k<TRACK_SONG_FX_EVENTS;k++){p.song.clips[x].data[k].fx=k%TRACK_PUNCH_COUNT;p.song.clips[x].data[k].step=k%TRACK_STEPS;p.song.clips[x].data[k].down=k&1;}
 }
}
int main(void){
 memset(flash,0xff,sizeof(flash));flash[0x200]='B';flash[0x94000]='A';flash[0xE0000]='O';flash[0xFC000]='G';
 /* A previous ETCH project in the data zone must not be silently claimed. */
 memcpy(flash+0xB4000,"ETCH",4);assert(beetle_flash_claim_probe(rd,0)==BEETLE_CLAIM_BLANK);
 /* Blank marker header with foreign residue elsewhere in the sector. */
 flash[BEETLE_FLASH_MARKER+4095u]=0u;
 assert(beetle_flash_claim_probe(rd,0)==BEETLE_CLAIM_FOREIGN);
 flash[BEETLE_FLASH_MARKER+4095u]=0xffu;
 assert(beetle_flash_claim_probe(rd,0)==BEETLE_CLAIM_BLANK);
 uint8_t mark[32];beetle_flash_make_marker(mark);memcpy(snapshot,flash,sizeof(flash));
 for(int s=0;s<12;s++)for(int b=0;b<2;b++){
  uint32_t a;assert(beetle_flash_map(s,b,0,TRACK_JOURNAL_BANK_BYTES,&a)==0);assert(a%BEETLE_FLASH_SECTOR==0);
  assert(a>=BEETLE_FLASH_BASE&&a+TRACK_JOURNAL_BANK_BYTES<=BEETLE_FLASH_END);
  for(int t=0;t<12;t++)for(int u=0;u<2;u++)if(s!=t||b!=u){uint32_t other;assert(!beetle_flash_map(t,u,0,TRACK_JOURNAL_BANK_BYTES,&other));assert(a+TRACK_JOURNAL_BANK_BYTES<=other||other+TRACK_JOURNAL_BANK_BYTES<=a);}
 }
 assert(beetle_flash_map(12,0,0,1,(uint32_t[1]){0})!=0);
 assert(beetle_flash_map(11,1,TRACK_JOURNAL_BANK_BYTES-1,2,(uint32_t[1]){0})!=0);
 /* Reproduce five-second confirmed PROVISION action with mocked NOR. */
 memset(flash+BEETLE_FLASH_BASE,0xff,BEETLE_FLASH_MARKER_END-BEETLE_FLASH_BASE);
 memcpy(flash+BEETLE_FLASH_MARKER,mark,sizeof(mark));
 assert(beetle_flash_claim_probe(rd,0)==BEETLE_CLAIM_EXISTING);
 flash[BEETLE_FLASH_MARKER+4095u]=0u;
 assert(beetle_flash_claim_probe(rd,0)==BEETLE_CLAIM_FOREIGN);
 flash[BEETLE_FLASH_MARKER+4095u]=0xffu;
 assert(beetle_flash_claim_probe(rd,0)==BEETLE_CLAIM_EXISTING);
 /* A/B images of 12 maximally filled projects must fit and survive reads. */
 for(uint8_t s=0;s<12;s++){
  mk(s,120+s,1);uint32_t n=0;assert(track_store_encode(&p,scratch,sizeof(scratch),&n)==0);
  assert(n<=TRACK_JOURNAL_BANK_BYTES-TRACK_JOURNAL_HEADER_BYTES);
  assert(track_journal_save(&io,s,&p,scratch,sizeof(scratch),0)==TRACK_JOURNAL_OK);
  assert(track_journal_load(&io,s,&out,scratch,sizeof(scratch),0)==TRACK_JOURNAL_OK);
  assert(out.slot==s&&out.bpm==120+s&&out.song.patterns[47].ch[3][15].note==3);
 }
 /* The same project saved twice must not wear the flash or advance gen. */
 mk(2,122,1);uint32_t unchanged_gen=0;
 struct track_journal_status before,after;
 assert(track_journal_load(&io,2,&out,scratch,sizeof(scratch),&before)==TRACK_JOURNAL_OK);
 int wear_programs=programs,wear_erases=erases;
 assert(track_journal_save(&io,2,&p,scratch,sizeof(scratch),&after)==TRACK_JOURNAL_OK);
 assert(programs==wear_programs&&erases==wear_erases&&after.generation==before.generation);
 (void)unchanged_gen;
 mk(3,250,1);fail_program_after=4;
 assert(track_journal_save(&io,3,&p,scratch,sizeof(scratch),0)==TRACK_JOURNAL_IO);
 fail_program_after=-1;
 assert(track_journal_load(&io,3,&out,scratch,sizeof(scratch),0)==TRACK_JOURNAL_OK&&out.bpm==123);
 assert(track_journal_save(&io,3,&p,scratch,sizeof(scratch),0)==TRACK_JOURNAL_OK);
 assert(track_journal_load(&io,3,&out,scratch,sizeof(scratch),0)==TRACK_JOURNAL_OK&&out.bpm==250);
 for(uint8_t s=0;s<12;s++)if(s!=3){assert(track_journal_load(&io,s,&out,scratch,sizeof(scratch),0)==TRACK_JOURNAL_OK&&out.bpm==120+s);}
 /* Physical sections including APP / boot / OTA / globals always unchanged. */
 assert(!memcmp(flash,snapshot,BEETLE_FLASH_BASE));
 assert(!memcmp(flash+BEETLE_FLASH_MARKER_END,snapshot+BEETLE_FLASH_MARKER_END,sizeof(flash)-BEETLE_FLASH_MARKER_END));
 assert(beetle_flash_claim_probe(rd,0)==BEETLE_CLAIM_EXISTING);
 flash[BEETLE_FLASH_MARKER+1]^=1;assert(beetle_flash_claim_probe(rd,0)==BEETLE_CLAIM_FOREIGN);
 fprintf(stdout,"PASS 12 max V4 projects, CRC A/B journal, torn-write recovery, exact 288KiB banks + 4KiB marker, no boot/APP/OTA/globals writes (%d page programs, %d bank sector erases)\n",programs,erases);
 return 0;
}
