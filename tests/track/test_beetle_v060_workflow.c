/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "beetle_input_safety.h"
#include "beetle_help.h"
#include "beetle_pattern_queue.h"
int main(void){
 struct beetle_load_input_gate guard={0};
 uint32_t pressed,released;
 beetle_load_input_arm(&guard,100u);
 pressed=BEETLE_SELECT_BUTTON;released=0;
 beetle_load_input_filter(&guard,150u,BEETLE_SELECT_BUTTON,&pressed,&released);
 assert(guard.active && !pressed);
 pressed=0;released=BEETLE_SELECT_BUTTON;
 beetle_load_input_filter(&guard,200u,0u,&pressed,&released);
 assert(guard.active && !released);
 pressed=BEETLE_SELECT_BUTTON;
 beetle_load_input_filter(&guard,450u,0u,&pressed,&released);
 assert(guard.active && !pressed); /* queued stale edge while waiting */
 pressed=BEETLE_SELECT_BUTTON;
 beetle_load_input_filter(&guard,521u,0u,&pressed,&released);
 assert(!guard.active && !pressed); /* current poll also consumed */
 pressed=BEETLE_SELECT_BUTTON;
 beetle_load_input_filter(&guard,540u,BEETLE_SELECT_BUTTON,&pressed,&released);
 assert(pressed==BEETLE_SELECT_BUTTON); /* genuine new press */
 beetle_load_input_arm(&guard,UINT32_MAX-30u);
 pressed=BEETLE_SELECT_BUTTON;
 beetle_load_input_filter(&guard,UINT32_MAX-20u,0u,&pressed,&released);
 beetle_load_input_filter(&guard,350u,0u,&pressed,&released);
 assert(!guard.active); /* wrap-safe clock */
 assert(BEETLE_HELP_COUNT>50);
 assert(beetle_help_heading(0u,1)>0u);
 uint8_t index=0u;
 for(int i=0;i<8;i++){
  uint8_t next=beetle_help_heading(index,1);
  assert(next>=index && next<=beetle_help_last_offset());
  index=next;
 }
 assert(beetle_help_heading(0u,-1)==0u);
 struct track_engine eng;
 static struct track_song_bank song;
 track_init(&eng);track_song_init(&song,&eng);
 assert(track_set_pattern_length(&eng,8)==0);
 assert(track_set_pattern_length(&eng,0)!=0);
 assert(track_set_pattern_length(&eng,17)!=0);
 song.patterns[3].length=12u;
 uint8_t pending=3;
 track_play(&eng);
 for(int i=0;i<7;i++){
  track_tick(&eng);
  assert(!beetle_pattern_queue_commit(&song,&eng,&pending));
  assert(eng.pattern_slot==0);
 }
 track_tick(&eng);
 assert(beetle_pattern_queue_commit(&song,&eng,&pending));
 assert(eng.pattern_slot==3 && eng.active.length==12u && pending==255u);
 assert(!beetle_pattern_queue_commit(&song,&eng,&pending));
 pending=1u;song.enabled=1u;
 for(int i=0;i<12;i++)track_tick(&eng);
 assert(!beetle_pattern_queue_commit(&song,&eng,&pending));
 assert(pending==1u && eng.pattern_slot==3);
 puts("PASS BEETLE V0.60: post-load SEL gating, HELP chapter jumps, 1-16 pattern length, queued switching");
 return 0;
}
