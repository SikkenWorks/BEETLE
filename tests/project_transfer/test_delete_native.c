/* BEETLE V0.24 - physical address policy + BEETLE-only record isolation.
 * Host simulated 1MiB SPI NOR, no device writes. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "beetle_flash_layout.h"
#include "track_journal.h"
#include "track_master_fx.h"
#include "beetle_instrument_library.h"
static unsigned fail_count,programs,erases;
static int fail_program_after=-1;
static int fail_erase_slot=-1;
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);fail_count++;}}while(0)
static uint8_t flash[0x100000],scratch[TRACK_STORE_MAX];
static struct track_store_project p,out;
static int rd(void *ctx,uint32_t a,uint8_t *dst,uint32_t n){(void)ctx;if(a>sizeof(flash)||n>sizeof(flash)-a)return -1;memcpy(dst,flash+a,n);return 0;}
static int rd_bank(void *c,uint8_t s,uint8_t b,uint32_t o,uint8_t *dst,uint32_t n){uint32_t a;return beetle_flash_map(s,b,o,n,&a)?-1:rd(c,a,dst,n);}
static int er_bank(void *c,uint8_t s,uint8_t b){(void)c;uint32_t a;
 if(beetle_flash_map(s,b,0,TRACK_JOURNAL_BANK_BYTES,&a))return -1;
 if(s==(uint8_t)fail_erase_slot)return -1;
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
static void mk(uint8_t slot){struct track_engine e;track_init(&e);memset(&p,0,sizeof(p));track_song_init(&p.song,&e);track_mixer_init(&p.mixer);track_master_default(&p.master);beetle_library_init(&p.library);
 for(uint8_t k=0;k<TRACK_DRUM_COUNT;k++)track_drum_timbre_default(&p.drum[k]);
 p.slot=slot;p.bpm=121u+slot;p.harmony.root=0u;p.harmony.mode=0u;
 memcpy(p.title,"BEETLE ONLY",11u);
 for(unsigned i=0;i<4;i++){p.instruments[i].type=(uint8_t)i;p.instruments[i].volume=65;p.instruments[i].duty=50;p.instruments[i].cutoff=7000;track_va_patch_default(&p.instruments[i].va);}
 p.song.patterns[47].ch[3][15].note=2;
}
int main(void){
 memset(flash,0xff,sizeof(flash));
 for(uint8_t s=0;s<12u;s++){
  if(s==2u||s==7u)continue;
  mk(s);int q=track_journal_save(&io,s,&p,scratch,sizeof(scratch),0);if(q)fprintf(stderr,"save slot %u rc=%d\n",s,q);CHECK(q==TRACK_JOURNAL_OK);
 }
 /* RC1.0.4: marker cannot be restored on faith alone; the journal must
  * independently prove ownership across every A/B bank. */
 CHECK(track_journal_audit_owned_volume(&io)==TRACK_JOURNAL_OK);
 CHECK(beetle_flash_marker_repairable(rd,0)==1); /* fully blank marker sector */
 uint8_t owner_marker[32];beetle_flash_make_marker(owner_marker);
 memcpy(flash+BEETLE_FLASH_MARKER,owner_marker,sizeof(owner_marker));
 CHECK(beetle_flash_claim_probe(rd,0)==BEETLE_CLAIM_EXISTING);
 CHECK(beetle_flash_marker_repairable(rd,0)==0);
 flash[BEETLE_FLASH_MARKER+24u]^=1u; /* damage CRC, not BEETLE identity */
 CHECK(beetle_flash_claim_probe(rd,0)==BEETLE_CLAIM_FOREIGN);
 CHECK(beetle_flash_marker_repairable(rd,0)==1);
 flash[BEETLE_FLASH_MARKER+4u]^=1u; /* destroyed layout/signature = unsafe */
 CHECK(beetle_flash_marker_repairable(rd,0)==0);
 flash[BEETLE_FLASH_MARKER+4u]^=1u;
 flash[BEETLE_FLASH_MARKER+100u]=0x01u; /* foreign marker-sector tail */
 CHECK(beetle_flash_marker_repairable(rd,0)==0);
 flash[BEETLE_FLASH_MARKER+100u]=0xFFu;
 memcpy(flash+BEETLE_FLASH_MARKER,owner_marker,sizeof(owner_marker));
 CHECK(beetle_flash_claim_probe(rd,0)==BEETLE_CLAIM_EXISTING);
 /* Both banks in slot 0 contain valid project versions; delete should erase both. */
 mk(0u);p.bpm=188u;CHECK(track_journal_save(&io,0u,&p,scratch,sizeof(scratch),0)==TRACK_JOURNAL_OK);
 uint8_t untouched[TRACK_JOURNAL_BANK_BYTES*2u];
 uint32_t slot1_addr;CHECK(!beetle_flash_map(1u,0u,0u,TRACK_JOURNAL_BANK_BYTES,&slot1_addr));
 memcpy(untouched,flash+slot1_addr,sizeof(untouched));
 uint8_t old_marker[32];memcpy(old_marker,flash+0xDF000u,sizeof(old_marker));
 unsigned before=erases;
 CHECK(track_journal_delete(&io,0u)==TRACK_JOURNAL_OK);
 CHECK(erases==before+6u); /* 2 banks x 3 sectors */
 CHECK(track_journal_load(&io,0u,&out,scratch,sizeof(scratch),0)==TRACK_JOURNAL_NO_PROJECT);
 char title[13];CHECK(track_journal_preview(&io,0u,title)==TRACK_JOURNAL_NO_PROJECT);
 uint32_t slot0_addr;CHECK(!beetle_flash_map(0u,0u,0u,TRACK_JOURNAL_BANK_BYTES,&slot0_addr));
 for(uint32_t n=0u;n<TRACK_JOURNAL_BANK_BYTES*2u;n++)CHECK(flash[slot0_addr+n]==0xFFu);
 CHECK(memcmp(untouched,flash+slot1_addr,sizeof(untouched))==0);
 CHECK(memcmp(old_marker,flash+0xDF000u,sizeof(old_marker))==0);
 CHECK(track_journal_load(&io,1u,&out,scratch,sizeof(scratch),0)==TRACK_JOURNAL_OK);
 CHECK(track_journal_delete(&io,0u)==TRACK_JOURNAL_NO_PROJECT);
 /* Reject corrupt/foreign bank in chosen slot BEFORE ANY ERASE. */
 mk(2);CHECK(track_journal_save(&io,2u,&p,scratch,sizeof(scratch),0)==TRACK_JOURNAL_OK);
 uint32_t badaddr;CHECK(!beetle_flash_map(2u,1u,0u,TRACK_JOURNAL_BANK_BYTES,&badaddr));
 flash[badaddr+800u]=1u;before=erases;
 CHECK(track_journal_delete(&io,2u)==TRACK_JOURNAL_FOREIGN);
 CHECK(erases==before);
 CHECK(track_journal_load(&io,1u,&out,scratch,sizeof(scratch),0)==TRACK_JOURNAL_OK);
 /* Inject erase failure; no false empty success and other slot remains intact. */
 fail_erase_slot=3u;
 CHECK(track_journal_delete(&io,3u)==TRACK_JOURNAL_IO);
 fail_erase_slot=-1;
 CHECK(track_journal_load(&io,3u,&out,scratch,sizeof(scratch),0)==TRACK_JOURNAL_OK);
 /* Later slot deletion must not disturb slot 1. */
 CHECK(track_journal_delete(&io,11u)==TRACK_JOURNAL_OK);
 CHECK(track_journal_preview(&io,11u,title)==TRACK_JOURNAL_NO_PROJECT);
 CHECK(track_journal_load(&io,1u,&out,scratch,sizeof(scratch),0)==TRACK_JOURNAL_OK);
 /* RC1.02: CRC-damaged BEETLE bank is recoverable by EXPLICIT command
  * without allowing regular deletion/overwrite or any foreign erase. */
 uint32_t ownedaddr;CHECK(!beetle_flash_map(6u,0u,0u,TRACK_JOURNAL_BANK_BYTES,&ownedaddr));
 flash[ownedaddr+TRACK_JOURNAL_HEADER_BYTES+51u]^=0x01u;
 before=erases;
 CHECK(track_journal_load(&io,6u,&out,scratch,sizeof(scratch),0)==TRACK_JOURNAL_FOREIGN);
 CHECK(track_journal_delete(&io,6u)==TRACK_JOURNAL_FOREIGN);
 CHECK(erases==before);
 CHECK(track_journal_delete_corrupt(&io,6u)==TRACK_JOURNAL_OK);
 CHECK(erases==before+3u);
 CHECK(track_journal_load(&io,6u,&out,scratch,sizeof(scratch),0)==TRACK_JOURNAL_NO_PROJECT);
 CHECK(track_journal_preview(&io,6u,title)==TRACK_JOURNAL_NO_PROJECT);
 CHECK(track_journal_load(&io,1u,&out,scratch,sizeof(scratch),0)==TRACK_JOURNAL_OK);
 /* Header CRC corruption with intact BEETLE slot signature is
  * positively owned; do not mistake it for arbitrary foreign material. */
 uint32_t hdraddr;CHECK(!beetle_flash_map(5u,0u,0u,TRACK_JOURNAL_BANK_BYTES,&hdraddr));
 flash[hdraddr+24u]^=0x01u;
 CHECK(track_journal_delete(&io,5u)==TRACK_JOURNAL_FOREIGN);
 CHECK(track_journal_delete_corrupt(&io,5u)==TRACK_JOURNAL_OK);
 CHECK(track_journal_preview(&io,5u,title)==TRACK_JOURNAL_NO_PROJECT);
 /* Corrupt secondary bank with good primary: ordinary LOAD can recover,
  * explicit recovery still asks to erase BOTH copies. */
 mk(8u);p.bpm=192u;CHECK(track_journal_save(&io,8u,&p,scratch,sizeof(scratch),0)==TRACK_JOURNAL_OK);
 uint32_t secondaddr;CHECK(!beetle_flash_map(8u,1u,0u,TRACK_JOURNAL_BANK_BYTES,&secondaddr));
 flash[secondaddr+TRACK_JOURNAL_HEADER_BYTES+51u]^=0x01u;
 CHECK(track_journal_load(&io,8u,&out,scratch,sizeof(scratch),0)==TRACK_JOURNAL_OK);
 CHECK(track_journal_delete(&io,8u)==TRACK_JOURNAL_FOREIGN);
 CHECK(track_journal_delete_corrupt(&io,8u)==TRACK_JOURNAL_OK);
 CHECK(track_journal_load(&io,8u,&out,scratch,sizeof(scratch),0)==TRACK_JOURNAL_NO_PROJECT);
 /* Corrupt own + foreign sibling MUST refuse even explicit recovery. */
 CHECK(track_journal_delete_corrupt(&io,2u)==TRACK_JOURNAL_FOREIGN);
 CHECK(track_journal_audit_owned_volume(&io)==TRACK_JOURNAL_FOREIGN);
 /* Do not use the recovery opcode to delete an ordinary valid project. */
 CHECK(track_journal_delete_corrupt(&io,4u)==TRACK_JOURNAL_INVALID);
 CHECK(track_journal_load(&io,4u,&out,scratch,sizeof(scratch),0)==TRACK_JOURNAL_OK);
 /* A fully missing slot remains missing, not a false recovery. */
 CHECK(track_journal_delete_corrupt(&io,6u)==TRACK_JOURNAL_NO_PROJECT);
 if(fail_count)return 1;
 printf("PASS true DELETE + explicit corrupt recovery: unrelated slots intact, foreign rejected, error-safe\n");return 0;
}
