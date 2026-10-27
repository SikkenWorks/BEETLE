/* SPDX-License-Identifier: GPL-3.0-only
 * Native host test: all 13 slots, six simultaneous sample voices, bounded
 * prefetch, exact PCM, missing I/O, failed header, and glitch accounting.
 */
#include "beetle_bd4_prefetch.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
struct fixture {uint8_t *data;uint32_t size;int16_t *expected;uint32_t frames;};
struct context {struct fixture slots[13];int in_render,fail_reads;uint32_t reads;};
static uint32_t le32(const uint8_t *p){return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static int read_blob(void *ctx,uint8_t slot,uint32_t off,uint8_t *dst,uint16_t len){
 struct context *c=ctx;
 assert(!c->in_render); /* No flash/storage reads inside audio output */
 c->reads++;
 if(c->fail_reads || slot>=13||off>c->slots[slot].size||len>c->slots[slot].size-off)return 0;
 memcpy(dst,c->slots[slot].data+off,len);return 1;
}
static uint8_t *file(const char *path,size_t *size){
 FILE *f=fopen(path,"rb");if(!f){perror(path);exit(1);}fseek(f,0,SEEK_END);
 long n=ftell(f);rewind(f);assert(n>0&&n<2000000);
 uint8_t *b=malloc((size_t)n);assert(b&&fread(b,1,(size_t)n,f)==(size_t)n);
 fclose(f);*size=(size_t)n;return b;
}
static void normal(struct context *c){
 struct beetle_bd4_stream s;beetle_bd4_stream_reset(&s);
 for(unsigned i=0;i<6;i++){
  assert(beetle_bd4_stream_prepare(&s,i,(uint8_t)i,c->slots[i].size)==0);
  assert(beetle_bd4_stream_start(&s,i)==0); /* no trigger before warming */
 }
 assert(beetle_bd4_prefetch_poll(&s,read_blob,c,12)==12);
 for(unsigned i=0;i<6;i++)assert(beetle_bd4_stream_start(&s,i)==1);
 uint32_t read_before=c->reads;
 int16_t out[128];
 uint32_t max_frames=0;for(unsigned i=0;i<6;i++)if(max_frames<c->slots[i].frames)max_frames=c->slots[i].frames;
 for(uint32_t pos=0;pos<max_frames*2u;pos+=128u){
  /* Main loop has bounded scheduled work; refill before each audio block. */
  assert(beetle_bd4_prefetch_poll(&s,read_blob,c,6)>=0);
  for(unsigned i=0;i<6;i++){
   c->in_render=1;beetle_bd4_stream_render(&s,i,out,128u);c->in_render=0;
   for(uint32_t k=0;k<128;k++){
    uint32_t t=pos+k,src=t/2;
    int32_t want=0;
    if(src<c->slots[i].frames){
     int32_t a=c->slots[i].expected[src];
     int32_t b=c->slots[i].expected[src+1<c->slots[i].frames?src+1:src];
     want=(t&1u)?(a+b)/2:a;
    }
    if(out[k]!=want){fprintf(stderr,"PCM mismatch slot %u frame %u actual %d expected %d\n",i,t,out[k],(int)want);abort();}
   }
  }
 }
 assert(c->reads>read_before);
 for(unsigned i=0;i<6;i++)assert(s.voices[i].underruns==0&&s.voices[i].invalid_blocks==0);
 printf("Six voices, >%u output frames, PCM-exact, zero underruns: PASS\n",max_frames*2u);
}
static void slow_io(struct context *c){
 struct beetle_bd4_stream s;beetle_bd4_stream_reset(&s);
 assert(beetle_bd4_stream_prepare(&s,0,5,c->slots[5].size)==0);
 assert(beetle_bd4_prefetch_poll(&s,read_blob,c,2)==2);
 assert(beetle_bd4_stream_start(&s,0)==1);
 c->fail_reads=1;
 int16_t out[128];
 /* Force consumer to run far beyond the prefetch horizon. */
 for(unsigned j=0;j<5;j++){
  c->in_render=1;beetle_bd4_stream_render(&s,0,out,128);c->in_render=0;
 }
 assert(s.voices[0].underruns==1);
 uint32_t early=s.voices[0].underruns;
 c->fail_reads=0;
 assert(beetle_bd4_prefetch_poll(&s,read_blob,c,8)==2);
 assert(s.voices[0].skipped_frames<=BEETLE_BD4_WINDOW_FRAMES);
 c->in_render=1;beetle_bd4_stream_render(&s,0,out,128);c->in_render=0;
 assert(s.voices[0].underruns==early);
 assert(out[127]!=0); /* refill and resumption after the missed interval */
 printf("Starvation counted; stale frames discarded; refill resumes without render I/O: PASS\n");
}
static void edge_cases(struct context *c){
 for(unsigned slot=6;slot<13;slot++){
  struct beetle_bd4_stream s;beetle_bd4_stream_reset(&s);
  assert(beetle_bd4_stream_prepare(&s,0,(uint8_t)slot,c->slots[slot].size)==0);
  unsigned first=c->slots[slot].frames>BEETLE_BD4_WINDOW_FRAMES?2u:1u;
  assert(beetle_bd4_prefetch_poll(&s,read_blob,c,first)==(int)first);
  assert(beetle_bd4_stream_start(&s,0)==1);
  int16_t out[128];
  uint32_t samples=c->slots[slot].frames*2u;
  for(uint32_t pos=0;pos<samples;pos+=128u){
   assert(beetle_bd4_prefetch_poll(&s,read_blob,c,2)>=0);
   c->in_render=1;beetle_bd4_stream_render(&s,0,out,128);c->in_render=0;
   for(unsigned k=0;k<128;k++){
    uint32_t t=pos+k,frame=t/2;int32_t want=0;
    if(frame<c->slots[slot].frames){
     int32_t a=c->slots[slot].expected[frame];
     int32_t b=c->slots[slot].expected[frame+1<c->slots[slot].frames?frame+1:frame];
     want=(t&1u)?(a+b)/2:a;
    }
    assert(out[k]==want);
   }
  }
  assert(s.voices[0].underruns==0);
 }
 puts("Seven short/edge/20-second BD4S sources, exact interpolation: PASS");
}
static void song_stop(struct context *c){
 struct beetle_bd4_stream s;beetle_bd4_stream_reset(&s);
 for(unsigned i=0;i<6;i++){
  assert(beetle_bd4_stream_prepare(&s,i,(uint8_t)i,c->slots[i].size)==0);
 }
 assert(beetle_bd4_prefetch_poll(&s,read_blob,c,12)==12);
 for(unsigned i=0;i<6;i++)assert(beetle_bd4_stream_start(&s,i)==1);
 int16_t pcm[96];
 for(unsigned i=0;i<6;i++)beetle_bd4_stream_render(&s,i,pcm,24u);
 uint32_t old_reads=c->reads;
 beetle_bd4_stream_choke_all(&s);
 for(unsigned i=0;i<6;i++){
  c->in_render=1;
  beetle_bd4_stream_render(&s,i,pcm,96u);
  c->in_render=0;
  for(unsigned j=48u;j<96u;j++)assert(pcm[j]==0);
  assert(s.voices[i].playing==0u);
 }
 assert(c->reads==old_reads);
 /* Explicitly assigned blank patterns must NOT call choke in song runtime. */
 puts("Six global sample voices obey Song -- STP with 48-frame fade, zero ISR reads: PASS");
}
/* V1.4.28: simulate rapid pattern-loop retriggering across all six drum
 * voices. Samples are independently decoded on each new note, and neither
 * a stale interpolation pair nor a previous voice can leak into the next. */
static void loop_retrigger(struct context *c){
 struct beetle_bd4_stream s;
 beetle_bd4_stream_reset(&s);
 int16_t pcm[128];
 for(unsigned round=0;round<64u;round++){
  for(unsigned voice=0;voice<6u;voice++){
   unsigned slot=(voice+round)%6u;
   assert(beetle_bd4_stream_prepare(&s,voice,(uint8_t)slot,c->slots[slot].size)==0);
  }
  uint32_t before=c->reads;
  assert(beetle_bd4_prefetch_poll(&s,read_blob,c,12u)==12);
  if(round==0){
   /* Header + one complete 256-frame encoded block: only TWO reads for
    * the initial two 128-frame PCM windows (V1.4.27 used three). */
   assert(c->reads-before==12u);
  }
  for(unsigned voice=0;voice<6u;voice++)assert(beetle_bd4_stream_start(&s,voice)==1);
  for(unsigned section=0;section<2u;section++){
   if(section)assert(beetle_bd4_prefetch_poll(&s,read_blob,c,6u)>=0);
   for(unsigned voice=0;voice<6u;voice++){
    const unsigned slot=(voice+round)%6u;
    uint32_t reads=c->reads;
    c->in_render=1;
    beetle_bd4_stream_render(&s,voice,pcm,128u);
    c->in_render=0;
    assert(c->reads==reads);
    for(unsigned frame=0;frame<128u;frame++){
     unsigned time=section*128u+frame,at=time>>1;
     int32_t a=c->slots[slot].expected[at],b=c->slots[slot].expected[at+1u];
     int32_t expected=(time&1u)?(a+b)/2:a;
     assert(pcm[frame]==expected);
    }
    assert(s.voices[voice].underruns==0u);
   }
  }
 }
 puts("64 pattern loops x 6 retriggered voices: PCM-exact, no I/O in render, zero underruns: PASS");
}
static void corrupt(struct context *c){
 struct beetle_bd4_stream s;beetle_bd4_stream_reset(&s);
 assert(beetle_bd4_stream_prepare(&s,0,0,c->slots[0].size)==0);
 uint8_t original=c->slots[0].data[0];c->slots[0].data[0]^=0x55;
 assert(beetle_bd4_prefetch_poll(&s,read_blob,c,1)==0);
 assert(s.voices[0].invalid_blocks>0&&!s.voices[0].configured);
 c->slots[0].data[0]=original;
 assert(beetle_bd4_stream_start(&s,0)==0);
 printf("Bad BD4S header rejected safely: PASS\n");
}
int main(int argc,char **argv){
 assert(argc==27);
 struct context c={0};
 for(unsigned i=0;i<13;i++){
  size_t len,pl;
  c.slots[i].data=file(argv[1+i*2],&len);
  uint8_t *pcm=file(argv[2+i*2],&pl);
  assert(len<=300000u&&pl%2==0);
  c.slots[i].size=(uint32_t)len;
  c.slots[i].frames=(uint32_t)pl/2u;
  assert(le32(c.slots[i].data+8)==c.slots[i].frames);
  c.slots[i].expected=malloc(pl);assert(c.slots[i].expected);
  for(unsigned j=0;j<pl/2;j++)c.slots[i].expected[j]=(int16_t)((uint16_t)pcm[j*2]|((uint16_t)pcm[j*2+1]<<8));
  free(pcm);
 }
 normal(&c);edge_cases(&c);slow_io(&c);corrupt(&c);song_stop(&c);loop_retrigger(&c);
 assert(beetle_bd4_stream_prepare(NULL,0,0,4000)==-1);
 assert(beetle_bd4_stream_prepare(&(struct beetle_bd4_stream){0},0,13,4000)==-1);
 for(unsigned i=0;i<13;i++){free(c.slots[i].data);free(c.slots[i].expected);}
 printf("BEETLE fixed 6-voice working state: %zu bytes\n",sizeof(struct beetle_bd4_stream));
 puts("BEETLE V1.4.9 prefetch native regression PASS");return 0;
}
