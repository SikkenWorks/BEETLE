/* SPDX-License-Identifier: GPL-3.0-only
 * BEETLE BD4S prefetch/PCM streaming, SikkenWorks.
 * Compiled into Storage V2 native firmware, with flash I/O restricted to the
 * main-loop prefetch; the audio renderer only reads bounded RAM windows.
 */
#include "beetle_bd4_prefetch.h"
#include <string.h>
struct bd4_reader_binding {
 beetle_bd4_blob_reader legacy;
 beetle_bd4_pinned_reader pinned;
 void *context;
 uint8_t voice_id;
};
static uint16_t word(const uint8_t *p){return (uint16_t)((uint16_t)p[0]|((uint16_t)p[1]<<8));}
static uint32_t dword(const uint8_t *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static int32_t clamp(int32_t v,int32_t lo,int32_t hi){return v<lo?lo:(v>hi?hi:v);}
static int32_t advance_step(int32_t s,int32_t magnitude){
 if(magnitude<=1)s-=s/16;
 else if(magnitude>=5)s+=(s*(magnitude-4))/4;
 return clamp(s,8,8192);
}
void beetle_bd4_stream_reset(struct beetle_bd4_stream *s){if(s)memset(s,0,sizeof(*s));}
int beetle_bd4_stream_prepare(struct beetle_bd4_stream *s,unsigned voice,uint8_t slot,uint32_t length){
 if(!s||voice>=BEETLE_BD4_STREAM_VOICES||slot>=BEETLE_BD4_STREAM_SLOTS||length<22u||length>300000u)return -1;
 memset(&s->voices[voice],0,sizeof(s->voices[voice]));
 s->voices[voice].slot=slot;s->voices[voice].byte_length=length;s->voices[voice].configured=1;
 return 0;
}
int beetle_bd4_stream_prepare_pinned(struct beetle_bd4_stream *s,unsigned voice,
                                     uint8_t slot,uint32_t length,uint32_t generation){
 if(!generation)return -1;
 int result=beetle_bd4_stream_prepare(s,voice,slot,length);
 if(result==0){s->voices[voice].generation=generation;s->voices[voice].pinned=1u;}
 return result;
}
static int read_bytes(struct beetle_bd4_stream *s,struct beetle_bd4_stream_voice *v,
                      const struct bd4_reader_binding *io,uint32_t offset,uint8_t *dst,uint16_t n){
 if(offset>v->byte_length||n>v->byte_length-offset||
    (v->pinned ? (!io->pinned||!io->pinned(io->context,io->voice_id,v->slot,v->generation,offset,dst,n))
               : (!io->legacy||!io->legacy(io->context,v->slot,offset,dst,n)))){
  v->invalid_blocks++;s->invalid_reads++;return 0;
 }
 v->reads++;s->background_reads++;return 1;
}
static int fetch_block(struct beetle_bd4_stream *s,struct beetle_bd4_stream_voice *v,
                       const struct bd4_reader_binding *io){
 /* Most sample blocks encode 256 PCM frames (134 bytes including header).
  * Fetch those in one background transaction instead of fetching their
  * 6-byte header and payload separately. Short tail blocks use bounded
  * two-stage reads; no speculative read beyond the verified sample blob. */
 if(v->source_offset>v->byte_length)return 0;
 uint32_t remaining=v->byte_length-v->source_offset;
 if(remaining<6u)return 0;
 uint8_t *head=v->encoded;
 if(remaining>=BEETLE_BD4_BLOCK_BYTES){
  if(!read_bytes(s,v,io,v->source_offset,v->encoded,BEETLE_BD4_BLOCK_BYTES))return 0;
 }else{
  if(!read_bytes(s,v,io,v->source_offset,head,6u))return 0;
 }
 const uint16_t n=word(head+4),step=word(head+2);
 if(!n||n>BEETLE_BD4_BLOCK_FRAMES||step<8u||step>8192u||
    n>v->frame_count-v->decoded_frames)return 0;
 const uint16_t size=(uint16_t)(6u+n/2u);
 if(size>remaining)return 0;
 if(remaining<BEETLE_BD4_BLOCK_BYTES && size>6u &&
    !read_bytes(s,v,io,v->source_offset+6u,v->encoded+6u,
                (uint16_t)(size-6u)))return 0;
 v->source_offset+=size;
 v->encoded_length=(uint8_t)(size-6u);
 v->block_frames=n;v->block_cursor=0u;
 v->predictor=(int16_t)word(head);v->step=step;
 return 1;
}
static int16_t decode_next(struct beetle_bd4_stream_voice *v){
 if(v->block_cursor==0u){v->block_cursor=1u;return (int16_t)v->predictor;}
 const uint32_t nibble=(uint32_t)v->block_cursor-1u;
 uint8_t b=v->encoded[6u+nibble/2u];
 uint8_t code=(uint8_t)((nibble&1u)?b>>4u:b&15u);
 int32_t q=code<8u?(int32_t)code:(int32_t)code-16;
 v->predictor=clamp(v->predictor+q*v->step,-32768,32767);
 v->step=advance_step(v->step,q<0?-q:q);
 v->block_cursor++;
 return (int16_t)v->predictor;
}
static int discard_stale(struct beetle_bd4_stream *s,struct beetle_bd4_stream_voice *v,
                         const struct bd4_reader_binding *io){
 /* After a real underrun, discard already-missed source frames in the
  * background. Otherwise two stale windows can deadlock the stream forever.
  * At most 64 decoded frames per operation, included in the poll budget. */
 uint32_t target=(v->output_frames>>1)&~(BEETLE_BD4_WINDOW_FRAMES-1u);
 if(target>v->frame_count)target=v->frame_count;
 if(v->decoded_frames>=target)return 0;
 uint32_t remaining=target-v->decoded_frames;
 if(remaining>BEETLE_BD4_WINDOW_FRAMES)remaining=BEETLE_BD4_WINDOW_FRAMES;
 for(uint32_t i=0;i<remaining;i++){
  if(v->block_cursor==v->block_frames){if(!fetch_block(s,v,io))return -1;}
  (void)decode_next(v);
  v->decoded_frames++;v->skipped_frames++;
 }
 return 2;
}
static int fill_one(struct beetle_bd4_stream *s,struct beetle_bd4_stream_voice *v,
                    const struct bd4_reader_binding *io){
 if(!v->configured||v->invalid_blocks||(v->verified_header&&v->decoded_frames>=v->frame_count))return 0;
 struct beetle_bd4_window *w=&v->windows[v->next_write];
 if(w->valid)return 0; /* Consumer has not released this window yet. */
 if(!v->verified_header){
  uint8_t h[16];
  if(!read_bytes(s,v,io,0,h,sizeof(h)))return -1;
  if(memcmp(h,"BD4S",4u)!=0||h[4]!=1u||h[5]!=1u||word(h+6)!=22050u||
     !dword(h+8)||dword(h+8)>441000u||v->byte_length<22u)return -1;
  v->frame_count=dword(h+8);v->source_offset=16u;
  v->verified_header=1;
 }
 int stale=discard_stale(s,v,io);
 if(stale)return stale;
 w->first_frame=v->decoded_frames;w->count=0;
 while(w->count<BEETLE_BD4_WINDOW_FRAMES&&v->decoded_frames<v->frame_count){
  if(v->block_cursor==v->block_frames){
   if(!fetch_block(s,v,io))return -1;
  }
  w->pcm[w->count++]=decode_next(v);
  ++v->decoded_frames;
 }
 __asm__ __volatile__("" ::: "memory");
 w->valid=1;
 v->next_write=(uint8_t)((v->next_write+1u)%BEETLE_BD4_WINDOW_COUNT);
 return 1;
}
static int bd4_prefetch_run(struct beetle_bd4_stream *s,
                            const struct bd4_reader_binding *input,unsigned budget){
 if(!s||!input||budget>1024u||(!input->legacy&&!input->pinned))return -1;
 /* Do not let an unpinned callback silently bypass a voice's generation lease. */
 for(unsigned i=0;i<BEETLE_BD4_STREAM_VOICES;i++){
  const struct beetle_bd4_stream_voice *v=&s->voices[i];
  if(v->configured && ((v->pinned!=0u)!=(input->pinned!=0)))return -2;
 }
 int done=0;
 while(budget){
  unsigned made=0;
  for(unsigned i=0;i<BEETLE_BD4_STREAM_VOICES&&budget;i++){
   struct beetle_bd4_stream_voice *v=&s->voices[i];
   struct bd4_reader_binding io=*input;
   io.voice_id=(uint8_t)i;
   int n=fill_one(s,v,&io);
   if(n<0){v->invalid_blocks++;v->configured=0;v->playing=0;continue;}
   if(n){if(n==1)++done;++made;--budget;}
  }
  if(!made)break;
 }
 return done;
}
int beetle_bd4_prefetch_poll(struct beetle_bd4_stream *s,beetle_bd4_blob_reader reader,
                             void *ctx,unsigned budget){
 struct bd4_reader_binding io={reader,0,ctx,0};
 return bd4_prefetch_run(s,&io,budget);
}
int beetle_bd4_prefetch_poll_pinned(struct beetle_bd4_stream *s,
                                    beetle_bd4_pinned_reader reader,
                                    void *ctx,unsigned budget){
 struct bd4_reader_binding io={0,reader,ctx,0};
 return bd4_prefetch_run(s,&io,budget);
}
void beetle_bd4_stream_disarm(struct beetle_bd4_stream *s,unsigned ix){
 if(!s||ix>=BEETLE_BD4_STREAM_VOICES)return;
 memset(&s->voices[ix],0,sizeof(s->voices[ix]));
}
int beetle_bd4_stream_start(struct beetle_bd4_stream *s,unsigned ix){
 if(!s||ix>=BEETLE_BD4_STREAM_VOICES)return 0;
 struct beetle_bd4_stream_voice *v=&s->voices[ix];
 if(!v->configured||!v->verified_header||v->invalid_blocks||v->playing)return 0;
 if(!v->windows[0].valid||
    (!v->windows[1].valid&&v->frame_count>BEETLE_BD4_WINDOW_FRAMES))return 0;
 v->output_frames=0;v->in_gap=0;v->last_output=0;v->fade_in=0;v->choke_frames=0u;v->interp_valid=0u;v->playing=1;
 return 1;
}
void beetle_bd4_stream_choke_voice(struct beetle_bd4_stream *s,unsigned ix){
 if(!s||ix>=BEETLE_BD4_STREAM_VOICES)return;
 struct beetle_bd4_stream_voice *v=&s->voices[ix];
 if(v->playing&&(!v->choke_frames||v->choke_frames>48u))v->choke_frames=48u;
}
void beetle_bd4_stream_choke_all(struct beetle_bd4_stream *s){
 if(!s)return;
 for(unsigned i=0u;i<BEETLE_BD4_STREAM_VOICES;i++)beetle_bd4_stream_choke_voice(s,i);
}
static int locate(struct beetle_bd4_stream_voice *v,uint32_t frame,int16_t *out){
 for(unsigned j=0;j<BEETLE_BD4_WINDOW_COUNT;j++){
  const struct beetle_bd4_window *w=&v->windows[j];
  if(w->valid&&frame>=w->first_frame&&frame-w->first_frame<w->count){
   *out=w->pcm[frame-w->first_frame];return 1;
  }
 }
 return 0;
}
static void release_old(struct beetle_bd4_stream_voice *v,uint32_t now){
 for(unsigned j=0;j<BEETLE_BD4_WINDOW_COUNT;j++){
  struct beetle_bd4_window *w=&v->windows[j];
  if(w->valid&&now>=w->first_frame+w->count)w->valid=0;
 }
}
int beetle_bd4_stream_ready_now(const struct beetle_bd4_stream *s,unsigned ix){
 if(!s||ix>=BEETLE_BD4_STREAM_VOICES)return -1;
 const struct beetle_bd4_stream_voice *v=&s->voices[ix];
 if(!v->configured||!v->playing||v->invalid_blocks)return -1;
 const uint32_t f=v->output_frames>>1u;
 if(f>=v->frame_count)return -1;
 /* Odd frame can use the interpolated pair cached by the previous even frame. */
 if((v->output_frames&1u)&&v->interp_valid)return 1;
 uint8_t a=0,b=(uint8_t)(f+1u>=v->frame_count);
 for(unsigned j=0;j<BEETLE_BD4_WINDOW_COUNT;j++){
  const struct beetle_bd4_window *w=&v->windows[j];
  if(!w->valid)continue;
  if(f>=w->first_frame&&f-w->first_frame<w->count)a=1;
  if(!b&&f+1u>=w->first_frame&&f+1u-w->first_frame<w->count)b=1;
 }
 return (a&&b)?1:0;
}
void beetle_bd4_stream_render(struct beetle_bd4_stream *s,unsigned ix,int16_t *out,uint32_t n){
 if(!out)return;
 if(!s||ix>=BEETLE_BD4_STREAM_VOICES){memset(out,0,(size_t)n*sizeof(*out));return;}
 struct beetle_bd4_stream_voice *v=&s->voices[ix];
 for(uint32_t k=0;k<n;k++){
  int32_t signal=0;
  if(v->playing){
   uint32_t frame=v->output_frames>>1;
   if(frame>=v->frame_count){v->playing=0;}
   else{
    /* Each BD4S frame is played twice (22,050 -> 44,100 Hz). The odd
     * output frame interpolates the SAME source pair fetched for the even
     * frame. The old path re-scanned both PCM windows twice, every time.
     * If the even frame underruns, retry normally on the odd one so a
     * background refill can still recover as soon as it becomes available. */
    int16_t a=0,b=0;
    const uint8_t odd=(uint8_t)(v->output_frames&1u);
    int ok=(odd&&v->interp_valid)?1:0;
    if(ok){a=v->interp_a;b=v->interp_b;}
    else{
     ok=locate(v,frame,&a);
     if(ok){
      if(frame+1u==v->frame_count)b=a;
      else if(!locate(v,frame+1u,&b))ok=0;
     }
     if(!odd){v->interp_a=a;v->interp_b=b;v->interp_valid=(uint8_t)ok;}
    }
    if(ok){
     signal=odd?((int32_t)a+(int32_t)b)/2:(int32_t)a;
     if(v->in_gap){v->in_gap=0;v->fade_in=16u;}
     if(v->fade_in){signal=signal*(int32_t)(17u-v->fade_in)/16;v->fade_in--;}
    }else{
     if(!v->in_gap){v->underruns++;v->in_gap=1;}
     signal=(v->last_output*7)/8; /* bounded click reduction, does not conceal underrun */
    }
    v->output_frames++;
    /* Release a completed 128-frame window before the main-loop refill. */
    release_old(v,v->output_frames>>1);
   }
  }
  if(v->choke_frames){
   signal=(signal*(int32_t)v->choke_frames)/48;
   v->choke_frames--;
   if(!v->choke_frames)v->playing=0u;
  }
  v->last_output=signal;
  out[k]=(int16_t)clamp(signal,-32768,32767);
 }
}
