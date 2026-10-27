/* Host regression: index-only NEW PROJECT allocation never treats corrupt
 * metadata as free or reads/decompresses occupied project bodies. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "beetle_storage_v2_reader.h"
static uint8_t flash[8192];
static uint32_t total_reads,project_reads;
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
 if(address<BEETLE_V2_BASE || off>sizeof(flash) || n>sizeof(flash)-off)return -1;
 if(off>=8192u)project_reads++;
 total_reads+=n;memcpy(out,flash+off,n);return 0;
}
static int find_first(void){
 struct beetle_v2_device d;uint8_t scratch[4096];
 if(beetle_v2_open(&d,read_flash,0,scratch))return -1;
 return beetle_v2_first_unused_project(&d,scratch);
}
static int find_audited(void){
 struct beetle_v2_device d;struct beetle_v2_domains audit;
 uint8_t scratch[4096];
 if(beetle_v2_open(&d,read_flash,0,scratch))return -1;
 return beetle_v2_audit_and_first_unused_project(&d,&audit,scratch);
}
int main(void){
 init_index();assert(find_first()==0);
 occupied(0,0);occupied(0,1);occupied(1,0);rehash();
 total_reads=project_reads=0;assert(find_first()==2);
 assert(total_reads==3u*4096u && project_reads==0);
 total_reads=project_reads=0;assert(find_audited()==2);
 assert(total_reads==3u*4096u && project_reads==0);
 for(unsigned s=2;s<12;s++)occupied(s,0);
 rehash();assert(find_first()==12);assert(find_audited()==12);
 init_index();occupied(0,1);rehash();assert(find_first()==1);assert(find_audited()==1);
 init_index();put32(flash+20+16,8192u);rehash();assert(find_first()==-1);assert(find_audited()==-1);
 init_index();occupied(0,0);put32(flash+20+4,42u);rehash();assert(find_first()==-1);assert(find_audited()==-1);
 init_index();flash[20] ^= 1u;assert(find_first()==-1);assert(find_audited()==-1);
 /* Audited lookup must reject cross-bank overlap even when both references
  * appear individually valid. The old basic slot scan does not audit it. */
 init_index();occupied(0,0);occupied(1,0);
 put32(flash+20+32u+4u,8192u);rehash();assert(find_audited()==-1);
 puts("PASS V1.7.9: combined audited slot allocation 12KiB index reads, no project bodies, fail-closed overlap/corruption");
 return 0;
}
