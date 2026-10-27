/* Host regression: index-only NEW PROJECT allocation never treats corrupt
 * metadata as free or reads/decompresses occupied project bodies. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "beetle_storage_v2_reader.h"
static uint8_t flash[8192];
static uint32_t total_reads,project_reads;
static int unreadable_copy=-1;
static void put16(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void put32(uint8_t *p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i));}
static uint32_t crc_step(uint32_t c,const uint8_t *p,unsigned n){
 for(unsigned i=0;i<n;i++){c^=p[i];for(unsigned b=0;b<8;b++)c=(c>>1)^(0xedb88320u&-(c&1u));}return c;
}
static void rehash(void){
 uint8_t *p=flash;
 uint32_t c=crc_step(0xffffffffu,p,16u);
 c=crc_step(c,p+20,566u);
 put32(p+16,~c);
}
static void init_index(void){
 memset(flash,0xff,sizeof(flash));memset(flash,0,4096u);
 memcpy(flash,"B2SB",4u);put16(flash+4,1u);put32(flash+8,7u);
 put32(flash+12,566u);memcpy(flash+4092,"B2OK",4u);
 rehash();
}
static void occupied(unsigned slot,unsigned bank){
 uint8_t *p=flash+20+32u*slot+16u*bank;
 put32(p,9u+slot);put32(p+4,8192u+(slot*2u+bank)*4096u);
 put32(p+8,200u);put32(p+12,0xaabbccdd);
}
static int read_flash(void *ctx,uint32_t address,uint8_t *out,uint32_t n){
 (void)ctx;
 uint32_t off=address-BEETLE_V2_BASE;
 if(off<8192u && unreadable_copy==(int)(off/4096u))return -1;
 if(address<BEETLE_V2_BASE || off>sizeof(flash) || n>sizeof(flash)-off)return -1;
 if(off>=8192u)project_reads++;
 total_reads+=n;memcpy(out,flash+off,n);return 0;
}
static int find_first(void){
 struct beetle_v2_device d;uint8_t scratch[4096];
 if(beetle_v2_open(&d,read_flash,0,scratch))return -1;
 return beetle_v2_first_unused_project(&d,scratch);
}
static int find_fast(void){
 struct beetle_v2_domains audit;
 uint8_t scratch[8192];
 return beetle_v2_audit_and_first_unused_project_fast(read_flash,0,&audit,scratch);
}
static int find_audited(void){
 struct beetle_v2_device d;struct beetle_v2_domains audit;
 uint8_t scratch[4096];
 if(beetle_v2_open(&d,read_flash,0,scratch))return -1;
 return beetle_v2_audit_and_first_unused_project(&d,&audit,scratch);
}
static void same_result(int expected){
 total_reads=project_reads=0;
 assert(find_audited()==expected);
 assert(total_reads>=2u*4096u && total_reads<=3u*4096u && project_reads==0);
 total_reads=project_reads=0;
 assert(find_fast()==expected);
 assert(total_reads==2u*4096u&&project_reads==0);
}
static void second_generation(uint32_t gen){
 memcpy(flash+4096u,flash,4096u);
 put32(flash+4096u+8u,gen);
 uint8_t *p=flash+4096u;
 uint32_t c=crc_step(0xffffffffu,p,16u);
 c=crc_step(c,p+20u,566u);put32(p+16u,~c);
}
int main(void){
 init_index();assert(find_first()==0);same_result(0);
 occupied(0,0);occupied(0,1);occupied(1,0);rehash();
 total_reads=project_reads=0;assert(find_first()==2);
 assert(total_reads==3u*4096u && project_reads==0);
 same_result(2);
 for(unsigned s=2;s<12;s++)occupied(s,0);
 rehash();assert(find_first()==12);same_result(12);
 init_index();occupied(0,1);rehash();assert(find_first()==1);same_result(1);
 init_index();put32(flash+20+16,8192u);rehash();assert(find_first()==-1);same_result(-1);
 init_index();occupied(0,0);put32(flash+20+4,42u);rehash();assert(find_first()==-1);same_result(-1);
 init_index();flash[20] ^= 1u;assert(find_first()==-1);same_result(-1);
 /* Audited lookup must reject cross-bank overlap even when both references
  * appear individually valid. The old basic slot scan does not audit it. */
 init_index();occupied(0,0);occupied(1,0);
 put32(flash+20+32u+4u,8192u);rehash();same_result(-1);
 /* A valid second copy with a newer generation must win. A corrupt
  * second copy must not mask a valid first copy (power loss safety). */
 init_index();occupied(0,0);rehash();second_generation(8u);
 same_result(1);
 /* A newer valid index with an extra assigned slot must win. */
 occupied(1,0);memcpy(flash+4096u,flash,4096u);
 put32(flash+4096u+8u,8u);
 {uint8_t *p=flash+4096u;uint32_t c=crc_step(0xffffffffu,p,16u);c=crc_step(c,p+20u,566u);put32(p+16u,~c);}
 memset(flash+20u+32u,0,16u);rehash();same_result(2);
 /* Corrupt latest CRC => fall back to first, which has slot 0 occupied only. */
 flash[4096u+20u]^=1u;same_result(1);
 /* First copy invalid, second good => use second. */
 init_index();second_generation(9u);flash[20u]^=1u;same_result(0);
 /* Both invalid => fail closed. */
 flash[4096u+20u]^=1u;same_result(-1);
 /* Partial flash failure: a valid companion index must remain readable. */
 init_index();second_generation(8u);
 unreadable_copy=1;assert(find_audited()==0&&find_fast()==0);
 unreadable_copy=0;assert(find_audited()==0&&find_fast()==0);
 unreadable_copy=-1;
 puts("PASS V1.7.11: 8KiB fast scan, same CRC/sector-overlap/slot results as 12KiB legacy scan");
 return 0;
}
