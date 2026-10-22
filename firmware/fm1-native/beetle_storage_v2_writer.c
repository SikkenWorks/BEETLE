/* SPDX-License-Identifier: GPL-3.0-only
 * BEETLE native Storage V2 copy-on-write NOR transaction writer.
 * No reference to external sampling implementations. No IRQ calls.
 */
#include "beetle_storage_v2_writer.h"
#include <string.h>
#define SECTOR 4096u
#define PAGE 256u
#define CAP BEETLE_V2_BYTES
#define NSEC 72u
#define MAX_CHUNK 4068u
static void move_bytes(uint8_t *dst,const uint8_t *src,uint32_t count){
 if(dst==src||!count)return;
 if(dst>src){for(uint32_t i=count;i>0u;i--)dst[i-1u]=src[i-1u];}
 else {for(uint32_t i=0;i<count;i++)dst[i]=src[i];}
}
static uint16_t g16(const uint8_t *p){return (uint16_t)(p[0]|((uint16_t)p[1]<<8));}
static uint32_t g32(const uint8_t *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static void s16(uint8_t *p,uint16_t x){p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8);}
static void s32(uint8_t *p,uint32_t x){s16(p,(uint16_t)x);s16(p+2,(uint16_t)(x>>16));}
static uint32_t crc_inc(uint32_t x,const uint8_t *p,uint32_t n){
 for(uint32_t i=0;i<n;i++){x^=p[i];for(int j=0;j<8;j++)x=(x>>1)^(0xEDB88320u&-(x&1u));}return x;
}
static uint32_t crc(const uint8_t *p,uint32_t n){return ~crc_inc(0xffffffffu,p,n);}
static int rd(const struct beetle_v2_writer *w,uint32_t rel,uint8_t *dst,uint32_t n){
 if(!w||!w->device.read||!dst||rel>CAP||n>CAP-rel)return -1;
 return w->device.read(w->context,BEETLE_V2_BASE+rel,dst,n)?-1:0;
}
static int pr(const struct beetle_v2_writer *w,uint32_t rel,const uint8_t *data,uint32_t n){
 if(!w||!w->program||!data||rel>CAP||n>CAP-rel||n>PAGE||(rel%PAGE)+n>PAGE)return -1;
 return w->program(w->context,BEETLE_V2_BASE+rel,data,n)?-1:0;
}
static int er(const struct beetle_v2_writer *w,uint32_t sector){
 if(!w||!w->erase||sector>=NSEC)return -1;
 return w->erase(w->context,BEETLE_V2_BASE+sector*SECTOR)?-1:0;
}
/* Two independent CRC-protected superblock generations. No partial commit. */
static int valid(const uint8_t *b){
 if(memcmp(b,"B2SB",4)||g16(b+4)!=1u||g16(b+6)!=0u||
    !g32(b+8)||memcmp(b+SECTOR-4,"B2OK",4))return 0;
 uint32_t sz=g32(b+12);
 if(sz<12u*32u+13u*14u||sz>SECTOR-24u)return 0;
 uint32_t c=crc_inc(0xffffffffu,b,16);
 if((~crc_inc(c,b+20,sz))!=g32(b+16))return 0;
 uint32_t pos=20u+12u*32u;
 for(unsigned s=0;s<13;s++){
  if(pos+14u>20u+sz)return 0;
  uint32_t gen=g32(b+pos),len=g32(b+pos+4),cnum=g16(b+pos+12);
  if(!gen&&(len||g32(b+pos+8)||cnum))return 0;
  if(gen&&(!len||!cnum||cnum>128u))return 0;
  pos+=14u+cnum*12u;
  if(pos>20u+sz)return 0;
 }
 return pos==20u+sz;
}
static int refresh(struct beetle_v2_writer *w,uint8_t *newest,uint8_t *other){
 if(rd(w,0u,newest,SECTOR)||rd(w,SECTOR,other,SECTOR))return -1;
 int a=valid(newest),b=valid(other);
 if(!a&&!b)return -3;
 if(b&&(!a||(int32_t)(g32(other+8)-g32(newest+8))>0)){
  /* swap 4096 bytes without third sector RAM, one byte at a time */
  for(unsigned i=0;i<SECTOR;i++){uint8_t t=newest[i];newest[i]=other[i];other[i]=t;}
  w->active_meta=1u;
 } else w->active_meta=0u;
 w->device.index_address=w->active_meta*SECTOR;
 w->device.generation=g32(newest+8);
 return 0;
}
/* For references in two indices: duplicates are normal. Build owner map from
 * newest, then import older using a non-destructive reference union. */
static int owner_union(struct beetle_v2_writer *w,const uint8_t *latest,const uint8_t *older){
 memset(w->domain,0,sizeof(w->domain));memset(w->occupied,0,sizeof(w->occupied));
 /* mark() is stricter than union: allow duplicate page references if both
  * superblocks point at exactly the same immutable record generation. */
 const uint8_t *idxs[2]={latest,older};
 for(int n=0;n<2;n++){
  const uint8_t *idx=idxs[n];if(!valid(idx))continue;
  uint16_t local[72]={0};
  for(unsigned slot=0;slot<12;slot++)for(unsigned bank=0;bank<2;bank++){
   const uint8_t *r=idx+20u+slot*32u+bank*16u;
   uint32_t off=g32(r+4),len=g32(r+8);
   if(!g32(r))continue;
   if(off<8192u||off%PAGE||len<16u||len>12272u||len>CAP-24u||off>CAP-24u-len)return -3;
   uint32_t end=off+((len+24u+255u)&~255u);if(end>CAP)return -3;
   for(uint32_t pg=off/PAGE;pg<end/PAGE;pg++){
    uint32_t sec=pg/16u;uint16_t bit=(uint16_t)(1u<<(pg%16u));
    if(w->domain[sec]&&w->domain[sec]!=1u)return -3;
    if(local[sec]&bit)return -3;
    w->domain[sec]=1u;w->occupied[sec]|=bit;local[sec]|=bit;
   }
  }
  uint32_t pos=20u+384u;
  for(unsigned slot=0;slot<13;slot++){
   uint32_t gen=g32(idx+pos),total=g32(idx+pos+4),cnt=g16(idx+pos+12);pos+=14u;
   uint32_t sum=0u;
   for(uint32_t k=0;k<cnt;k++){
    uint32_t off=g32(idx+pos),len=g32(idx+pos+4);pos+=12u;
    if(!gen||off<8192u||off%PAGE||!len||len>MAX_CHUNK||
       len>CAP-24u||off>CAP-24u-len||
       off/SECTOR!=(off+((len+24u+255u)&~255u)-1u)/SECTOR)return -3;
    if(sum>total||len>total-sum)return -3;
    sum+=len;
    for(uint32_t pg=off/PAGE;pg<(off+((len+24u+255u)&~255u))/PAGE;pg++){
     uint32_t sec=pg/16u;uint16_t bit=(uint16_t)(1u<<(pg%16u));
     if(w->domain[sec]&&w->domain[sec]!=2u)return -3;
     if(local[sec]&bit)return -3;
     w->domain[sec]=2u;w->occupied[sec]|=bit;local[sec]|=bit;
    }
   }
   if(gen&&sum!=total)return -3;
  }
 }
 return 0;
}
int beetle_v2_writer_open(struct beetle_v2_writer *w,beetle_v2_read read,
 beetle_v2_program program,beetle_v2_erase erase,void *ctx,uint8_t *a,uint8_t *b){
 if(!w||!read||!program||!erase||!a||!b)return -1;
 memset(w,0,sizeof(*w));w->device.read=read;w->device.context=ctx;
 w->program=program;w->erase=erase;w->context=ctx;
 int rc=refresh(w,a,b);if(rc)return rc;
 return owner_union(w,a,b);
}
static int page_blank(struct beetle_v2_writer *w,uint32_t off,uint8_t *page){
 if(rd(w,off,page,PAGE))return -1;
 for(unsigned i=0;i<PAGE;i++)if(page[i]!=0xff)return 0;
 return 1;
}
static int reserve(struct beetle_v2_writer *w,uint8_t domain,uint32_t n,uint8_t *scratch,uint32_t *found){
 uint32_t need=(n+24u+255u)&~255u;
 if(domain==2u && need>SECTOR)return -2;
 if(need>CAP-8192u)return -2;
 uint32_t start=domain==1u?8192u:CAP-need;
 for(uint32_t at=start;;){
  uint32_t end=at+need;
  if(end<=CAP&&((domain!=2u)||(at/SECTOR==(end-1u)/SECTOR))){
   int ok=1;
   for(uint32_t pg=at/PAGE;pg<end/PAGE;pg++){
    uint32_t sec=pg/16u;
    if((w->domain[sec]&&w->domain[sec]!=domain)||
       (w->occupied[sec]&(1u<<(pg%16u)))){ok=0;break;}
    /* An ownerless sector containing orphan pages from an interrupted
     * transaction is NOT eligible for reassignment to another namespace.
     * It must first be garbage-collected as a whole. */
    if(w->domain[sec]==0u){
     for(uint32_t sp=0;sp<16u;sp++){
      int sb=page_blank(w,sec*SECTOR+sp*PAGE,scratch);
      if(sb<0)return -1;
      if(sb==0){ok=0;break;}
     }
     if(!ok)break;
    }
    int blank=page_blank(w,pg*PAGE,scratch);
    if(blank<0)return -1;
    if(!blank){ok=0;break;}
   }
   if(ok){
    for(uint32_t pg=at/PAGE;pg<end/PAGE;pg++){
     uint32_t sec=pg/16u;
     w->domain[sec]=domain;w->occupied[sec]|=(uint16_t)(1u<<(pg%16u));
    }
    *found=at;return 0;
   }
  }
  if(domain==1u){if(at>CAP-need-PAGE)break;at+=PAGE;}
  else {if(at<8192u+PAGE)break;at-=PAGE;}
 }
 return -2;
}
/* Must retain four COMPLETE erased sectors, even when there are spare pages in
 * sectors already containing project/sample records. */
static int guard(struct beetle_v2_writer *w,uint8_t *page){
 unsigned remain=0;
 for(unsigned sec=2;sec<NSEC;sec++){
  if(w->domain[sec])continue;
  int all=1;
  for(unsigned j=0;j<16;j++){
   int blank=page_blank(w,sec*SECTOR+j*PAGE,page);
   if(blank<0)return -1;
   if(!blank){all=0;break;}
  }
  remain+=(unsigned)all;
 }
 return remain>=BEETLE_V2_GUARD_SECTORS?0:-2;
}
static int write_record(struct beetle_v2_writer *w,uint8_t kind,uint8_t slot,
 uint16_t part,uint32_t gen,const uint8_t *data,uint32_t len,
 uint32_t off,uint8_t *page){
 uint32_t check=crc(data,len),head_crc;
 uint8_t head[24];memcpy(head,"B2RC",4);head[4]=kind;head[5]=slot;
 s16(head+6,part);s32(head+8,gen);s32(head+12,len);s32(head+16,check);
 head_crc=crc(head,20);s32(head+20,head_crc);
 uint32_t total=(24u+len+255u)&~255u;
 for(uint32_t p=0;p<total;p+=PAGE){
  memset(page,0xff,PAGE);
  for(uint32_t j=0;j<PAGE;j++){
   uint32_t k=p+j;
   if(k<24u)page[j]=head[k];
   else if(k-24u<len)page[j]=data[k-24u];
  }
  if(pr(w,off+p,page,PAGE))return -1;
 }
 /* Read-back CRC in bounded chunks (outside the audio callback). */
 uint8_t verify[256];
 if(rd(w,off,verify,24u)||memcmp(verify,head,24u))return -4;
 uint32_t c=0xffffffffu;
 for(uint32_t p=0;p<len;p+=PAGE){
  uint32_t count=len-p;if(count>PAGE)count=PAGE;
  if(rd(w,off+24u+p,verify,count))return -1;
  c=crc_inc(c,verify,count);
 }
 return (~c)==check?0:-4;
}
static void header_crc(uint8_t *idx){
 uint32_t n=g32(idx+12);uint32_t x=crc_inc(0xffffffffu,idx,16u);
 x=crc_inc(x,idx+20,n);s32(idx+16,~x);
}
static int commit(struct beetle_v2_writer *w,uint8_t *idx,uint8_t *other){
 uint32_t oldgen=w->device.generation,target=(1u-w->active_meta)*SECTOR;
 s32(idx+8,oldgen+1u);header_crc(idx);
 if(!valid(idx))return -3;
 /* ALL records are already written+verified. Keep the older metadata valid
  * while programming the new generation. Marker is the last page write. */
 if(er(w,target/SECTOR))return -1;
 for(uint32_t p=0;p<SECTOR-PAGE;p+=PAGE){
  if(pr(w,target+p,idx+p,PAGE))return -1;
 }
 if(pr(w,target+SECTOR-PAGE,idx+SECTOR-PAGE,PAGE))return -1;
 if(rd(w,target,other,SECTOR))return -1;
 if(memcmp(idx,other,SECTOR)||!valid(other))return -4;
 w->active_meta=(uint8_t)(target/SECTOR);
 w->device.generation=oldgen+1u;
 w->device.index_address=target;
 return 0;
}
int beetle_v2_write_project(struct beetle_v2_writer *w,uint8_t slot,
 const uint8_t *data,uint32_t len,uint8_t *idx,uint8_t *other,uint8_t page[256]){
 if(!w||slot>=12u||!data||len<16u||len>12272u||!idx||!other||!page)return -3;
 int rc=refresh(w,idx,other);if(rc)return rc;
 rc=owner_union(w,idx,other);if(rc)return rc;
 uint32_t slot_off=20u+(uint32_t)slot*32u;
 uint32_t oldgen=g32(idx+slot_off),gen=oldgen?oldgen+1u:1u;
 uint32_t off;
 rc=reserve(w,1u,len,page,&off);if(rc)return rc;
 rc=guard(w,page);if(rc)return rc;
 rc=write_record(w,1u,slot,0u,gen,data,len,off,page);if(rc)return rc;
 move_bytes(idx+slot_off+16u,idx+slot_off,16u);
 s32(idx+slot_off,gen);s32(idx+slot_off+4,off);
 s32(idx+slot_off+8,len);s32(idx+slot_off+12,crc(data,len));
 return commit(w,idx,other);
}
int beetle_v2_write_sample(struct beetle_v2_writer *w,uint8_t slot,
 const uint8_t *data,uint32_t len,uint8_t *idx,uint8_t *other,uint8_t page[256]){
 if(!w||slot>=13u||!data||!len||!idx||!other||!page)return -3;
 if(len>MAX_CHUNK*128u)return -2;
 uint32_t count=(len+MAX_CHUNK-1u)/MAX_CHUNK;
 if(count>128u)return -2;
 int rc=refresh(w,idx,other);if(rc)return rc;
 rc=owner_union(w,idx,other);if(rc)return rc;
 uint32_t pos=20u+384u;
 for(unsigned i=0;i<slot;i++)pos+=14u+12u*g16(idx+pos+12);
 uint32_t oldcnt=g16(idx+pos+12),tail=pos+14u+oldcnt*12u;
 uint32_t end=20u+g32(idx+12),new_end=end-oldcnt*12u+count*12u;
 if(new_end>SECTOR-4u)return -2;
 uint32_t gen=g32(idx+pos)?g32(idx+pos)+1u:1u;
 /* Build trial index before writing; its total CRC is filled at commit. */
 move_bytes(idx+pos+14u+count*12u,idx+tail,end-tail);
 s32(idx+pos,gen);s32(idx+pos+4,len);s32(idx+pos+8,crc(data,len));s16(idx+pos+12,(uint16_t)count);
 s32(idx+12,new_end-20u);
 /* FULL preflight before any NOR mutation. Do not strand partial writes
  * merely because a later sample chunk cannot fit. This requires only a
  * bounded 128 x 32-bit offsets on the main-thread stack (512 bytes). */
 uint32_t planned[128];
 for(uint32_t i=0;i<count;i++){
  uint32_t n=len-i*MAX_CHUNK;if(n>MAX_CHUNK)n=MAX_CHUNK;
  rc=reserve(w,2u,n,page,&planned[i]);if(rc)return rc;
 }
 rc=guard(w,page);if(rc)return rc;
 /* Caller data must remain immutable until commit, and no voice can be
  * reading from sectors being reallocated during this transaction. */
 for(uint32_t i=0;i<count;i++){
  uint32_t n=len-i*MAX_CHUNK;if(n>MAX_CHUNK)n=MAX_CHUNK;
  rc=write_record(w,2u,slot,(uint16_t)i,gen,data+i*MAX_CHUNK,n,planned[i],page);
  if(rc)return rc;
  uint32_t ref=pos+14u+i*12u;
  s32(idx+ref,planned[i]);s32(idx+ref+4,n);
  s32(idx+ref+8,crc(data+i*MAX_CHUNK,n));
 }
 return commit(w,idx,other);
}
int beetle_v2_remove_project(struct beetle_v2_writer *w,uint8_t slot,
 uint8_t *idx,uint8_t *other,uint8_t page[256]){
 if(!w||slot>=12u||!idx||!other||!page)return -3;
 int rc=refresh(w,idx,other);if(rc)return rc;
 rc=owner_union(w,idx,other);if(rc)return rc;
 memset(idx+20u+(uint32_t)slot*32u,0,32u);
 return commit(w,idx,other);
}
int beetle_v2_remove_sample(struct beetle_v2_writer *w,uint8_t slot,
 uint8_t *idx,uint8_t *other,uint8_t page[256]){
 if(!w||slot>=13u||!idx||!other||!page)return -3;
 int rc=refresh(w,idx,other);if(rc)return rc;
 rc=owner_union(w,idx,other);if(rc)return rc;
 uint32_t pos=20u+384u;
 for(unsigned i=0;i<slot;i++)pos+=14u+12u*g16(idx+pos+12);
 uint32_t count=g16(idx+pos+12);
 uint32_t end=20u+g32(idx+12),old_end=pos+14u+count*12u;
 move_bytes(idx+pos+14u,idx+old_end,end-old_end);
 memset(idx+pos,0,14u); /* don't erase sample metadata of later slots */
 s32(idx+12,g32(idx+12)-count*12u);
 return commit(w,idx,other);
}
int beetle_v2_collect(struct beetle_v2_writer *w,uint8_t *idx,uint8_t *other){
 if(!w||!idx||!other)return -3;
 int rc=refresh(w,idx,other);if(rc)return rc;
 rc=owner_union(w,idx,other);if(rc)return rc;
 int count=0;
 for(unsigned sec=2;sec<NSEC;sec++){
  if(w->domain[sec])continue;
  uint8_t page[256];int dirty=0;
  for(unsigned p=0;p<16;p++){
   int blank=page_blank(w,sec*SECTOR+p*PAGE,page);
   if(blank<0)return -1;
   if(!blank){dirty=1;break;}
  }
  if(dirty){if(er(w,sec))return -1;count++;}
 }
 return count;
}

/* BEETLE original V1.4.21 streaming sample transaction. Destination sectors
 * and B2SB index capacity are fully checked before any program/erase call. */
int beetle_v2_sample_upload_begin(struct beetle_v2_writer *w,
 struct beetle_v2_sample_upload *u,uint8_t slot,uint32_t total,
 uint32_t expected_crc,uint8_t *idx,uint8_t *other,uint8_t page[256]){
 if(!w||!u||!idx||!other||!page||slot>=13u||!total||total>MAX_CHUNK*128u)return -3;
 memset(u,0,sizeof(*u));
 int rc=refresh(w,idx,other);if(rc)return rc;
 rc=owner_union(w,idx,other);if(rc)return rc;
 uint32_t count=(total+MAX_CHUNK-1u)/MAX_CHUNK;
 uint32_t pos=20u+384u;
 for(unsigned i=0;i<slot;i++)pos+=14u+12u*g16(idx+pos+12u);
 uint32_t oldcnt=g16(idx+pos+12u),tail=pos+14u+oldcnt*12u;
 uint32_t end=20u+g32(idx+12u),new_end=end-oldcnt*12u+count*12u;
 if(new_end>SECTOR-4u)return -2;
 for(uint32_t i=0;i<count;i++){
  uint32_t n=total-i*MAX_CHUNK;if(n>MAX_CHUNK)n=MAX_CHUNK;
  rc=reserve(w,2u,n,page,&u->destinations[i]);if(rc)return rc;
 }
 rc=guard(w,page);if(rc)return rc;
 uint32_t oldgen=g32(idx+pos);
 u->generation=oldgen?oldgen+1u:1u;
 if(u->generation==0u)return -3;
 move_bytes(idx+pos+14u+count*12u,idx+tail,end-tail);
 s32(idx+pos,u->generation);s32(idx+pos+4,total);
 s32(idx+pos+8,expected_crc);s16(idx+pos+12u,(uint16_t)count);
 s32(idx+12u,new_end-20u);
 u->total=total;u->expected_crc=expected_crc;u->accumulator=0xffffffffu;
 u->chunks=(uint16_t)count;u->slot=slot;u->index_pos=(uint16_t)pos;
 u->active=1u;
 return 0;
}
int beetle_v2_sample_upload_feed(struct beetle_v2_writer *w,
 struct beetle_v2_sample_upload *u,const uint8_t *data,uint16_t len,
 uint8_t *idx,uint8_t page[256],uint8_t chunk[4068]){
 if(!w||!u||!u->active||!data||!len||!idx||!page||!chunk||
    len>u->total-u->received||u->current>=u->chunks)return -3;
 /* The standard 24-byte SysEx packets can straddle 4068-byte records.
  * Consume each payload in bounded slices without requiring the host to know
  * the V2 flash record layout. */
 while(len){
  uint32_t part=u->current,part_length=u->total-part*MAX_CHUNK;
  if(part_length>MAX_CHUNK)part_length=MAX_CHUNK;
  uint16_t take=len;
  if(take>part_length-u->filled)take=(uint16_t)(part_length-u->filled);
  memcpy(chunk+u->filled,data,take);
  u->filled=(uint16_t)(u->filled+take);
  u->accumulator=crc_inc(u->accumulator,data,take);
  u->received+=take;data+=take;len=(uint16_t)(len-take);
  if(u->filled==part_length){
   int rc=write_record(w,2u,(uint8_t)u->slot,(uint16_t)part,
                       u->generation,chunk,part_length,u->destinations[part],page);
   if(rc){u->active=0u;return rc;}
   uint32_t pos=u->index_pos+14u+part*12u;
   s32(idx+pos,u->destinations[part]);s32(idx+pos+4u,part_length);
   s32(idx+pos+8u,crc(chunk,part_length));
   u->current++;u->filled=0u;
  }
 }
 return 0;
}
/* Validate the complete original BD4S container from the already-written
 * UNCOMMITTED immutable record chunks. No untrusted content is made visible
 * in the V2 index until both the codec and transport CRCs pass. */
static int read_uncommitted(const struct beetle_v2_writer *w,
 const struct beetle_v2_sample_upload *u,uint32_t pos,uint8_t *dst,uint32_t n){
 if(pos>u->total||n>u->total-pos)return -1;
 while(n){
  uint32_t part=pos/MAX_CHUNK,where=pos%MAX_CHUNK;
  if(part>=u->chunks)return -1;
  uint32_t count=u->total-part*MAX_CHUNK;
  if(count>MAX_CHUNK)count=MAX_CHUNK;
  uint32_t take=count-where;if(take>n)take=n;
  if(rd(w,u->destinations[part]+24u+where,dst,take))return -1;
  dst+=take;pos+=take;n-=take;
 }
 return 0;
}
static int validate_uncommitted_bd4s(const struct beetle_v2_writer *w,
 const struct beetle_v2_sample_upload *u){
 uint8_t h[16],block[6],buf[256];
 if(u->total<22u||read_uncommitted(w,u,0u,h,16u))return -4;
 if(memcmp(h,"BD4S",4u)||h[4]!=1u||h[5]!=1u||g16(h+6u)!=22050u)return -4;
 uint32_t frames=g32(h+8u),pos=16u,decoded=0u,check=0xffffffffu;
 if(!frames||frames>441000u)return -4;
 while(decoded<frames){
  if(pos>u->total||u->total-pos<6u||
     read_uncommitted(w,u,pos,block,6u))return -4;
  uint32_t count=g16(block+4u),step=g16(block+2u);
  if(!count||count>256u||count>frames-decoded||step<8u||step>8192u)return -4;
  uint32_t size=6u+count/2u;
  if(size>u->total-pos)return -4;
  uint32_t off=0u;
  while(off<size){
   uint32_t n=size-off;if(n>sizeof(buf))n=sizeof(buf);
   if(read_uncommitted(w,u,pos+off,buf,n))return -4;
   check=crc_inc(check,buf,n);off+=n;
  }
  decoded+=count;pos+=size;
 }
 return pos==u->total && (~check)==g32(h+12u)?0:-4;
}
int beetle_v2_sample_upload_commit(struct beetle_v2_writer *w,
 struct beetle_v2_sample_upload *u,uint8_t *idx,uint8_t *other,
 uint8_t page[256],uint8_t chunk[4068]){
 if(!w||!u||!u->active||!idx||!other||!page||!chunk||
    u->received!=u->total||u->filled||u->current!=u->chunks)return -3;
 if((~u->accumulator)!=u->expected_crc || validate_uncommitted_bd4s(w,u)) {
  u->active=0u;return -4;
 }
 int rc=commit(w,idx,other);
 u->active=0u;
 return rc;
}
void beetle_v2_sample_upload_abort(struct beetle_v2_sample_upload *u){
 if(u)u->active=0u;
}
