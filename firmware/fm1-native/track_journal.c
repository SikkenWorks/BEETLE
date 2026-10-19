/* Transactional TRACK-only journal, GPL-3.0-only. No physical flash addresses. */
#include "track_journal.h"
#include <string.h>
#if !defined(TRACK_FIRMWARE) || BEETLE_NATIVE_STORAGE
static void wr16(uint8_t *b,uint16_t v){b[0]=(uint8_t)v;b[1]=(uint8_t)(v>>8);}
static void wr32(uint8_t *b,uint32_t v){for(uint8_t i=0;i<4u;i++)b[i]=(uint8_t)(v>>(i*8u));}
#endif
static uint16_t rd16(const uint8_t *b){return (uint16_t)(b[0]|((uint16_t)b[1]<<8));}
static uint32_t rd32(const uint8_t *b){return (uint32_t)b[0]|((uint32_t)b[1]<<8)|((uint32_t)b[2]<<16)|((uint32_t)b[3]<<24);}
static int valid_io(const struct track_journal_io *i,uint8_t s){return i&&s<TRACK_JOURNAL_SLOTS&&i->read&&i->erase&&i->program;}
struct bank_status {uint8_t kind;uint32_t generation,length,crc;};
/* Kind: 0 blank, 1 committed valid, 2 owned incomplete/corrupt, 3 foreign. */
static int inspect(const struct track_journal_io *i,uint8_t slot,uint8_t bank,struct bank_status *s){
 uint8_t h[TRACK_JOURNAL_HEADER_BYTES];memset(s,0,sizeof(*s));
 if(i->read(i->ctx,slot,bank,0,h,sizeof(h)))return TRACK_JOURNAL_IO;
 uint8_t blank=1;
 for(uint32_t x=0;x<sizeof(h);x++)if(h[x]!=0xFFu){blank=0;break;}
 /* An erased header is NOT enough: all five physical 4KiB sectors
  * must be FF before a bank may be selected for an erase/write.  A power
  * interruption can leave a bank looking blank at offset zero. */
 uint8_t chunk[256];
 if(blank){
  for(uint32_t pos=TRACK_JOURNAL_HEADER_BYTES;pos<TRACK_JOURNAL_BANK_BYTES;){
   uint32_t len=TRACK_JOURNAL_BANK_BYTES-pos;if(len>sizeof(chunk))len=sizeof(chunk);
   if(i->read(i->ctx,slot,bank,pos,chunk,len))return TRACK_JOURNAL_IO;
   for(uint32_t j=0;j<len;j++)if(chunk[j]!=0xFFu){s->kind=3;return 0;}
   pos+=len;
  }
  return 0;
 }
 /* An intact BEETLE magic, format, exact slot and inverse-slot pair
  * establish ownership even if a torn write damaged the header CRC.
  * A CRC-damaged header is NEVER considered committed, nor reusable by
  * ordinary SAVE/DELETE. The explicit recovery opcode may remove it after
  * a full bank/tail scan. Anything with an unknown owner stays foreign. */
 if(rd32(h)!=TRACK_JOURNAL_MAGIC||rd16(h+4u)!=1u||h[6]!=slot||
    h[7]!=(uint8_t)(slot^0xFFu)||
    (h[20]!=0x00u&&h[20]!=0xFFu)){
  s->kind=3;return 0;
 }
 s->generation=rd32(h+8u);s->length=rd32(h+12u);s->crc=rd32(h+16u);
 if(s->length<42u||s->length>TRACK_STORE_MAX||
    TRACK_JOURNAL_HEADER_BYTES+s->length>TRACK_JOURNAL_BANK_BYTES){
  s->kind=3;return 0;
 }
 /* A valid header may be an unfinished write; only a verified sister
  * permits replacing such a bank. Unexpected bytes beyond the declared
  * payload indicate foreign occupancy and prohibit any erase. */
 for(uint32_t pos=TRACK_JOURNAL_HEADER_BYTES+s->length;pos<TRACK_JOURNAL_BANK_BYTES;){
  uint32_t len=TRACK_JOURNAL_BANK_BYTES-pos;if(len>sizeof(chunk))len=sizeof(chunk);
  if(i->read(i->ctx,slot,bank,pos,chunk,len))return TRACK_JOURNAL_IO;
  for(uint32_t j=0;j<len;j++)if(chunk[j]!=0xFFu){s->kind=3;return 0;}
  pos+=len;
 }
 s->kind=2;
 if(h[20]!=0u||rd32(h+24u)!=track_store_crc32(h,20u))return 0;
 /* V0.75: the outer CRC alone does not mean an old bank contains a valid
  * TRACK project. If the inner TRK1 header is bad but the outer CRC is
  * intact, treating this as "committed" could erase the sole decodable
  * sister bank on the next SAVE. Validate the inner header and its payload
  * CRC while streaming, without allocating a second project buffer. */
 uint32_t crc=0xFFFFFFFFu,body_crc=0xFFFFFFFFu;
 uint8_t prefix[24]={0};
 for(uint32_t at=0;at<s->length;){
  uint32_t n=s->length-at;if(n>sizeof(chunk))n=sizeof(chunk);
  if(i->read(i->ctx,slot,bank,TRACK_JOURNAL_HEADER_BYTES+at,chunk,n))return TRACK_JOURNAL_IO;
  if(at==0u)memcpy(prefix,chunk,sizeof(prefix));
  for(uint32_t j=0;j<n;j++){
   const uint8_t value=chunk[j];
   crc^=value;for(uint8_t k=0;k<8u;k++)crc=(crc>>1)^((crc&1u)?0xEDB88320u:0u);
   if(at+j>=24u){
    body_crc^=value;
    for(uint8_t k=0;k<8u;k++)body_crc=(body_crc>>1)^((body_crc&1u)?0xEDB88320u:0u);
   }
  }
  at+=n;
 }
 if(~crc==s->crc && rd32(prefix)==TRACK_STORE_MAGIC &&
    rd16(prefix+4u)>=1u && rd16(prefix+4u)<=TRACK_STORE_VERSION &&
    rd16(prefix+6u)==slot && rd32(prefix+8u)==s->length &&
    rd32(prefix+12u)==~body_crc &&
    rd32(prefix+16u)==track_store_crc32(prefix,16u))s->kind=1;
 return 0;
}
static int newer(uint32_t a,uint32_t b){return (int32_t)(a-b)>0;}
static int choose(const struct bank_status b[2],uint8_t *bank){
 if(b[0].kind==3||b[1].kind==3)return TRACK_JOURNAL_FOREIGN;
 if(b[0].kind==1&&b[1].kind==1){*bank=(uint8_t)(newer(b[1].generation,b[0].generation)?1:0);return 1;}
 if(b[0].kind==1){*bank=0;return 1;}
 if(b[1].kind==1){*bank=1;return 1;}
 /* Never erase the sole surviving uncommitted/corrupt record in a slot.
  * Refuse destructive recovery unless another committed bank survives. */
 if(b[0].kind==2||b[1].kind==2)return TRACK_JOURNAL_FOREIGN;
 return 0;
}
/* V0.49: constant-time project picker. Full journal inspection scans up to
 * 24 KiB per slot (including erased tails), which stalls encoder polling.
 * The browser needs only a title; every actual LOAD and SAVE continues to
 * use the original full CRC/ownership and foreign-sector safety checks. */
int track_journal_preview(const struct track_journal_io *io,uint8_t slot,char title[13]){
 if(!valid_io(io,slot)||!title)return TRACK_JOURNAL_INVALID;
 title[0]=0;
 uint8_t found=0u,blank_count=0u;
 uint32_t generation=0u;
 for(uint8_t bank=0u;bank<2u;bank++){
  uint8_t h[TRACK_JOURNAL_HEADER_BYTES];
  if(io->read(io->ctx,slot,bank,0u,h,sizeof(h)))return TRACK_JOURNAL_IO;
  uint8_t blank=1u;
  for(uint8_t j=0u;j<sizeof(h);j++)if(h[j]!=0xFFu){blank=0u;break;}
  if(blank){blank_count++;continue;}
  uint32_t size=rd32(h+12u);
  if(rd32(h)!=TRACK_JOURNAL_MAGIC||rd16(h+4u)!=1u||
     h[6]!=slot||h[7]!=(uint8_t)(slot^0xFFu)||h[20]!=0u||
     rd32(h+24u)!=track_store_crc32(h,20u)||size<42u||
     size>TRACK_STORE_MAX||size+TRACK_JOURNAL_HEADER_BYTES>TRACK_JOURNAL_BANK_BYTES)continue;
  uint8_t prefix[36];
  if(io->read(io->ctx,slot,bank,TRACK_JOURNAL_HEADER_BYTES,prefix,sizeof(prefix)))return TRACK_JOURNAL_IO;
  uint16_t version=rd16(prefix+4u);
  if(rd32(prefix)!=TRACK_STORE_MAGIC||version<1u||version>TRACK_STORE_VERSION||
     rd16(prefix+6u)!=slot||rd32(prefix+8u)!=size||
     rd32(prefix+16u)!=track_store_crc32(prefix,16u))continue;
  uint32_t gen=rd32(h+8u);
  if(found && !newer(gen,generation))continue;
  found=1u;generation=gen;
  for(uint8_t j=0u;j<12u;j++){
   uint8_t ch=prefix[24u+j];
   title[j]=(char)(ch>=32u&&ch<=126u?ch:' ');
  }
  title[12]=0;
 }
 if(found)return TRACK_JOURNAL_OK;
 /* An FF header does not prove the underlying bank is empty. This preview
  * is never trusted for writing: save/load perform comprehensive scans. */
 return blank_count==2u?TRACK_JOURNAL_NO_PROJECT:TRACK_JOURNAL_INVALID;
}
int track_journal_load(const struct track_journal_io *i,uint8_t slot,
 struct track_store_project *out,uint8_t *scratch,uint32_t cap,
 struct track_journal_status *status){
 /* The physical FM1 workspace is TRACK_STORE_V4_MAX (12,256 bytes), not
  * legacy TRACK_STORE_MAX (24,576 bytes).  No physical bank can contain
  * more than TRACK_JOURNAL_BANK_BYTES - HEADER_BYTES bytes. */
 if(!valid_io(i,slot)||!out||!scratch||cap<TRACK_STORE_V4_MAX)return TRACK_JOURNAL_INVALID;
 struct bank_status b[2];int r;
 for(uint8_t k=0;k<2;k++)if((r=inspect(i,slot,k,&b[k]))!=0)return r;
 uint8_t sel=0;r=choose(b,&sel);if(r<0)return r;if(r==0)return TRACK_JOURNAL_NO_PROJECT;
 for(uint8_t attempt=0;attempt<2u;attempt++){
  uint8_t which=(uint8_t)(attempt?(sel^1u):sel);
  if(b[which].kind!=1)continue;
  if(b[which].length>cap)return TRACK_JOURNAL_CAPACITY;
  if(i->read(i->ctx,slot,which,TRACK_JOURNAL_HEADER_BYTES,scratch,b[which].length))return TRACK_JOURNAL_IO;
  if(track_store_decode(out,scratch,b[which].length)==0&&out->slot==slot){
   if(status)*status=(struct track_journal_status){b[which].generation,b[which].length,which,1};
   return TRACK_JOURNAL_OK;
  }
 }
 return TRACK_JOURNAL_INVALID;
}
/* Physical deletion of ONE project slot only. A/B journal records are
 * verified by the full scanner BEFORE either bank can be erased.
 * Erase the non-current bank first so a power cut there retains the newest
 * valid project. If power fails while erasing the final bank, the slot may
 * be unreadable; no false success is reported. */
int track_journal_delete(const struct track_journal_io *i,uint8_t slot){
#if defined(TRACK_FIRMWARE) && !BEETLE_NATIVE_STORAGE
 (void)i;(void)slot;return TRACK_JOURNAL_HARDWARE_LOCKED;
#else
 if(!valid_io(i,slot))return TRACK_JOURNAL_INVALID;
 struct bank_status b[2];int r;
 for(uint8_t k=0;k<2u;k++)if((r=inspect(i,slot,k,&b[k]))!=0)return r;
 uint8_t active=0u;r=choose(b,&active);
 if(r<0)return r;
 if(r==0)return TRACK_JOURNAL_NO_PROJECT;
 /* Never delete an ambiguous, incomplete, or foreign bank. */
 if((b[0].kind!=0u&&b[0].kind!=1u) ||
    (b[1].kind!=0u&&b[1].kind!=1u))return TRACK_JOURNAL_FOREIGN;
 const uint8_t order[2]={(uint8_t)(active^1u),active};
 for(uint8_t n=0;n<2u;n++){
  const uint8_t bank=order[n];
  if(b[bank].kind==0u)continue;
  if(i->erase(i->ctx,slot,bank))return TRACK_JOURNAL_IO;
  struct bank_status verify;
  if((r=inspect(i,slot,bank,&verify))!=0)return r;
  if(verify.kind!=0u)return TRACK_JOURNAL_IO;
 }
 /* Verify both entire banks, not only their headers. */
 for(uint8_t k=0;k<2u;k++){
  struct bank_status verify;
  if((r=inspect(i,slot,k,&verify))!=0)return r;
  if(verify.kind!=0u)return TRACK_JOURNAL_IO;
 }
 return TRACK_JOURNAL_OK;
#endif
}
/* Explicit RC1.02 recovery for [READ ERROR] banks. Unlike regular DELETE,
 * the caller must confirm data loss and own the BEETLE volume. Only banks
 * whose FULL header/sector inspection proves BEETLE ownership (kind 1 or 2)
 * are eligible. A foreign bank (kind 3) is never erased, even if its sibling
 * is valid BEETLE data. This is deliberately NOT a general flash formatter. */
int track_journal_audit_owned_volume(const struct track_journal_io *i){
 if(!valid_io(i,0u))return TRACK_JOURNAL_INVALID;
 uint8_t owned=0u;
 for(uint8_t slot=0u;slot<TRACK_JOURNAL_SLOTS;slot++){
  for(uint8_t bank=0u;bank<2u;bank++){
   struct bank_status state;
   int rc=inspect(i,slot,bank,&state);
   if(rc)return rc;
   if(state.kind==3u)return TRACK_JOURNAL_FOREIGN;
   if(state.kind==1u||state.kind==2u)owned=1u;
  }
 }
 return owned?TRACK_JOURNAL_OK:TRACK_JOURNAL_NO_PROJECT;
}
int track_journal_delete_corrupt(const struct track_journal_io *i,uint8_t slot){
#if defined(TRACK_FIRMWARE) && !BEETLE_NATIVE_STORAGE
 (void)i;(void)slot;return TRACK_JOURNAL_HARDWARE_LOCKED;
#else
 if(!valid_io(i,slot))return TRACK_JOURNAL_INVALID;
 struct bank_status b[2];int r;
 for(uint8_t bank=0u;bank<2u;bank++)if((r=inspect(i,slot,bank,&b[bank]))!=0)return r;
 if(b[0].kind==3u||b[1].kind==3u)return TRACK_JOURNAL_FOREIGN;
 if(b[0].kind==0u&&b[1].kind==0u)return TRACK_JOURNAL_NO_PROJECT;
 /* Require an owned damaged bank; otherwise use ordinary DELETE. */
 if(b[0].kind!=2u&&b[1].kind!=2u)return TRACK_JOURNAL_INVALID;
 /* Damaged banks FIRST; surviving valid copies LAST. */
 for(uint8_t pass=0u;pass<2u;pass++)for(uint8_t bank=0u;bank<2u;bank++){
  const uint8_t kind=b[bank].kind;
  if((pass==0u&&kind!=2u)||(pass==1u&&kind!=1u))continue;
  if(i->erase(i->ctx,slot,bank))return TRACK_JOURNAL_IO;
  struct bank_status verified;
  if((r=inspect(i,slot,bank,&verified))!=0)return r;
  if(verified.kind!=0u)return TRACK_JOURNAL_IO;
 }
 for(uint8_t bank=0u;bank<2u;bank++){
  struct bank_status verified;
  if((r=inspect(i,slot,bank,&verified))!=0)return r;
  if(verified.kind!=0u)return TRACK_JOURNAL_IO;
 }
 return TRACK_JOURNAL_OK;
#endif
}
int track_journal_save(const struct track_journal_io *i,uint8_t slot,
 const struct track_store_project *p,uint8_t *scratch,uint32_t cap,
 struct track_journal_status *status){
 /* Hard lock: host tests may exercise a simulated backend, the physical
  * TRACK image must NOT gain flash writes merely by linking this module. */
#if defined(TRACK_FIRMWARE) && !BEETLE_NATIVE_STORAGE
 (void)i;(void)slot;(void)p;(void)scratch;(void)cap;(void)status;
 return TRACK_JOURNAL_HARDWARE_LOCKED;
#else
 if(!valid_io(i,slot)||!p||!scratch||cap<TRACK_STORE_V4_MAX||p->slot!=slot)return TRACK_JOURNAL_INVALID;
 /* Pass the TRUE buffer capacity to the encoder. Passing TRACK_STORE_MAX
  * here could silently overwrite adjacent FM1 RAM in a dense project. */
 uint32_t n=0;if(track_store_encode(p,scratch,cap,&n)!=0)return TRACK_JOURNAL_CAPACITY;
 if(TRACK_JOURNAL_HEADER_BYTES+n>TRACK_JOURNAL_BANK_BYTES)return TRACK_JOURNAL_CAPACITY;
 struct bank_status b[2];int r;
 for(uint8_t k=0;k<2u;k++)if((r=inspect(i,slot,k,&b[k]))!=0)return r;
 uint8_t active=0;r=choose(b,&active);if(r<0)return r;
 /* Wear protection: saving an unchanged project does not consume NOR
  * erase cycles. Compare every byte, not only the CRC, before returning. */
 if(r==1 && b[active].length==n && b[active].crc==track_store_crc32(scratch,n)){
  uint8_t chunk[256];uint32_t off=0;uint8_t identical=1u;
  while(off<n){
   uint32_t count=n-off;if(count>sizeof(chunk))count=sizeof(chunk);
   if(i->read(i->ctx,slot,active,TRACK_JOURNAL_HEADER_BYTES+off,chunk,count))return TRACK_JOURNAL_IO;
   if(memcmp(chunk,scratch+off,count)){identical=0u;break;}
   off+=count;
  }
  if(identical){
   if(status)*status=(struct track_journal_status){b[active].generation,n,active,1};
   return TRACK_JOURNAL_OK;
  }
 }
 uint8_t target=(uint8_t)(r==1?(active^1u):0);
 uint32_t generation=(r==1?b[active].generation+1u:1u);
 /* Erase only a logically TRACK-owned or blank bank; refuse all foreign data. */
 if(b[target].kind==3)return TRACK_JOURNAL_FOREIGN;
 if(i->erase(i->ctx,slot,target))return TRACK_JOURNAL_IO;
 uint8_t h[TRACK_JOURNAL_HEADER_BYTES];memset(h,0xFF,sizeof(h));
 wr32(h,TRACK_JOURNAL_MAGIC);wr16(h+4u,1u);h[6]=slot;h[7]=(uint8_t)(slot^0xFFu);
 wr32(h+8u,generation);wr32(h+12u,n);wr32(h+16u,track_store_crc32(scratch,n));
 wr32(h+24u,track_store_crc32(h,20u));
 if(i->program(i->ctx,slot,target,0u,h,sizeof(h)))return TRACK_JOURNAL_IO;
 /* Programs are aligned to chunks; previously valid bank remains untouched. */
 for(uint32_t at=0;at<n;){
  uint32_t abs=TRACK_JOURNAL_HEADER_BYTES+at;
  uint32_t len=256u-(abs&255u); /* Keep each write within a NOR page. */
  if(len>n-at)len=n-at;
  if(i->program(i->ctx,slot,target,abs,scratch+at,len))return TRACK_JOURNAL_IO;
  at+=len;
 }
 /* Verify with the same read-only scanner before committing; pending is expected. */
 uint8_t check[256];uint32_t pos=0;
 for(;pos<n;){uint32_t len=n-pos;if(len>sizeof(check))len=sizeof(check);
  if(i->read(i->ctx,slot,target,TRACK_JOURNAL_HEADER_BYTES+pos,check,len)||
     memcmp(check,scratch+pos,len)!=0)return TRACK_JOURNAL_IO;
  pos+=len;
 }
 const uint8_t commit=0u;
 if(i->program(i->ctx,slot,target,20u,&commit,1u))return TRACK_JOURNAL_IO;
 struct bank_status verify;
 if((r=inspect(i,slot,target,&verify))!=0)return r;
 if(verify.kind!=1||verify.generation!=generation)return TRACK_JOURNAL_IO;
 if(status)*status=(struct track_journal_status){generation,n,target,1};
 return TRACK_JOURNAL_OK;
#endif
}
