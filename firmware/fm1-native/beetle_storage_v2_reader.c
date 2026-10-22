/* SPDX-License-Identifier: GPL-3.0-only */
#include "beetle_storage_v2_reader.h"
#include "beetle_project_pack_v2.h"
#include <string.h>
static uint16_t u16(const uint8_t *p){return (uint16_t)(p[0]|((uint16_t)p[1]<<8));}
static uint32_t u32(const uint8_t *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static uint32_t crc_advance(uint32_t c,const uint8_t *b,uint32_t n){
 for(uint32_t i=0;i<n;i++){c^=b[i];for(unsigned bit=0;bit<8;bit++)c=(c>>1)^(0xEDB88320u&-(c&1u));}
 return c;
}
static uint32_t crc32(const uint8_t *p,uint32_t n){return ~crc_advance(0xFFFFFFFFu,p,n);}
void beetle_v2_make_marker(uint8_t out[32]){
 static const uint8_t magic[8]={'B','T','L','S','T','V','2',1u};
 if(!out)return;
 memset(out,0xff,32u);memcpy(out,magic,8u);
 const uint32_t fields[3]={BEETLE_V2_BASE,BEETLE_V2_BASE+BEETLE_V2_BYTES,4096u};
 for(unsigned k=0;k<3u;k++)for(unsigned j=0;j<4u;j++)out[8u+k*4u+j]=(uint8_t)(fields[k]>>(j*8u));
 uint32_t check=crc32(out,20u);
 for(unsigned j=0;j<4u;j++)out[20u+j]=(uint8_t)(check>>(8u*j));
}
static int reader(const struct beetle_v2_device *d,uint32_t rel,uint8_t *p,uint32_t n){
 if(!d||!d->read||!p||n>BEETLE_V2_BYTES||rel>BEETLE_V2_BYTES-n)return -1;
 return d->read(d->context,BEETLE_V2_BASE+rel,p,n)?-1:0;
}
static int index_parse(const uint8_t *b,uint32_t *generation){
 if(memcmp(b,"B2SB",4)||u16(b+4)!=1||u16(b+6)!=0||memcmp(b+4092,"B2OK",4))return -1;
 uint32_t n=u32(b+12);if(!u32(b+8)||n>4096u-24u||n<12u*32u+13u*14u)return -1;
 uint32_t c=crc_advance(0xFFFFFFFFu,b,16);
 c=crc_advance(c,b+20,n);
 if((~c)!=u32(b+16))return -1;
 uint32_t pos=20u+12u*32u,total=0u;
 for(unsigned i=0;i<13;i++){
  if(pos+14u>20u+n)return -1;
  uint32_t g=u32(b+pos),len=u32(b+pos+4),check=u32(b+pos+8);
  uint16_t count=u16(b+pos+12);pos+=14u;
  if(!g){if(len||check||count)return -1;}
  else if(!len||!count||count>128u-total)return -1;
  total+=count;
  if(pos+12u*(uint32_t)count>20u+n)return -1;
  for(unsigned k=0;k<count;k++){
   uint32_t off=u32(b+pos),size=u32(b+pos+4);
   if(off<8192u||size==0||size>4068u||(off&255u)||
      (size>BEETLE_V2_BYTES-24u)||off>BEETLE_V2_BYTES-24u-size)return -1;
   pos+=12u;
  }
 }
 if(pos!=20u+n)return -1;
 if(generation)*generation=u32(b+8);
 return 0;
}
int beetle_v2_open(struct beetle_v2_device *d,beetle_v2_read rd,void *context,uint8_t *scratch){
 if(!d||!rd||!scratch)return -1;
 memset(d,0,sizeof(*d));d->read=rd;d->context=context;
 uint32_t best=0u;
 for(unsigned sec=0;sec<2;sec++){
  if(reader(d,4096u*sec,scratch,4096u))continue;
  uint32_t gen;
  if(index_parse(scratch,&gen))continue;
  if(!best||(int32_t)(gen-best)>0){best=gen;d->index_address=sec*4096u;}
 }
 if(!best){d->read=0;return -2;}
 d->generation=best;return 0;
}
static int current_index(struct beetle_v2_device *d,uint8_t *scratch){
 if(!d||!d->read||!scratch||reader(d,d->index_address,scratch,4096u))return -1;
 uint32_t gen=0u;return index_parse(scratch,&gen)||gen!=d->generation?-1:0;
}
static int sample_scan(uint8_t slot,const uint8_t *index,struct beetle_v2_sample *info){
 uint32_t pos=20u+12u*32u;
 for(unsigned i=0;i<13;i++){
  uint16_t count=u16(index+pos+12);
  if(i==slot){
   if(!u32(index+pos)){memset(info,0,sizeof(*info));return 0;}
   info->generation=u32(index+pos);info->byte_length=u32(index+pos+4);
   info->crc=u32(index+pos+8);info->chunks=count;info->index_offset=(uint16_t)(pos+14u);
   return 1;
  }
  pos+=14u+12u*count;
 }
 return -1;
}
int beetle_v2_sample_info(struct beetle_v2_device *d,uint8_t slot,struct beetle_v2_sample *info,uint8_t *scratch){
 if(slot>=13||!info||current_index(d,scratch))return -1;
 return sample_scan(slot,scratch,info);
}
/* Verify full chunk record against its index without loading it in RAM. */
static int chunk_read(struct beetle_v2_device *d,uint8_t slot,uint32_t gen,uint16_t part,
 struct beetle_v2_ref ref,uint32_t start,uint8_t *out,uint32_t n){
 if(!ref.length||ref.length>4068||start>ref.length||n>ref.length-start||
    (ref.offset&255u)||ref.offset<8192u||ref.offset>BEETLE_V2_BYTES-24u-ref.length)return -1;
 uint8_t header[24],temp[256];
 if(reader(d,ref.offset,header,24))return -1;
 if(memcmp(header,"B2RC",4)||header[4]!=2||header[5]!=slot||u16(header+6)!=part||
    u32(header+8)!=gen||u32(header+12)!=ref.length||u32(header+16)!=ref.crc||
    crc32(header,20)!=u32(header+20))return -1;
 uint32_t c=0xFFFFFFFFu;
 /* Complete-chunk callers avoid the old second full flash read. */
 uint8_t *full=(start==0u&&n==ref.length)?out:0;
 for(uint32_t p=0;p<ref.length;){
  uint32_t count=ref.length-p;if(count>256u)count=256u;
  uint8_t *buffer=full?full+p:temp;
  if(reader(d,ref.offset+24u+p,buffer,count))return -1;
  c=crc_advance(c,buffer,count);p+=count;
 }
 if((~c)!=ref.crc)return -1;
 if(n&&!full&&reader(d,ref.offset+24u+start,out,n))return -1;
 return 0;
}
int beetle_v2_sample_read(struct beetle_v2_device *d,uint8_t slot,uint32_t offset,
 uint8_t *out,uint32_t length,uint8_t *scratch){
 struct beetle_v2_sample info;
 if((length&&!out)||beetle_v2_sample_info(d,slot,&info,scratch)!=1||
     offset>info.byte_length||length>info.byte_length-offset)return -1;
 uint32_t pos=0u,remain=length;
 for(unsigned part=0;part<info.chunks;part++){
  uint32_t q=info.index_offset+part*12u;
  struct beetle_v2_ref r={u32(scratch+q),u32(scratch+q+4),u32(scratch+q+8)};
  if(pos>info.byte_length||r.length>info.byte_length-pos)return -1;
  uint32_t lo=offset>pos?offset-pos:0u;
  if(lo<r.length&&remain){
   uint32_t take=r.length-lo;if(take>remain)take=remain;
   if(chunk_read(d,slot,info.generation,(uint16_t)part,r,lo,out,take))return -1;
   out+=take;offset+=take;remain-=take;
  }
  pos+=r.length;
 }
 return remain||pos!=info.byte_length?-1:0;
}
int beetle_v2_sample_chunk_read(struct beetle_v2_device *d,uint8_t slot,
 const struct beetle_v2_sample *expected,uint16_t part,uint8_t *out,
 uint16_t *written,uint8_t index[4096]){
 if(written)*written=0u;
 if(!d||!expected||!out||!written||!index||slot>=BEETLE_V2_SAMPLES||
    current_index(d,index))return -1;
 struct beetle_v2_sample now;
 if(sample_scan(slot,index,&now)!=1||now.generation!=expected->generation||
    now.byte_length!=expected->byte_length||now.crc!=expected->crc||
    part>=now.chunks)return -2;
 uint32_t q=(uint32_t)now.index_offset+part*12u;
 struct beetle_v2_ref r={u32(index+q),u32(index+q+4),u32(index+q+8)};
 if(r.length>4068u||chunk_read(d,slot,now.generation,part,r,0u,out,r.length))return -3;
 *written=(uint16_t)r.length;return 0;
}
static int project_attempt(struct beetle_v2_device *d,uint8_t slot,uint32_t g,
 struct beetle_v2_ref r,uint8_t *out,uint32_t capacity,uint32_t *written,
 uint8_t *packed,uint32_t packed_capacity){
 if(!g||r.length<16u||r.length>12272u||r.length>packed_capacity||
    (r.offset&255u)||r.offset<8192u||r.offset>BEETLE_V2_BYTES-24u-r.length)return -1;
 uint8_t h[24];
 if(reader(d,r.offset,h,24))return -1;
 if(memcmp(h,"B2RC",4)||h[4]!=1u||h[5]!=slot||u16(h+6)!=0u||
    u32(h+8)!=g||u32(h+12)!=r.length||u32(h+16)!=r.crc||
    crc32(h,20)!=u32(h+20))return -1;
 uint32_t c=0xFFFFFFFFu;
 for(uint32_t p=0;p<r.length;){
  uint32_t n=r.length-p;if(n>256)n=256;
  if(reader(d,r.offset+24u+p,packed+p,n))return -1;
  c=crc_advance(c,packed+p,n);p+=n;
 }
 if((~c)!=r.crc)return -1;
 size_t decoded=0;
 if(beetle_unpack_project_v2(packed,r.length,out,capacity,&decoded))return -1;
 if(written)*written=(uint32_t)decoded;
 return 1;
}
int beetle_v2_project_read(struct beetle_v2_device *d,uint8_t slot,uint8_t *out,
 uint32_t capacity,uint32_t *written,uint8_t *scratch,
 uint8_t *packed_work,uint32_t packed_capacity){
 if(written)*written=0;
 if(slot>=BEETLE_V2_PROJECTS||!out||!packed_work||current_index(d,scratch))return -1;
 const uint32_t pos=20u+(uint32_t)slot*32u;
 int available=0;
 for(unsigned n=0;n<2;n++){
  const uint8_t *p=scratch+pos+n*16u;
  uint32_t gen=u32(p);
  struct beetle_v2_ref r={u32(p+4),u32(p+8),u32(p+12)};
  if(!gen)continue;
  available=1;
  if(project_attempt(d,slot,gen,r,out,capacity,written,packed_work,packed_capacity)==1)return 1;
 }
 return available?-2:0;
}

/* Verify PROJECT and SAMPLE sector ownership without allocating a second
 * full-flash bitmap. This deliberately does NOT run in audio or USB IRQ.
 * It can be called before enabling a migrated Storage V2 volume.
 */
int beetle_v2_audit_domains(struct beetle_v2_device *d,
 struct beetle_v2_domains *report,uint8_t *scratch){
 if(!report||current_index(d,scratch))return -1;
 uint8_t owners[72]={0};
 uint16_t occupied_pages[72]={0};
 struct beetle_v2_domains result={0};
 for(unsigned slot=0;slot<BEETLE_V2_PROJECTS;slot++){
  uint32_t pos=20u+slot*32u;
  int assigned=0;
  for(unsigned bank=0;bank<2;bank++){
   const uint8_t *r=scratch+pos+bank*16u;
   uint32_t gen=u32(r),offset=u32(r+4),length=u32(r+8);
   if(!gen){if(offset||length||u32(r+12))return -2;continue;}
   if(length<16u||length>12272u||offset<8192u||(offset&255u)||
      offset>BEETLE_V2_BYTES-24u-length)return -2;
   uint32_t first=offset/4096u;
   uint32_t last=(offset+((length+24u+255u)&~255u)-1u)/4096u;
   if(last>=72u)return -2;
   for(uint32_t sector=first;sector<=last;sector++){
    if(owners[sector]==2u)return -2;
    if(owners[sector]==0u){owners[sector]=1u;result.project_sectors++;}
   }
   uint32_t end=offset+((length+24u+255u)&~255u);
   for(uint32_t page=offset/256u;page<end/256u;page++){
    uint32_t sector=page/16u;
    uint16_t mask=(uint16_t)(1u<<(page%16u));
    if(occupied_pages[sector]&mask)return -2;
    occupied_pages[sector]|=mask;
   }
   assigned=1;
  }
  result.assigned_projects+=(uint16_t)assigned;
 }
 uint32_t pos=20u+BEETLE_V2_PROJECTS*32u;
 for(unsigned slot=0;slot<BEETLE_V2_SAMPLES;slot++){
  uint32_t gen=u32(scratch+pos),len=u32(scratch+pos+4);
  uint16_t count=u16(scratch+pos+12);pos+=14u;
  if(!gen){if(len||count) return -2;continue;}
  uint32_t sum=0u;
  for(unsigned part=0;part<count;part++){
   uint32_t offset=u32(scratch+pos),length=u32(scratch+pos+4);
   if(offset<8192u||(offset&255u)||length==0u||length>4068u||
      offset>BEETLE_V2_BYTES-24u-length)return -2;
   uint32_t sector=offset/4096u;
   uint32_t padded=(length+24u+255u)&~255u;
   if(sector>=72u||(offset%4096u)+padded>4096u||owners[sector]==1u)return -2;
   if(owners[sector]==0u){owners[sector]=2u;result.sample_sectors++;}
   for(uint32_t page=offset/256u;page<(offset+padded)/256u;page++){
    uint16_t mask=(uint16_t)(1u<<(page%16u));
    if(occupied_pages[sector]&mask)return -2;
    occupied_pages[sector]|=mask;
   }
   if(sum>0xFFFFFFFFu-length)return -2;
   sum+=length;pos+=12u;
  }
  if(sum!=len)return -2;
  result.assigned_samples++;
 }
 if(pos!=20u+u32(scratch+12))return -2;
 *report=result;
 return 0;
}
