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
/* Only the physical on-device double-YES PROJECTS flow may set this. */
static uint8_t beetle_full_wipe_confirmed;
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
/* Explicitly repair ONLY a missing/CRC-corrupt marker, NEVER a foreign one.
 * The ENTIRE A/B area must first be audited read-only for ownership. */
static int beetle_hal_marker_audit(void){
 int ownership=beetle_hal_probe_ownership();
 if(ownership==BEETLE_CLAIM_EXISTING)return 0; /* nothing to repair */
 if(ownership==BEETLE_CLAIM_IO)return -2;
 int marker=beetle_flash_marker_repairable(beetle_hal_abs_read,0);
 if(marker<0)return -2;
 if(marker!=1)return -1;
 int banks=track_journal_audit_owned_volume(&beetle_hal_journal_io);
 if(banks==TRACK_JOURNAL_IO)return -2;
 if(banks==TRACK_JOURNAL_NO_PROJECT)return -3;
 if(banks!=TRACK_JOURNAL_OK)return -1;
 return 1; /* repair is eligible, but still requires explicit MIDI consent */
}
static int beetle_hal_restore_marker(void){
 if(!beetle_write_permit||native_safe||native_tripped||native_updating)return -1;
 if(beetle_hal_marker_audit()!=1)return -1;
 uint32_t took=0u;
 fm1_irq_off();int rc=FL_FAR(fl_erase4k_ram)(BEETLE_FLASH_MARKER,&took);fm1_irq_enable_all();
 fm1_wdt_feed();if(rc)return -2;
 uint8_t block[256];
 for(uint32_t at=BEETLE_FLASH_MARKER;at<BEETLE_FLASH_MARKER_END;at+=sizeof(block)){
  if(beetle_hal_abs_read(0,at,block,sizeof(block)))return -2;
  for(uint16_t j=0;j<sizeof(block);j++)if(block[j]!=0xFFu)return -2;
 }
 uint8_t marker[32];beetle_flash_make_marker(marker);
 fm1_irq_off();rc=FL_FAR(fl_prog_ram)(BEETLE_FLASH_MARKER,marker,sizeof(marker),&took);fm1_irq_enable_all();
 fm1_wdt_feed();if(rc)return -2;
 return beetle_hal_probe_ownership()==BEETLE_CLAIM_EXISTING?0:-2;
}

/* RC1.0.5: explicitly consented factory reinitialization of the fixed
 * BEETLE PROJECT PARTITION, INCLUDING any older ETCH/Felucca user records
 * within that same range. NO firmware/header/OTA sectors are addressed.
 * A separate physical double-YES gesture is required even if BEETLE's
 * marker is absent or corrupt. This does NOT enable normal journal writes
 * to foreign/unknown data; only this whole-volume reinitialization can
 * erase it. Any flash read error or unknown runtime mode is rejected. */
static int beetle_hal_wipe_projects(void (*progress)(uint8_t,uint8_t)){
 if(!beetle_write_permit||!beetle_full_wipe_confirmed||native_safe||native_tripped||native_updating)return -1;
 int owner=beetle_hal_probe_ownership();
 if(owner==BEETLE_CLAIM_IO||owner==BEETLE_CLAIM_INVALID)return -1;
 /* FOREIGN and BLANK are allowed ONLY after two physical confirmations.
  * The region is exactly bounded by compile-time BEETLE_FLASH_* constants. */
 const uint8_t total=(uint8_t)((BEETLE_FLASH_MARKER_END-BEETLE_FLASH_BASE)/BEETLE_FLASH_SECTOR);
 const uint8_t verify_size=128u;
 uint8_t checked[128];
 uint8_t done=0u;
 /* Data sectors FIRST. If interrupted, intact ownership remains and user
  * can retry the wipe; no successful result is reported. Marker LAST. */
 for(uint32_t off=BEETLE_FLASH_BASE;off<BEETLE_FLASH_MARKER_END;off+=BEETLE_FLASH_SECTOR){
  uint32_t took=0u;
  fm1_irq_off();int rc=FL_FAR(fl_erase4k_ram)(off,&took);fm1_irq_enable_all();
  fm1_wdt_feed();if(rc)return -2;
  for(uint32_t k=0u;k<BEETLE_FLASH_SECTOR;k+=verify_size){
   if(beetle_hal_abs_read(0,off+k,checked,sizeof(checked)))return -3;
   for(uint8_t j=0u;j<verify_size;j++)if(checked[j]!=0xFFu)return -3;
  }
  done++;
  if(progress&&(done==total||(done%4u)==0u))progress(done,total);
 }
 uint8_t marker[32];beetle_flash_make_marker(marker);
 uint32_t took=0u;
 fm1_irq_off();int rc=FL_FAR(fl_prog_ram)(BEETLE_FLASH_MARKER,marker,sizeof(marker),&took);fm1_irq_enable_all();
 fm1_wdt_feed();if(rc)return -4;
 if(beetle_hal_probe_ownership()!=BEETLE_CLAIM_EXISTING)return -5;
 /* Read back every bank, including its erased tail, before reporting EMPTY. */
 for(uint8_t slot=0u;slot<TRACK_PROJECTS;slot++){
  char title[13];
  if(track_journal_preview(&beetle_hal_journal_io,slot,title)!=TRACK_JOURNAL_NO_PROJECT)return -5;
 }
 return 0;
}
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
