/* SPDX-License-Identifier: GPL-3.0-only
 * Host race/load regression for BEETLE V1.4.22 live FM1 BD4S.
 * No flash reads are permitted between prefetch polls.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "beetle_bd4_live.h"
#define V2LEN (288u*1024u)
static uint8_t volume[V2LEN];
static uint32_t flash_reads;
static int io(void *ctx,uint32_t physical,uint8_t *out,uint32_t count){
 (void)ctx;
 if(physical<BEETLE_V2_BASE||count>V2LEN||physical-BEETLE_V2_BASE>V2LEN-count)return -1;
 memcpy(out,volume+physical-BEETLE_V2_BASE,count);flash_reads++;return 0;
}
static void lock(void){}static void unlock(void){}
int main(int argc,char **argv){
 if(argc!=2)return 2;
 FILE *f=fopen(argv[1],"rb");if(!f)return 3;
 if(fread(volume,1,V2LEN,f)!=V2LEN||fgetc(f)!=EOF)return 4;
 fclose(f);
 static uint8_t scratch[30000];
 beetle_bd4_live_bind(scratch,sizeof(scratch));
 beetle_bd4_live_poll(io,0,lock,unlock);
 if(!beetle_bd4_live_available(0))return 5;
 for(unsigned i=0;i<6;i++)beetle_bd4_live_request((uint8_t)i,0,i+1u);
 for(unsigned k=0;k<7;k++)beetle_bd4_live_poll(io,0,lock,unlock);
 uint32_t nonzero=0;
 for(unsigned k=0;k<44000;k++){
  uint32_t before=flash_reads;
  int16_t first=0;
  if(beetle_bd4_live_next(0,1u,&first)!=1)return 6;
  if(first)nonzero++;
  for(unsigned i=1;i<6;i++){
   int16_t sample=0;
   if(beetle_bd4_live_next((uint8_t)i,i+1u,&sample)!=1||sample!=first)return 7;
  }
  if(flash_reads!=before)return 8;
  if((k%64u)==0)beetle_bd4_live_poll(io,0,lock,unlock);
 }
 if(beetle_bd4_live_underruns()!=0u||nonzero<20000u)return 9;
 printf("PASS six real streaming samplevoices for 44000 frames; reads outside audio only; 0 underruns\n");
 return 0;
}
