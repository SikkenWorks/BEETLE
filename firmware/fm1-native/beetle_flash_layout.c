/* SPDX-License-Identifier: GPL-3.0-only */
#include "beetle_flash_layout.h"
#include <string.h>
static void put32(uint8_t *p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(8u*i));}
static uint32_t get32(const uint8_t *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
int beetle_flash_map(uint8_t slot,uint8_t bank,uint32_t offset,uint32_t n,uint32_t *physical){
 if(!physical||slot>=TRACK_JOURNAL_SLOTS||bank>=2u||offset>TRACK_JOURNAL_BANK_BYTES||n>TRACK_JOURNAL_BANK_BYTES-offset)return -1;
 uint32_t base=BEETLE_FLASH_BASE+((uint32_t)slot*2u+(uint32_t)bank)*TRACK_JOURNAL_BANK_BYTES;
 if(base<BEETLE_FLASH_BASE||base>BEETLE_FLASH_END||TRACK_JOURNAL_BANK_BYTES>BEETLE_FLASH_END-base)return -1;
 *physical=base+offset;return 0;
}
void beetle_flash_make_marker(uint8_t out[32]){
 static const uint8_t magic[8]={'B','T','L','3','3','F','S','1'};
 memset(out,0xFF,32u);memcpy(out,magic,8u);
 put32(out+8u,BEETLE_FLASH_BASE);put32(out+12u,BEETLE_FLASH_END);
 put32(out+16u,TRACK_JOURNAL_BANK_BYTES);put32(out+20u,TRACK_JOURNAL_SLOTS);
 put32(out+24u,track_store_crc32(out,24u));
}
int beetle_flash_claim_probe(beetle_flash_read_fn read,void *ctx){
 if(!read)return BEETLE_CLAIM_INVALID;
 uint8_t marker[32],expected[32];
 if(read(ctx,BEETLE_FLASH_MARKER,marker,sizeof(marker)))return BEETLE_CLAIM_IO;
 beetle_flash_make_marker(expected);
 if(memcmp(marker,expected,sizeof(marker))==0){
  /* V0.34: a matching 32-byte ownership marker cannot legitimize foreign
   * data elsewhere in its 4KiB sector. Check the entire marker sector. */
  uint8_t block[256];
  for(uint32_t pos=BEETLE_FLASH_MARKER+32u;pos<BEETLE_FLASH_MARKER_END;){
   uint32_t count=BEETLE_FLASH_MARKER_END-pos;
   if(count>sizeof(block))count=sizeof(block);
   if(read(ctx,pos,block,count))return BEETLE_CLAIM_IO;
   for(uint32_t k=0;k<count;k++)if(block[k]!=0xFFu)return BEETLE_CLAIM_FOREIGN;
   pos+=count;
  }
  return BEETLE_CLAIM_EXISTING;
 }
 /* Never treat foreign/unknown bytes as an owned filesystem. Even blank NOR
  * needs explicit user confirmation to write the ownership header. */
 for(unsigned i=0;i<sizeof(marker);i++)if(marker[i]!=0xFFu)return BEETLE_CLAIM_FOREIGN;
 /* Also reject residue in the same sector when the first 32 bytes look
  * erased. Header-only emptiness is not sufficient ownership evidence. */
 uint8_t page[256];
 for(uint32_t pos=BEETLE_FLASH_MARKER+32u;pos<BEETLE_FLASH_MARKER_END;){
  uint32_t len=BEETLE_FLASH_MARKER_END-pos;if(len>sizeof(page))len=sizeof(page);
  if(read(ctx,pos,page,len))return BEETLE_CLAIM_IO;
  for(uint32_t k=0;k<len;k++)if(page[k]!=0xffu)return BEETLE_CLAIM_FOREIGN;
  pos+=len;
 }
 return BEETLE_CLAIM_BLANK;
}

/* RC1.0.4: Recognize only an erased marker or a damaged CRC in an otherwise
 * EXACT BEETLE layout marker. The rest of the marker sector must be erased.
 * A foreign/partially overwritten marker is NEVER recovered automatically. */
int beetle_flash_marker_repairable(beetle_flash_read_fn read,void *ctx){
 if(!read)return -1;
 uint8_t expected[32],marker[32],block[256];
 beetle_flash_make_marker(expected);
 if(read(ctx,BEETLE_FLASH_MARKER,marker,32u))return -1;
 uint8_t blank=1,header_matches=1;
 for(uint8_t n=0;n<32u;n++){
  if(marker[n]!=0xFFu)blank=0;
  if((n<24u||n>=28u) && marker[n]!=expected[n])header_matches=0;
 }
 if(!blank&&!header_matches)return 0;
 if(!blank&&!memcmp(marker,expected,32u))return 0; /* already owned */
 for(uint32_t at=BEETLE_FLASH_MARKER+32u;at<BEETLE_FLASH_MARKER_END;at+=sizeof(block)){
  if(read(ctx,at,block,sizeof(block)))return -1;
  for(uint16_t k=0;k<sizeof(block);k++)if(block[k]!=0xFFu)return 0;
 }
 return 1;
}
