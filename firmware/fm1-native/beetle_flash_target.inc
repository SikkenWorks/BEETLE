/* SPDX-License-Identifier: GPL-3.0-only
 * BEETLE standalone V0.34 physical flash backend. Included by bringup.c after
 * fm1_flash.h via wave_store_target.c. All flash operations are outside audio.
 * No writes happen until a manual hold-to-format, or in an owned TRKJ journal.
 */
#include "beetle_flash_layout.h"
#if !defined(TRACK_FIRMWARE) || !BEETLE_NATIVE_STORAGE || ETCH_NATIVE_STORAGE
#error "Standalone BEETLE storage cannot be linked into an ETCH storage build"
#endif
static uint8_t beetle_write_permit;
static int beetle_storage_last_audit=BEETLE_CLAIM_INVALID;
static int beetle_hal_abs_read(void *ctx,uint32_t off,uint8_t *dst,uint32_t n){
 (void)ctx;
 if(!dst||!n||!((FL_IN(off,n,BEETLE_FLASH_BASE,BEETLE_FLASH_END))||
                   FL_IN(off,n,BEETLE_FLASH_MARKER,BEETLE_FLASH_MARKER_END)))return -1;
 while(n){
  uint32_t k=n>256u?256u:n;
  fm1_irq_off();int rc=FL_FAR(fl_read_ram)(off,dst,k);fm1_irq_enable_all();
  fm1_wdt_feed();if(rc)return rc;
  off+=k;dst+=k;n-=k;
 }
 return 0;
}
static int beetle_hal_probe_ownership(void){
 int rc=beetle_flash_claim_probe(beetle_hal_abs_read,0);
 beetle_storage_last_audit=rc;
 return rc;
}
static int beetle_hal_journal_read(void *ctx,uint8_t slot,uint8_t bank,
 uint32_t offset,uint8_t *dst,uint32_t n){
 uint32_t addr;(void)ctx;
 if(beetle_flash_map(slot,bank,offset,n,&addr))return -1;
 return beetle_hal_abs_read(0,addr,dst,n);
}
static int beetle_hal_journal_erase(void *ctx,uint8_t slot,uint8_t bank){
 uint32_t addr;(void)ctx;
 if(!beetle_write_permit||beetle_hal_probe_ownership()!=BEETLE_CLAIM_EXISTING||
    beetle_flash_map(slot,bank,0u,TRACK_JOURNAL_BANK_BYTES,&addr))return -1;
 for(uint32_t at=0u;at<TRACK_JOURNAL_BANK_BYTES;at+=BEETLE_FLASH_SECTOR){
  uint32_t took=0u;
  fm1_irq_off();int rc=FL_FAR(fl_erase4k_ram)(addr+at,&took);fm1_irq_enable_all();
  fm1_wdt_feed();if(rc)return rc;
 }
 return 0;
}
static int beetle_hal_journal_program(void *ctx,uint8_t slot,uint8_t bank,
 uint32_t offset,const uint8_t *src,uint32_t n){
 uint32_t addr;(void)ctx;
 if(!src||!n||!beetle_write_permit||beetle_hal_probe_ownership()!=BEETLE_CLAIM_EXISTING||
    beetle_flash_map(slot,bank,offset,n,&addr))return -1;
 while(n){
  uint32_t k=256u-(addr&255u);if(k>n)k=n;
  uint32_t took=0u;
  fm1_irq_off();int rc=FL_FAR(fl_prog_ram)(addr,src,k,&took);fm1_irq_enable_all();
  fm1_wdt_feed();if(rc)return rc;
  addr+=k;src+=k;n-=k;
 }
 return 0;
}
static const struct track_journal_io beetle_hal_journal_io={
 0,beetle_hal_journal_read,beetle_hal_journal_erase,beetle_hal_journal_program
};
/* Destructive provisioning allowed ONLY from TRACK's explicit five-second
 * SELECT+SAVE consent screen. The mark is written LAST, so any interrupted
 * formatting stays unowned and can never accidentally enable journal writes.
 * This standalone partition DESTROYS any prior ETCH/Felucca user data in it.
 */
static int beetle_hal_init_exclusive(void (*progress)(uint8_t,uint8_t)){
 if(native_safe||native_tripped||native_updating)return -1;
 int prev=beetle_hal_probe_ownership();
 if(prev==BEETLE_CLAIM_EXISTING)return 0; /* never erase saved BEETLE files */
 if(prev==BEETLE_CLAIM_IO||prev==BEETLE_CLAIM_INVALID)return -1;
 /* Never touch boot header, app, 0xE0000 OTA, globals or 0xE5000 autosave. */
 const uint8_t total=(uint8_t)((BEETLE_FLASH_MARKER_END-BEETLE_FLASH_BASE)/BEETLE_FLASH_SECTOR);
 uint8_t done=0u;
 for(uint32_t off=BEETLE_FLASH_BASE;off<BEETLE_FLASH_MARKER_END;off+=BEETLE_FLASH_SECTOR){
  uint32_t took=0u;
  fm1_irq_off();int rc=FL_FAR(fl_erase4k_ram)(off,&took);fm1_irq_enable_all();
  fm1_wdt_feed();if(rc)return -2;
  uint8_t verify[256];
  for(uint32_t k=0;k<BEETLE_FLASH_SECTOR;k+=sizeof(verify)){
   if(beetle_hal_abs_read(0,off+k,verify,sizeof(verify)))return -3;
   for(uint32_t j=0;j<sizeof(verify);j++)if(verify[j]!=0xFFu)return -4;
  }
  ++done;
  if(progress&&(done==total || (done%4u)==0u))progress(done,total);
 }
 uint8_t marker[32];beetle_flash_make_marker(marker);
 uint32_t took=0u;
 fm1_irq_off();int rc=FL_FAR(fl_prog_ram)(BEETLE_FLASH_MARKER,marker,sizeof(marker),&took);fm1_irq_enable_all();
 fm1_wdt_feed();if(rc)return -5;
 return beetle_hal_probe_ownership()==BEETLE_CLAIM_EXISTING?0:-6;
}
