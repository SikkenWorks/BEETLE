/* BEETLE project storage, native HAL-compatible offsets. Host-only safety tests. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "beetle_flash_layout.h"
static unsigned fails;
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);fails++;}}while(0)
static uint8_t chip[0x100000];
static int rd(void *ctx,uint32_t off,uint8_t *dest,uint32_t n){
 (void)ctx;if(off>sizeof(chip)||n>sizeof(chip)-off)return -1;memcpy(dest,chip+off,n);return 0;
}
static void put32(uint8_t *b,uint32_t v){for(unsigned i=0;i<4;i++)b[i]=(uint8_t)(v>>(8*i));}
static void put16(uint8_t *b,uint16_t v){b[0]=(uint8_t)v;b[1]=(uint8_t)(v>>8);}
int main(void){
 memset(chip,0xFF,sizeof(chip));
 CHECK(BEETLE_FLASH_BYTES==4u*2u*20480u);
 CHECK(beetle_flash_claim_probe(rd,0)==BEETLE_CLAIM_BLANK);
 unsigned banks[4][2]={0};
 for(unsigned s=0;s<4;s++)for(unsigned b=0;b<2;b++){
  uint32_t at=0;CHECK(beetle_flash_map(s,b,0,20480,&at)==0);
  CHECK(at==0xB4000u+(s*2+b)*20480u);
  CHECK(at>=0xB4000u&&at+20480u<=0xDC000u);banks[s][b]=at;
  CHECK(beetle_flash_map(s,b,20479,2,&at)<0);
 }
 uint32_t at=0;CHECK(beetle_flash_map(4,0,0,1,&at)<0);
 CHECK(beetle_flash_map(0,2,0,1,&at)<0);
 CHECK(beetle_flash_map(0,0,0xffffffffu,1,&at)<0);
 chip[0xB4000u+12000]=0;CHECK(beetle_flash_claim_probe(rd,0)==BEETLE_CLAIM_FOREIGN);
 chip[0xB4000u+12000]=0xff;
 memcpy(chip+0xA0000,"FSMP",4);CHECK(beetle_flash_claim_probe(rd,0)==BEETLE_CLAIM_FOREIGN);
 memset(chip+0xA0000,0xff,4);
 memcpy(chip+0xA0000u+254u,"FSMP",4);
 CHECK(beetle_flash_claim_probe(rd,0)==BEETLE_CLAIM_FOREIGN);
 memset(chip+0xA0000u+254u,0xff,4);
 memcpy(chip+0xB4000,"ETCH",4);CHECK(beetle_flash_claim_probe(rd,0)==BEETLE_CLAIM_FOREIGN);
 memset(chip+0xB4000,0xff,4);
 /* Valid committed BEETLE TRKJ record: only owned project metadata.
  * No ETCH, Felucca or audio samples are serialized. */
 uint8_t* h=chip+banks[0][0];unsigned n=50;
 for(unsigned i=0;i<n;i++)h[TRACK_JOURNAL_HEADER_BYTES+i]=(uint8_t)i;
 put32(h,TRACK_JOURNAL_MAGIC);put16(h+4,1);h[6]=0;h[7]=255;
 put32(h+8,1);put32(h+12,n);
 put32(h+16,track_store_crc32(h+TRACK_JOURNAL_HEADER_BYTES,n));
 h[20]=0;put32(h+24,track_store_crc32(h,20));
 CHECK(beetle_flash_claim_probe(rd,0)==BEETLE_CLAIM_EXISTING);
 h[TRACK_JOURNAL_HEADER_BYTES+2]^=1;
 CHECK(beetle_flash_claim_probe(rd,0)==BEETLE_CLAIM_FOREIGN);
 h[TRACK_JOURNAL_HEADER_BYTES+2]^=1;
 h[TRACK_JOURNAL_HEADER_BYTES+n+9]=1;
 CHECK(beetle_flash_claim_probe(rd,0)==BEETLE_CLAIM_FOREIGN);
 h[TRACK_JOURNAL_HEADER_BYTES+n+9]=0xff;
 chip[0xE0000u]='O';chip[0xFC000u]='G';
 CHECK(beetle_flash_claim_probe(rd,0)==BEETLE_CLAIM_EXISTING);
 if(fails)return 1;
 puts("PASS: ETCH/Felucca flash partition map, 8x20KiB banks, byte-range validation, foreign data protection, committed BEETLE CRC, OTA/global isolation");
 return 0;
}
