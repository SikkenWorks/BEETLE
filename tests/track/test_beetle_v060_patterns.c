/* BEETLE V0.60: pattern length and HELP heading navigation smoke tests.
 * Host-only and deterministic; doesn't claim FM1 hardware validation. */
#include <assert.h>
#include <stdio.h>
#include "track_core.h"
#include "track_song.h"
#include "beetle_help.h"
#include "beetle_pattern_queue.h"
int main(void){
 struct track_engine engine;track_init(&engine);
 assert(track_set_pattern_length(&engine,0u)==-1);
 assert(track_set_pattern_length(&engine,17u)==-1);
 assert(track_set_pattern_length(&engine,3u)==0);
 assert(engine.active.length==3u);
 track_play(&engine);
 assert(track_set_pattern_length(&engine,2u)==-1);
 for(int i=0;i<12;i++){
  assert(track_tick(&engine)==1);
  assert(engine.transport.play_step==(i+1)%3);
 }
 track_stop(&engine);
 uint8_t b[TRACK_PATTERN_BYTES];
 assert(track_pattern_encode(&engine.active,b,sizeof(b))==TRACK_PATTERN_BYTES);
 struct track_pattern decoded;
 assert(track_pattern_decode(&decoded,b,sizeof(b))==0);
 assert(decoded.length==3u);
 b[3]=0;assert(track_pattern_decode(&decoded,b,sizeof(b))==-1);
 b[3]=17;assert(track_pattern_decode(&decoded,b,sizeof(b))==-1);
 struct track_song_bank bank; track_song_init(&bank,&engine);
 assert(track_song_switch(&bank,&engine,1u)==0);
 assert(engine.active.length==16u);
 assert(track_song_switch(&bank,&engine,0u)==0);
 assert(engine.active.length==3u);
 volatile uint8_t pending=2u;
 track_play(&engine);engine.transport.play_step=1u;
 assert(beetle_pattern_queue_commit(&bank,&engine,&pending)==0);
 assert(engine.pattern_slot==0u&&pending==2u);
 engine.transport.play_step=0u;
 assert(beetle_pattern_queue_commit(&bank,&engine,&pending)==1);
 assert(engine.pattern_slot==2u&&pending==255u);
 pending=4u;bank.enabled=1u;
 assert(beetle_pattern_queue_commit(&bank,&engine,&pending)==0);
 assert(engine.pattern_slot==2u&&pending==4u);
 track_stop(&engine);
 assert(beetle_help_heading(0,1)>0);
 assert(beetle_help_lines[beetle_help_heading(0,1)][0]=='#');
 assert(beetle_help_heading(beetle_help_heading(0,1),-1)==0u);
 uint8_t bottom=beetle_help_last_offset();
 assert(beetle_help_heading(bottom,1)==bottom);
 assert(beetle_help_heading(bottom,-1)<bottom);
 puts("PASS BEETLE V0.60 pattern length, per-pattern persistence, HELP K4 headings");
 return 0;
}
