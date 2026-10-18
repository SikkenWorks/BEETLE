#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#define TRACK_FIRMWARE 1
#define BEETLE_NATIVE_STORAGE 1
#define ETCH_NATIVE_STORAGE 0
#define FL_IN(off,n,start,end) ((off)>=(start) && (off)<=(end) && (n)<=(end)-(off))
#define FL_FAR(f) f
#include "beetle_flash_layout.h"
static uint8_t nor[1024*1024];
static uint8_t native_safe,native_tripped,native_updating;
static unsigned erases,fail_erase_at,programs;
static void fm1_irq_off(void){}
static void fm1_irq_enable_all(void){}
static void fm1_wdt_feed(void){}
static int fl_read_ram(uint32_t address,uint8_t* dst,uint32_t n){if(n>sizeof(nor)-address)return -1;memcpy(dst,nor+address,n);return 0;}
static int fl_erase4k_ram(uint32_t address,uint32_t *took){
 if(address%4096||address>sizeof(nor)-4096)return -1;
 if((address<BEETLE_FLASH_BASE || address>=BEETLE_FLASH_MARKER_END))return -1;
 erases++;if(fail_erase_at && erases==fail_erase_at)return -1;
 memset(nor+address,0xFF,4096);*took=1;return 0;
}
static int fl_prog_ram(uint32_t address,const uint8_t *v,uint32_t n,uint32_t *took){
 if(n>sizeof(nor)-address)return -1;
 if(address<BEETLE_FLASH_BASE || address+n>BEETLE_FLASH_MARKER_END)return -1;
 programs++;for(uint32_t i=0;i<n;i++)nor[address+i]&=v[i];*took=1;return 0;
}
uint32_t track_store_crc32(const uint8_t *p,uint32_t n){uint32_t crc=~0u;for(uint32_t j=0;j<n;j++){crc^=p[j];for(int k=0;k<8;k++)crc=crc&1?(crc>>1)^0xEDB88320:crc>>1;}return ~crc;}
int track_journal_audit_owned_volume(const struct track_journal_io *io){(void)io;return TRACK_JOURNAL_FOREIGN;}
int track_journal_preview(const struct track_journal_io *io,uint8_t slot,char title[13]){
 (void)title;
 uint8_t test[128];for(int bank=0;bank<2;bank++)for(uint32_t off=0;off<TRACK_JOURNAL_BANK_BYTES;off+=128){
  if(io->read(io->ctx,slot,bank,off,test,128))return TRACK_JOURNAL_IO;
  for(int j=0;j<128;j++)if(test[j]!=0xFF)return TRACK_JOURNAL_FOREIGN;
 }
 return TRACK_JOURNAL_NO_PROJECT;
}
#include "beetle_flash_target.inc"
static void reset_flash(void){
 memset(nor,0xA5,sizeof(nor));
 memset(nor+BEETLE_FLASH_BASE,0xff,BEETLE_FLASH_MARKER_END-BEETLE_FLASH_BASE);
 uint8_t marker[32];beetle_flash_make_marker(marker);
 memcpy(nor+BEETLE_FLASH_MARKER,marker,32);
 nor[BEETLE_FLASH_BASE + (7u*2u+0u)*TRACK_JOURNAL_BANK_BYTES+40u]=0x55;
 nor[BEETLE_FLASH_BASE + (7u*2u+1u)*TRACK_JOURNAL_BANK_BYTES+20u]=0x29;
 erases=programs=fail_erase_at=0;beetle_write_permit=1;beetle_full_wipe_confirmed=1;
}
int main(void){
 const uint32_t total=(BEETLE_FLASH_MARKER_END-BEETLE_FLASH_BASE)/4096;
 uint8_t before[1024];
 reset_flash();memcpy(before,nor+BEETLE_FLASH_BASE-1024,1024);
 assert(beetle_hal_wipe_projects(0)==0);
 assert(erases==total && programs==1);
 assert(beetle_hal_probe_ownership()==BEETLE_CLAIM_EXISTING);
 assert(!memcmp(before,nor+BEETLE_FLASH_BASE-1024,1024));
 for(unsigned s=0;s<TRACK_PROJECTS;s++){
  char title[13];assert(track_journal_preview(&beetle_hal_journal_io,s,title)==TRACK_JOURNAL_NO_PROJECT);
 }
 for(unsigned j=BEETLE_FLASH_MARKER_END;j<sizeof(nor);j++)assert(nor[j]==0xA5);
 reset_flash();nor[BEETLE_FLASH_MARKER]=0x11;
 beetle_full_wipe_confirmed=0;
 assert(beetle_hal_wipe_projects(0)==-1 && erases==0 && programs==0);
 beetle_full_wipe_confirmed=1;
 assert(beetle_hal_wipe_projects(0)==0); // explicit takeover of corrupted owner marker
 reset_flash();beetle_write_permit=0;
 assert(beetle_hal_wipe_projects(0)==-1 && erases==0);
 reset_flash();fail_erase_at=3;
 assert(beetle_hal_wipe_projects(0)==-2 && erases==3);
 assert(beetle_hal_probe_ownership()==BEETLE_CLAIM_EXISTING); // marker written last
 fail_erase_at=0;assert(beetle_hal_wipe_projects(0)==0);
 puts("PASS: all 12 EMPTY; protected flash intact; double YES gate; explicit foreign-marker reinit; power-cut retry");
}
