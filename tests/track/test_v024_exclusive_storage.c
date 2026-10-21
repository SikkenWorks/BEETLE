/* BEETLE V0.24 - physical address policy + BEETLE-only record isolation.
 * Host simulated 1MiB SPI NOR, no device writes. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "beetle_flash_layout.h"
#include "track_journal.h"
static unsigned fail_count,programs,erases;
static int fail_program_after=-1;
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);fail_count++;}}while(0)
static uint8_t flash[0x100000],scratch[TRACK_STORE_MAX];
static struct track_store_project p,out;
static int rd(void *ctx,uint32_t a,uint8_t *dst,uint32_t n){(void)ctx;if(a>sizeof(flash)||n>sizeof(flash)-a)return -1;memcpy(dst,flash+a,n);return 0;}
static int rd_bank(void *c,uint8_t s,uint8_t b,uint32_t o,uint8_t *dst,uint32_t n){uint32_t a;return beetle_flash_map(s,b,o,n,&a)?-1:rd(c,a,dst,n);}
static int er_bank(void *c,uint8_t s,uint8_t b){(void)c;uint32_t a;
 if(beetle_flash_map(s,b,0,TRACK_JOURNAL_BANK_BYTES,&a))return -1;
 for(uint32_t at=0;at<TRACK_JOURNAL_BANK_BYTES;at+=BEETLE_FLASH_SECTOR){if(a+at<BEETLE_FLASH_BASE||a+at>=BEETLE_FLASH_END)return -1;erases++;memset(flash+a+at,0xff,BEETLE_FLASH_SECTOR);}return 0;
}
static int pg_bank(void *c,uint8_t s,uint8_t b,uint32_t o,const uint8_t *src,uint32_t n){(void)c;uint32_t a;
 if(!src||n>256u||beetle_flash_map(s,b,o,n,&a)||(a&255u)+n>256u)return -1;
 if(fail_program_after==0)return -1;
 if(fail_program_after>0)fail_program_after--;
 for(uint32_t i=0;i<n;i++)if((flash[a+i]&src[i])!=src[i])return -1;
 for(uint32_t i=0;i<n;i++)flash[a+i]&=src[i];programs++;return 0;
}
static struct track_journal_io io={0,rd_bank,er_bank,pg_bank};
static void mk(uint8_t slot){struct track_engine e;track_init(&e);memset(&p,0,sizeof(p));track_song_init(&p.song,&e);track_mixer_init(&p.mixer);
 p.slot=slot;p.bpm=121u+slot;p.harmony.root=0u;p.harmony.mode=0u;
 memcpy(p.title,"BEETLE ONLY",11u);
 for(unsigned i=0;i<4;i++){p.instruments[i].type=(uint8_t)i;p.instruments[i].volume=65;p.instruments[i].duty=50;p.instruments[i].cutoff=7000;track_va_patch_default(&p.instruments[i].va);}
 p.song.patterns[47].ch[3][15].note=2;
}
int main(void){
 memset(flash,0xff,sizeof(flash));
 flash[0x4000u]='F';flash[0xE0000u]='O';flash[0xFC000u]='G';
 CHECK(beetle_flash_claim_probe(rd,0)==BEETLE_CLAIM_BLANK);
 for(uint8_t s=0;s<4;s++){
  mk(s);CHECK(track_journal_save(&io,s,&p,scratch,sizeof(scratch),0)==TRACK_JOURNAL_OK);
  CHECK(track_journal_load(&io,s,&out,scratch,sizeof(scratch),0)==TRACK_JOURNAL_OK);
  CHECK(out.slot==s&&out.bpm==121+s&&out.song.patterns[47].ch[3][15].note==2);
 }
 CHECK(beetle_flash_claim_probe(rd,0)==BEETLE_CLAIM_EXISTING);
 /* Interrupted write on target B leaves A intact, and interrupted B cannot
  * be mistaken for ETCH/Felucca samples or promoted to current project. */
 mk(0);p.bpm=209u;fail_program_after=10;
 CHECK(track_journal_save(&io,0,&p,scratch,sizeof(scratch),0)==TRACK_JOURNAL_IO);
 fail_program_after=-1;
 CHECK(beetle_flash_claim_probe(rd,0)==BEETLE_CLAIM_EXISTING);
 CHECK(track_journal_load(&io,0,&out,scratch,sizeof(scratch),0)==TRACK_JOURNAL_OK);
 CHECK(out.bpm==121u);
 CHECK(flash[0x4000u]=='F'&&flash[0xE0000u]=='O'&&flash[0xFC000u]=='G');
 for(uint8_t s=0;s<4;s++){
  mk(s);p.bpm=190+s;CHECK(track_journal_save(&io,s,&p,scratch,sizeof(scratch),0)==TRACK_JOURNAL_OK);
 }
 CHECK(beetle_flash_claim_probe(rd,0)==BEETLE_CLAIM_EXISTING);
 /* Simulate power interruption after header and one page, before commit.
  * The other BEETLE bank must remain loadable; uncommitted new bank is
  * recognized only with a committed partner. */
 memset(flash+BEETLE_FLASH_BASE+TRACK_JOURNAL_BANK_BYTES,0xff,TRACK_JOURNAL_BANK_BYTES);
 uint32_t at=BEETLE_FLASH_BASE+TRACK_JOURNAL_BANK_BYTES;
 flash[at]='T';flash[at+1]='R';flash[at+2]='K';flash[at+3]='J';
 /* Partial header alone isn't proof: fail closed, requiring re-provisioning. */
 CHECK(beetle_flash_claim_probe(rd,0)==BEETLE_CLAIM_FOREIGN);
 memset(flash+at,0xff,TRACK_JOURNAL_BANK_BYTES);
 CHECK(beetle_flash_claim_probe(rd,0)==BEETLE_CLAIM_EXISTING);
 memcpy(flash+0xA0000u+254u,"FSMP",4);
 CHECK(beetle_flash_has_felucca_samples(rd,0)==1);
 CHECK(beetle_flash_claim_probe(rd,0)==BEETLE_CLAIM_FOREIGN);
 memset(flash+0xA0000u+254u,0xff,4);
 uint32_t before=erases;
 mk(0);
 /* V0.25 regression: erased *header* but non-erased trailing bytes is
  * never a blank bank and must not be erased even with a valid A bank. */
 memset(flash+at,0xff,TRACK_JOURNAL_BANK_BYTES);
 flash[at+BEETLE_FLASH_SECTOR+51u]=0x5Au;
 before=erases;
 CHECK(beetle_flash_claim_probe(rd,0)==BEETLE_CLAIM_FOREIGN);
 CHECK(track_journal_save(&io,0,&p,scratch,sizeof(scratch),0)==TRACK_JOURNAL_FOREIGN);
 CHECK(erases==before);
 memset(flash+at,0xff,TRACK_JOURNAL_BANK_BYTES);
 /* A valid TRKJ header does not excuse foreign trailing records. */
 mk(0);CHECK(track_journal_save(&io,0,&p,scratch,sizeof(scratch),0)==TRACK_JOURNAL_OK);
 flash[at+TRACK_JOURNAL_BANK_BYTES-7u]=0x01u;
 before=erases;
 CHECK(beetle_flash_claim_probe(rd,0)==BEETLE_CLAIM_FOREIGN);
 CHECK(track_journal_save(&io,0,&p,scratch,sizeof(scratch),0)==TRACK_JOURNAL_FOREIGN);
 CHECK(erases==before);
 memset(flash+at+TRACK_JOURNAL_BANK_BYTES-7u,0xff,1);
 /* A sole uncommitted record cannot be overwritten without a verified
  * sister bank: data may be the only recoverable surviving project. */
 uint32_t a0=BEETLE_FLASH_BASE;
 uint8_t saved_h[TRACK_JOURNAL_HEADER_BYTES];
 memcpy(saved_h,flash+a0,TRACK_JOURNAL_HEADER_BYTES);
 memset(flash+a0,0xff,TRACK_JOURNAL_BANK_BYTES);
 memset(flash+at,0xff,TRACK_JOURNAL_BANK_BYTES);
 memcpy(flash+a0,saved_h,sizeof(saved_h));flash[a0+20]=0xffu;
 before=erases;
 CHECK(track_journal_save(&io,0,&p,scratch,sizeof(scratch),0)==TRACK_JOURNAL_FOREIGN);
 CHECK(erases==before);
 /* Restore the slot to a known-good blank state for final foreign test. */
 memset(flash+a0,0xff,TRACK_JOURNAL_BANK_BYTES);
 CHECK(beetle_flash_claim_probe(rd,0)==BEETLE_CLAIM_EXISTING);
 /* Foreign records are never interpreted by the BEETLE journal. */
 memcpy(flash+at,"ETCH",4);
 CHECK(beetle_flash_claim_probe(rd,0)==BEETLE_CLAIM_FOREIGN);
 before=erases;mk(0);
 CHECK(track_journal_save(&io,0,&p,scratch,sizeof(scratch),0)==TRACK_JOURNAL_FOREIGN);
 CHECK(erases==before);
 CHECK(beetle_flash_map(0,0,20470u,20u,&at)<0);
 CHECK(beetle_flash_map(3,1,20479u,2u,&at)<0);
 if(fail_count)return 1;
 printf("PASS: four BEETLE projects, no imported ETCH/Felucca records, 40-sector exact partition, A/B restore, FSMP and foreign refusal, untouched boot/OTA/globals, %u writes / %u erase ops\n",programs,erases);
 return 0;
}
