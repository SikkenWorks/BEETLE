/* GPL-3.0-only. Host replay of the *native* OTA CHECK/commit source, not a
 * browser mock. Input is the exact decoded logical .fwsc byte stream. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define FELUCCA_ID "FM-1_2110"
#include "../../firmware/fm1-native/ota.c"

static uint8_t *host_image;
static uint32_t host_length;
static uint8_t device_flash[0x100000];
static uint8_t response_wire[700];
static uint32_t response_size,ticks,read_count,erase_count,commit_count;
static uint32_t error_request;
static int response_pending;

static int ota_wire_send(const uint8_t *p,uint32_t size){
 uint8_t q[700],reply[620],wire[700];
 uint32_t d=ota_unpack7(p+1,size-2,q,sizeof(q));
 if(d!=15||q[2]!=0x30)return 0;
 uint32_t addr=ota_rd32(q+7),n=ota_rd24(q+11);
 if(n>512){error_request=addr;return -1;}
 const uint8_t *data;
 if(addr==0xE0000000u){data=(const uint8_t*)"success";n=8;}
 else if(addr>host_length||n>host_length-addr){error_request=addr;return -1;}
 else data=host_image+addr;
 memset(reply,0,sizeof(reply));reply[1]=0x59;reply[2]=0x30;
 ota_wr32(reply+7,addr);
 reply[3]=(uint8_t)(n+8u);reply[4]=(uint8_t)((n+8u)>>8);
 reply[11]=(uint8_t)n;reply[12]=(uint8_t)(n>>8);
 memcpy(reply+14,data,n);
 uint32_t sum=0;for(uint32_t i=6;i<14u+n;i++)sum+=reply[i];
 reply[14+n]=(uint8_t)~sum;
 wire[0]=0xf0;uint32_t w=ota_pack7(reply,n+15u,wire+1)+1u;wire[w++]=0xf7;
 memcpy(response_wire,wire,w);response_size=w;response_pending=1;read_count++;
 return 0;
}
static int ota_frame_get(const uint8_t **p,uint32_t *n){if(!response_pending)return 0;*p=response_wire+1;*n=response_size-2u;return 1;}
static void ota_frame_done(void){response_pending=0;}
static uint32_t ota_now_ms(void){return ticks++;}
static void ota_idle(void){ticks++;}
static int ota_aborted(void){return 0;}
static int ota_erase(uint32_t off){if(off<OTA_AREA||off>=OTA_AREA+OTA_AREA_LEN||(off&0xfffu))return -1;memset(device_flash+off,0xff,0x1000u);erase_count++;return 0;}
static int ota_prog(uint32_t off,const void *data,uint32_t n){if(off<OTA_AREA||off>OTA_AREA+OTA_AREA_LEN||n>OTA_AREA+OTA_AREA_LEN-off)return -1;const uint8_t *p=data;for(uint32_t i=0;i<n;i++)device_flash[off+i]&=p[i];return 0;}
static int ota_fread(uint32_t off,void *dst,uint32_t n){if(off>sizeof(device_flash)||n>sizeof(device_flash)-off)return -1;memcpy(dst,device_flash+off,n);return 0;}
static void ota_show(uint32_t step,int32_t rc){(void)step;(void)rc;}
static void ota_commit(const uint8_t *parm){(void)parm;commit_count++;}
int main(int argc,char **argv){
 if(argc!=4){fprintf(stderr,"usage: native_cross_firmware_replay logical.bin expected_exit expected_commit\n");return 2;}
 FILE *f=fopen(argv[1],"rb");if(!f)return 3;
 fseek(f,0,SEEK_END);host_length=(uint32_t)ftell(f);rewind(f);
 host_image=malloc(host_length);if(!host_image)return 3;
 if(fread(host_image,1,host_length,f)!=host_length)return 3;fclose(f);
 memset(device_flash,0xff,sizeof(device_flash));
 int result=ota_session();
 int expected=atoi(argv[2]),expected_commit=atoi(argv[3]);
 printf("native_ota_result=%d read_requests=%u erases=%u commits=%u staged_loader=%s\n",result,read_count,erase_count,commit_count,
  memcmp(device_flash+OTA_AREA+16,"LOADER.BIN",10)==0?"yes":"no");
 if(result!=expected||commit_count!=(uint32_t)expected_commit)return 1;
 if(result!=0&&(erase_count!=0||commit_count!=0))return 1;
 if(result==0&&(erase_count!=5||commit_count!=1))return 1;
 return 0;
}
