/* GPL-3.0-only: the synth clipboard must not leak into CH6 NOTE3. */
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "../../firmware/fm1-native/track_lane_clipboard.h"
int main(void){
 struct track_pattern p;struct track_lane_clipboard c;
 memset(&p,0,sizeof p);memset(&c,0x62,sizeof c);
 p.length=TRACK_STEPS;
 for(unsigned i=0;i<TRACK_STEPS;i++){
  p.ch[0][i].note=(uint8_t)i;
  p.ch[TRACK_DRUM_CHANNEL][i].note=10u;
  p.note3[i]=(uint8_t)(i%27u);
 }
 assert(track_lane_clip_copy(&c,&p,0u));
 for(unsigned i=0;i<TRACK_STEPS;i++) assert(c.note3[i]==TRACK_NOTE_EMPTY);
 assert(track_lane_clip_paste(&c,&p,2u));
 for(unsigned i=0;i<TRACK_STEPS;i++)assert(p.note3[i]==i%27u);
 assert(track_lane_clip_copy_range(&c,&p,0u,4u,11u));
 assert(track_lane_clip_paste_at(&c,&p,1u,3u));
 for(unsigned i=0;i<TRACK_STEPS;i++)assert(p.note3[i]==i%27u);
 assert(track_lane_clip_copy(&c,&p,TRACK_DRUM_CHANNEL));
 for(unsigned i=0;i<TRACK_STEPS;i++) assert(c.note3[i]==i%27u);
 for(unsigned i=0;i<TRACK_STEPS;i++)p.note3[i]=TRACK_NOTE_EMPTY;
 assert(track_lane_clip_paste(&c,&p,TRACK_DRUM_CHANNEL));
 for(unsigned i=0;i<TRACK_STEPS;i++)assert(p.note3[i]==i%27u);
 puts("PASS V1.6.11: synth lane copy leaves CH6 NOTE3 unchanged; drum copy preserves NOTE3");
 return 0;
}
