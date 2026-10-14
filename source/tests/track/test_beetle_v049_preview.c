/* BEETLE V0.49: project picker only reads journal headers + tiny prefixes. */
#include "track_journal.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static uint8_t flash[2][TRACK_JOURNAL_BANK_BYTES];
static unsigned reads,bytes_read,erases,writes;
static void put16(uint8_t *p,uint16_t n){p[0]=(uint8_t)n;p[1]=(uint8_t)(n>>8);}
static void put32(uint8_t *p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(n>>(i*8));}
static int rd(void *ctx,uint8_t slot,uint8_t bank,uint32_t off,uint8_t *p,uint32_t n){
 (void)ctx;if(slot!=0||bank>=2||off+n>TRACK_JOURNAL_BANK_BYTES)return -1;
 reads++;bytes_read+=n;memcpy(p,flash[bank]+off,n);return 0;
}
static int er(void *ctx,uint8_t slot,uint8_t bank){(void)ctx;(void)slot;(void)bank;erases++;return -1;}
static int wr(void *ctx,uint8_t slot,uint8_t bank,uint32_t off,const uint8_t *p,uint32_t n){
 (void)ctx;(void)slot;(void)bank;(void)off;(void)p;(void)n;writes++;return -1;
}
static void fake_bank(uint8_t bank,uint32_t gen,const char *title){
 uint8_t *b=flash[bank];
 put32(b,TRACK_JOURNAL_MAGIC);put16(b+4,1);b[6]=0;b[7]=0xff;
 put32(b+8,gen);put32(b+12,100);put32(b+16,0x12345678u);b[20]=0;
 put32(b+24,track_store_crc32(b,20));
 uint8_t *p=b+TRACK_JOURNAL_HEADER_BYTES;
 put32(p,TRACK_STORE_MAGIC);put16(p+4,TRACK_STORE_VERSION);put16(p+6,0);
 put32(p+8,100);put32(p+16,track_store_crc32(p,16));
 memset(p+24,' ',12);
 for(unsigned j=0;j<12&&title[j];j++)p[24+j]=(uint8_t)title[j];
}
int main(void){
 struct track_journal_io io={0,rd,er,wr};char title[13];
 memset(flash,0xff,sizeof flash);
 assert(track_journal_preview(&io,0,title)==TRACK_JOURNAL_NO_PROJECT);
 assert(bytes_read==64u&&erases==0&&writes==0);
 fake_bank(0,1,"FIRST");bytes_read=reads=0;
 assert(track_journal_preview(&io,0,title)==TRACK_JOURNAL_OK);
 assert(memcmp(title,"FIRST",5)==0);assert(bytes_read<=136u);
 fake_bank(1,2,"SECOND");bytes_read=reads=0;
 assert(track_journal_preview(&io,0,title)==TRACK_JOURNAL_OK);
 assert(memcmp(title,"SECOND",6)==0);assert(bytes_read==136u);
 /* One invalid/torn newer header never hides a usable older preview. */
 flash[1][20]=0xff;
 assert(track_journal_preview(&io,0,title)==TRACK_JOURNAL_OK);
 assert(memcmp(title,"FIRST",5)==0);
 assert(erases==0&&writes==0);
 puts("PASS V0.49 browser: bounded <=136-byte provisional previews, correct A/B selection, no writes");
 return 0;
}
