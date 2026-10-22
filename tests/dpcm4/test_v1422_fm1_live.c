/* GPL-3.0-only. Real native live BD4S integration against a CRC V2 flash image. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "beetle_bd4_live.h"
#define V2LEN (288u*1024u)
static uint8_t region[V2LEN];
static int io(void *ctx,uint32_t physical,uint8_t *out,uint32_t count){
 (void)ctx;
 if(physical<BEETLE_V2_BASE||count>V2LEN||physical-BEETLE_V2_BASE>V2LEN-count)return -1;
 memcpy(out,region+physical-BEETLE_V2_BASE,count);return 0;
}
static void lock(void){} static void unlock(void){}
int main(int argc,char **argv){
 if(argc!=3)return 2;
 FILE *f=fopen(argv[1],"rb");if(!f)return 3;
 if(fread(region,1,V2LEN,f)!=V2LEN||fgetc(f)!=EOF)return 4;
 fclose(f);
 unsigned char workspace[30000];
 beetle_bd4_live_bind(workspace,sizeof(workspace));
 beetle_bd4_live_poll(io,0,lock,unlock);
 if(!beetle_bd4_live_available(0))return 5;
 beetle_bd4_live_request(0,0,21u);
 for(unsigned k=0;k<5;k++)beetle_bd4_live_poll(io,0,lock,unlock);
 f=fopen(argv[2],"wb");if(!f)return 6;
 unsigned played=0,waiting=0;
 for(unsigned k=0;k<40000;k++){
  int16_t a=0;
  int ok=beetle_bd4_live_next(0,21u,&a);
  if(ok==1){fwrite(&a,2,1,f);played++;}
  if(ok==0)waiting++;
  if(ok<0)break;
  if((k%64)==0)beetle_bd4_live_poll(io,0,lock,unlock);
 }
 fclose(f);
 fprintf(stderr,"BD4S LIVE: %u played, %u delayed, %u underruns\n",played,waiting,(unsigned)beetle_bd4_live_underruns());
 if(played<10000||waiting||beetle_bd4_live_underruns())return 7;
 return 0;
}
