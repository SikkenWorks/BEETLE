#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "beetle_step_selection.h"
#include "track_drum.h"
#include "track_lane_clipboard.h"
int main(void){
 uint8_t active=0,anchor=0,end=0;
 struct track_pattern pat={0};
 struct track_lane_clipboard clip={0};
 pat.length=16;
 for(uint8_t row=0;row<16;row++){
  pat.ch[0][row].note=(uint8_t)(60u+row);
  pat.ch[0][row].chord=(uint8_t)(row%TRACK_CHORD_COUNT);
  pat.ch[0][row].effect=(uint8_t)(row%6u);
  pat.ch[TRACK_DRUM_CHANNEL][row].note=row;
  pat.ch[TRACK_DRUM_CHANNEL][row].chord=(uint8_t)((row%14u)+1u);
  pat.note2_fx[row].effect=(uint8_t)(row%6u);
 }
 for(uint8_t start=0;start<16;start++){
  for(uint8_t last=0;last<16;last++){
   beetle_step_selection_arm(&active,&anchor,&end,start);
   assert(!active && anchor==start && end==start);
   beetle_step_selection_extend(&active,&end,last);
   assert(active && anchor==start && end==last);
   uint8_t a=anchor,b=end;if(a>b){uint8_t t=a;a=b;b=t;}
   assert(track_lane_clip_copy_range(&clip,&pat,0,a,b));
   assert(clip.count==b-a+1u);
   assert(clip.step[0].note==60u+a);
   assert(clip.step[clip.count-1u].note==60u+b);
   assert(clip.step[0].chord==a%TRACK_CHORD_COUNT);
   assert(track_lane_clip_copy_range(&clip,&pat,TRACK_DRUM_CHANNEL,a,b));
   assert(clip.step[0].note==a && clip.step[0].chord==(a%14u)+1u);
   assert(clip.note2_fx[0].effect==a%6u);
   assert(clip.note2_fx[clip.count-1u].effect==b%6u);
  }
 }
 puts("PASS: all 256 anchored selections are inclusive in both directions; synth and drum NOTE1/NOTE2 FX retained.");
 return 0;
}
