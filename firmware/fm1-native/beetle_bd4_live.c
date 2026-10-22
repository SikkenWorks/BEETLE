/* SPDX-License-Identifier: GPL-3.0-only
 * BEETLE live DPCM4 bridge. Six sample voices REUSE the six drum voices;
 * no seventh-to-twelfth drum voices are silently added. Main-loop prefetch
 * verifies a full V2 record once per voice/sector. DMA renders RAM only.
 */
#include "beetle_bd4_live.h"
#include "beetle_bd4_prefetch.h"
#include <string.h>
#define BD4_CHUNK 4068u
/* 6 * 4068-byte immutable sector caches; every cache retains its sample
 * generation. The PCM voice windows are separately bounded. */
struct live_chunk {uint32_t generation;uint16_t part;uint8_t slot,valid;};
struct live_note {volatile uint32_t serial;volatile uint8_t pending,slot;uint8_t starting;};
/* The existing project staging region is used only when NO project load,
 * save, preview or sample-transfer operation is in flight. This ownership
 * contract is enforced by the caller; the audio DMA is paused on reuse. */
struct live_runtime {
 struct beetle_bd4_stream stream;
 struct live_chunk chunks[BEETLE_BD4_STREAM_VOICES];
 struct live_note notes[BEETLE_BD4_STREAM_VOICES];
 struct beetle_v2_sample slots[BEETLE_BD4_STREAM_SLOTS];
 struct beetle_v2_device device;
 uint8_t index[4096];
 uint8_t chunk_scratch[4068];
 uint8_t valid,refresh;
 uint32_t failed_reads;
};
static struct live_runtime *live_rt;
#define live_stream (live_rt->stream)
#define live_chunks (live_rt->chunks)
#define live_notes (live_rt->notes)
#define live_slots (live_rt->slots)
#define live_device (live_rt->device)
#define live_index (live_rt->index)
#define live_chunk_scratch (live_rt->chunk_scratch)
#define live_valid (live_rt->valid)
#define live_refresh (live_rt->refresh)
#define live_failed_reads (live_rt->failed_reads)
void beetle_bd4_live_bind(void *workspace,uint32_t bytes){
 live_rt=(workspace&&bytes>=sizeof(struct live_runtime))?
       (struct live_runtime *)workspace:0;
 if(live_rt){memset(live_rt,0,sizeof(*live_rt));live_rt->refresh=1u;}
}
void beetle_bd4_live_init(void){
 if(live_rt)beetle_bd4_live_bind(live_rt,sizeof(*live_rt));
}
void beetle_bd4_live_invalidate(void){if(live_rt){live_refresh=1u;live_valid=0u;}}
int beetle_bd4_live_available(uint8_t slot){
 return live_rt&&slot<BEETLE_BD4_STREAM_SLOTS && live_valid && live_slots[slot].generation!=0u;
}
void beetle_bd4_live_request(uint8_t voice,uint8_t slot,uint32_t serial){
 if(!live_rt||voice>=BEETLE_BD4_STREAM_VOICES||!beetle_bd4_live_available(slot))return;
 struct live_note *n=&live_notes[voice];
 n->serial=serial;n->slot=slot;
 n->pending=1u;n->starting=1u;
}
int beetle_bd4_live_next(uint8_t voice,uint32_t serial,int16_t *sample){
 if(!live_rt||!sample||voice>=BEETLE_BD4_STREAM_VOICES)return -1;
 *sample=0;
 struct live_note *n=&live_notes[voice];
 if(n->serial!=serial)return -1;
 if(n->pending||n->starting)return 0;
 if(!live_stream.voices[voice].playing)return -1;
 beetle_bd4_stream_render(&live_stream,voice,sample,1u);
 return 1;
}
void beetle_bd4_live_stop(uint8_t voice){
 if(!live_rt||voice>=BEETLE_BD4_STREAM_VOICES)return;
 live_notes[voice].serial=0u;
 live_notes[voice].pending=0u;
 live_notes[voice].starting=0u;
 beetle_bd4_stream_choke_voice(&live_stream,voice);
}
void beetle_bd4_live_choke_all(void){
 if(!live_rt)return;
 for(unsigned i=0;i<BEETLE_BD4_STREAM_VOICES;i++){
  live_notes[i].pending=0u;live_notes[i].starting=0u;
  beetle_bd4_stream_choke_voice(&live_stream,i);
 }
}
uint32_t beetle_bd4_live_underruns(void){
 if(!live_rt)return 0u;
 uint32_t n=live_failed_reads;
 for(unsigned i=0;i<BEETLE_BD4_STREAM_VOICES;i++)n+=live_stream.voices[i].underruns;
 return n;
}
/* main-loop ONLY. The index and sector CRCs are checked outside audio IRQ.
 * This callback cannot silently follow a replaced U-slot generation. */
static int live_read(void *unused,uint8_t voice,uint8_t slot,uint32_t generation,
 uint32_t offset,uint8_t *dst,uint16_t size){
 (void)unused;
 if(!live_valid||voice>=6u||slot>=13u||!dst||!size||
    !live_slots[slot].generation||generation!=live_slots[slot].generation||
    offset>live_slots[slot].byte_length||size>live_slots[slot].byte_length-offset)return 0;
 struct live_chunk *c=&live_chunks[voice];
 while(size){
  uint16_t part=(uint16_t)(offset/BD4_CHUNK);
  uint16_t within=(uint16_t)(offset%BD4_CHUNK);
  uint16_t count=(uint16_t)(BD4_CHUNK-within);
  if(count>size)count=size;
  if(!c->valid||c->part!=part||c->slot!=slot||c->generation!=generation){
   c->valid=0u;
   uint16_t verified_length=0u;
   if(beetle_v2_sample_chunk_read(&live_device,slot,&live_slots[slot],part,
       live_chunk_scratch,&verified_length,live_index)){
    live_failed_reads++;return 0;
   }
   c->part=part;c->slot=slot;c->generation=generation;c->valid=1u;
  }
  uint32_t q=(uint32_t)live_slots[slot].index_offset+(uint32_t)part*12u;
  uint32_t rel=(uint32_t)live_index[q] | ((uint32_t)live_index[q+1]<<8) |
               ((uint32_t)live_index[q+2]<<16)|((uint32_t)live_index[q+3]<<24);
  uint32_t bytes=(uint32_t)live_index[q+4] | ((uint32_t)live_index[q+5]<<8) |
               ((uint32_t)live_index[q+6]<<16)|((uint32_t)live_index[q+7]<<24);
  if(bytes>BD4_CHUNK||within>bytes||count>bytes-within||
     rel<8192u||rel>BEETLE_V2_BYTES-24u-bytes||
     live_device.read(live_device.context,BEETLE_V2_BASE+rel+24u+within,dst,count)){
    c->valid=0u;return 0;
  }
  offset+=count;dst+=count;size=(uint16_t)(size-count);
 }
 return 1;
}
/* Poll is intentionally bounded: one window per voice per invocation.
 * Audio interrupt never accesses V2 storage, USB or DPCM4 decode. */
void beetle_bd4_live_poll(beetle_v2_read read,void *ctx,void (*lock)(void),void (*unlock)(void)){
 if(!read||!live_rt||!lock||!unlock)return;
 if(live_refresh){
  /* Installation/migration required: refuses the legacy layout. */
  live_valid=0u;memset(live_slots,0,sizeof(live_slots));
  if(beetle_v2_open(&live_device,read,ctx,live_index)==0){
   live_valid=1u;
   for(uint8_t i=0;i<13u;i++){
    struct beetle_v2_sample info;
    if(beetle_v2_sample_info(&live_device,i,&info,live_index)==1)
     live_slots[i]=info;
   }
  }
  live_refresh=0u;
 }
 if(!live_valid)return;
 for(unsigned i=0;i<6u;i++){
  struct live_note *n=&live_notes[i];
  if(n->pending){
   uint32_t seq=n->serial;uint8_t slot=n->slot;
   n->pending=0u;
   live_chunks[i].valid=0u;
   if(!beetle_bd4_live_available(slot)){
    n->starting=0u;n->serial=0u;continue;
   }
   lock();
   int prepare=beetle_bd4_stream_prepare_pinned(&live_stream,i,slot,
     live_slots[slot].byte_length,live_slots[slot].generation);
   unlock();
   if(prepare){n->starting=0u;n->serial=0u;continue;}
   /* An IRQ may have retriggered the same drum voice during this setup.
    * Its newer serial/pending flag must win. */
   if(n->serial!=seq||n->pending)continue;
  }
 }
 (void)beetle_bd4_prefetch_poll_pinned(&live_stream,live_read,0,6u);
 for(unsigned i=0;i<6u;i++){
  struct live_note *n=&live_notes[i];
  if(n->starting&&!n->pending){
   lock();
   if(beetle_bd4_stream_start(&live_stream,i))n->starting=0u;
   unlock();
  }
 }
}
