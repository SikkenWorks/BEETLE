/* V0.65: sample-boundary live mute queue is transient and bounded. */
#include "beetle_quantized_mute.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 struct track_mixer m={0};struct beetle_mute_queue q={0};
 for(unsigned c=0;c<TRACK_CHANNELS;c++)m.ch[c].volume=80;
 assert(sizeof(q)==2);
 assert(beetle_mute_toggle(&q,&m,0));assert(q.pending==1&&m.ch[0].mute==0);
 assert(beetle_mute_next(&q,&m,0)==1);
 assert(beetle_mute_toggle(&q,&m,0));assert(!q.pending&&m.ch[0].mute==0);
 assert(beetle_mute_arm(&q,&m,1,1));assert(beetle_mute_arm(&q,&m,5,1));
 assert(q.pending==(1u<<1 | 1u<<5));
 assert(beetle_mute_commit(&q,&m));assert(!q.pending);
 assert(m.ch[1].mute==1&&m.ch[5].mute==1&&m.ch[0].mute==0);
 assert(!beetle_mute_commit(&q,&m));
 assert(beetle_mute_arm(&q,&m,5,0));beetle_mute_cancel(&q);
 assert(m.ch[5].mute==1 && !q.pending);
 assert(!beetle_mute_arm(&q,&m,10,1));
 assert(beetle_mute_arm(&q,&m,1,0));assert(beetle_mute_commit(&q,&m));
 assert(!m.ch[1].mute && m.ch[5].mute);
 puts("PASS V0.65 queue/toggle/cancel/commit, six lanes, mixer persistence isolated");
 return 0;
}
