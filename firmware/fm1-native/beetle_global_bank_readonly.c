/* SPDX-License-Identifier: GPL-3.0-only
 * Native PC/FM1-portable, read-only parser for the PC .beetlebank container.
 * This file does NOT authorize FM1 flash allocation, USB upload or playback.
 */
#include "beetle_global_bank_readonly.h"
#include <string.h>
#define HEADER_BYTES 24u
#define ENTRY_BYTES 40u
#define INDEX_END (HEADER_BYTES+BEETLE_GBANK_SLOTS*ENTRY_BYTES)
static uint16_t r16(const uint8_t *p){return (uint16_t)(p[0]|(uint16_t)p[1]<<8);}
static uint32_t r32(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|
 (uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint32_t crc_step(uint32_t v,const uint8_t *p,uint32_t n){
 for(uint32_t i=0;i<n;i++){v^=p[i];for(uint32_t b=0;b<8;b++)v=(v>>1)^((v&1u)?0xEDB88320u:0u);}return v;
}
static int read_at(beetle_gbank_read_fn f,void *ctx,uint32_t max,uint32_t off,uint8_t *p,uint32_t n){
 if(off>max||n>max-off||n>512u)return -1;
 return f(ctx,off,p,n)==0?0:-2;
}
static int crc_region(beetle_gbank_read_fn f,void *ctx,uint32_t max,
                      uint32_t off,uint32_t len,uint32_t *result){
 if(off>max||len>max-off)return -1;
 uint8_t buf[256];uint32_t state=~0u;
 while(len){uint32_t n=len>sizeof(buf)?(uint32_t)sizeof(buf):len;
  int ok=read_at(f,ctx,max,off,buf,n);if(ok)return ok;
  state=crc_step(state,buf,n);off+=n;len-=n;
 }
 *result=state^~0u;return 0;
}
static int is_id(const uint8_t *p,const char id[4]){return !memcmp(p,id,4);}
static int wav_info(beetle_gbank_read_fn f,void *ctx,uint32_t max,uint32_t off,
                    uint32_t length,struct beetle_gbank_slot *slot){
 uint8_t a[48];
 if(length<44u)return -1;
 int err=read_at(f,ctx,max,off,a,12);if(err)return err;
 if(!is_id(a,"RIFF")||!is_id(a+8,"WAVE"))return -1;
 uint32_t riff=r32(a+4);
 if(riff>length-8u||riff<36u)return -1;
 uint32_t end=riff+8u,pos=12u,align=0u,bytes=0u,rate=0u;
 uint16_t bits=0u,channels=0u,format=0u;
 int fmt=0,data=0;
 while(pos<end){
  if(end-pos<8u)return -1;
  err=read_at(f,ctx,max,off+pos,a,8);if(err)return err;
  uint32_t size=r32(a+4);
  if(size>end-pos-8u||size+(size&1u)>end-pos-8u)return -1;
  if(is_id(a,"fmt ")){
   if(fmt||size<16u)return -1;
   uint32_t n=size<40u?size:40u;
   err=read_at(f,ctx,max,off+pos+8u,a,n);if(err)return err;
   format=r16(a);channels=r16(a+2);rate=r32(a+4);
   uint32_t throughput=r32(a+8);align=r16(a+12);bits=r16(a+14);
   if(format==65534u){
    if(size<40u||r16(a+16)<22u||r16(a+18)>bits)return -1;
    static const uint8_t tail[14]={0,0,0,0,16,0,128,0,0,170,0,56,155,113};
    if(memcmp(a+26,tail,sizeof(tail)))return -3;
    format=r16(a+24);
   }
   if(format!=1u&&format!=3u)return -3;
   if(!channels||channels>32u||rate<1000u||rate>384000u||
      !((format==1u&&(bits==8u||bits==16u||bits==24u||bits==32u))||
        (format==3u&&(bits==32u||bits==64u)))||
      align!=(uint32_t)channels*(bits/8u)||throughput!=rate*align)return -1;
   fmt=1;
  }else if(is_id(a,"data")){
   if(data)return -3;
   data=1;bytes=size;
  }
  pos+=8u+size+(size&1u);
 }
 if(!fmt||!data||!bytes||bytes%align)return -1;
 slot->audio_bytes=bytes;slot->frames=bytes/align;slot->sample_rate=rate;
 slot->channels=channels;slot->bits_per_sample=bits;slot->format=(uint8_t)format;
 return 0;
}
int beetle_gbank_open_readonly(beetle_gbank_read_fn read,void *ctx,uint32_t total,
                              struct beetle_gbank_index *out){
 if(!read||!out||total<INDEX_END||total>BEETLE_GBANK_MAX_FILE)return -1;
 struct beetle_gbank_index tmp;memset(&tmp,0,sizeof(tmp));
 uint8_t hdr[HEADER_BYTES],ent[ENTRY_BYTES];
 int err=read_at(read,ctx,total,0u,hdr,sizeof(hdr));if(err)return err;
 if(memcmp(hdr,"BEETBANK",8)||r16(hdr+8)!=1u||r16(hdr+10)!=BEETLE_GBANK_SLOTS||r32(hdr+12)!=total)return -1;
 if((crc_step(~0u,hdr,20u)^~0u)!=r32(hdr+20))return -1;
 uint32_t actual;
 err=crc_region(read,ctx,total,HEADER_BYTES,total-HEADER_BYTES,&actual);if(err)return err;
 if(actual!=r32(hdr+16))return -1;
 uint32_t next=INDEX_END;
 for(uint32_t i=0;i<BEETLE_GBANK_SLOTS;i++){
  err=read_at(read,ctx,total,HEADER_BYTES+i*ENTRY_BYTES,ent,sizeof(ent));if(err)return err;
  uint32_t at=r32(ent),len=r32(ent+4),crc=r32(ent+8);
  const uint8_t name_len=ent[12],file_len=ent[13];
  if(at!=next||len>total-at||name_len>12u||file_len>12u||ent[38]||ent[39])return -1;
  struct beetle_gbank_slot *s=&tmp.slots[i];
  s->wav_offset=at;s->wav_bytes=len;
  memcpy(s->name,ent+14,name_len);memcpy(s->filename,ent+26,file_len);
  if(!len&&(name_len||file_len))return -1;
  err=crc_region(read,ctx,total,at,len,&actual);if(err)return err;
  if(actual!=crc)return -1;
  if(len){err=wav_info(read,ctx,total,at,len,s);if(err)return err;}
  next+=len;tmp.bytes_audio+=len;
 }
 if(next!=total)return -1;
 tmp.bytes_total=total;
 *out=tmp;
 return 0;
}
