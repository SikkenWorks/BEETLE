/* SPDX-License-Identifier: GPL-3.0-only
 * V1.4.4: NOTE3/step-1 initial window refill gate.
 * No decoder skip / no DMA I/O. This exercise uses a synthetic user-sample
 * window timeline, so it does not require a device or flash image.
 */
#include "beetle_bd4_prefetch.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static void fill(struct beetle_bd4_window *w,unsigned first,unsigned count){
 memset(w,0,sizeof(*w));w->first_frame=first;w->count=(uint16_t)count;
 for(unsigned i=0;i<count;i++)w->pcm[i]=(int16_t)(1000u+first+i);
 w->valid=1;
}
int main(void){
 struct beetle_bd4_stream stream;memset(&stream,0,sizeof(stream));
 struct beetle_bd4_stream_voice *v=&stream.voices[0];
 v->configured=1;v->verified_header=1;v->playing=1;v->frame_count=130;
 fill(&v->windows[0],0,64);fill(&v->windows[1],64,64);
 for(unsigned i=0;i<254u;i++){
  int ready=beetle_bd4_stream_ready_now(&stream,0);
  if(!ready)break;
  int16_t sample=0;beetle_bd4_stream_render(&stream,0,&sample,1);
  const unsigned frame=i/2;
  const int want=(i&1u)?(2000u+2u*frame+1u)/2:(1000u+frame);
  assert(sample==want);
 }
 assert(v->output_frames==254u); /* frame 127 cannot interpolate frame 128 yet */
 for(unsigned i=0;i<5000;i++){
  assert(beetle_bd4_stream_ready_now(&stream,0)==0);
  assert(v->output_frames==254u);
 }
 assert(!v->windows[0].valid); /* earlier 0..63 window returned to producer */
 fill(&v->windows[0],128,2);
 assert(beetle_bd4_stream_ready_now(&stream,0)==1);
 for(unsigned i=254u;i<260u;i++){
  assert(beetle_bd4_stream_ready_now(&stream,0)==1);
  int16_t sample=0;beetle_bd4_stream_render(&stream,0,&sample,1);
  const unsigned frame=i/2;
  const int want=(i&1u)?(2000u+2u*frame+1u)/2:(1000u+frame);
  assert(sample==want);
 }
 assert(v->output_frames==260u);
 assert(beetle_bd4_stream_ready_now(&stream,0)==-1);
 puts("PASS: missing first refill cannot skip early NOTE3 sample frames");
 return 0;
}
