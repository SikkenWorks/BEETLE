/* BEETLE V0.62: pattern-specific cursor and hidden-row preservation. */
#include <assert.h>
#include <stdio.h>
#include "track_navigation.h"
#include "beetle_pattern_edit.h"
static struct track_engine e;
static struct track_song_bank bank;
static struct beetle_pattern_editor hist;
static struct beetle_pattern_image pending,whole;
static struct track_step block[TRACK_STEPS];
int main(void){
 track_init(&e);track_song_init(&bank,&e);
 hist.pending=&pending;hist.whole_clip=&whole;hist.block=block;beetle_edit_reset(&hist);
 for(uint8_t len=1;len<=16;len++){
   assert(track_set_pattern_length(&e,len)==0);
   for(int i=-40;i<=40;i++){
     uint8_t moved=track_row_move_length(15,i,len);
     assert(moved<len);
     assert(track_row_move_length(0,i,len)<len);
   }
   assert(track_row_move_length((uint8_t)(len-1),1,len)==len-1);
   assert(track_row_move_length(0,-1,len)==0);
   assert(track_row_move_length(20,0,len)==len-1);
   assert(beetle_block_bound_length(200,len)==len-1);
 }
 /* Shortening never destroys the backing memory of steps 9..16. */
 e.active.ch[0][15].note=73;
 e.transport.edit_step=15;
 assert(track_set_pattern_length(&e,8)==0);
 assert(e.transport.edit_step==7 && e.active.ch[0][15].note==73);
 assert(track_set_pattern_length(&e,16)==0);
 assert(e.active.ch[0][15].note==73);
 /* Switch and undo/redo into a shorter pattern clamps stale focus. */
 assert(track_set_pattern_length(&e,5)==0);
 track_song_capture(&bank,&e);
 assert(track_song_switch(&bank,&e,1)==0);
 e.transport.edit_step=14;
 assert(track_song_switch(&bank,&e,0)==0);
 assert(e.transport.edit_step==4);
 e.transport.edit_step=4;
 beetle_edit_begin(&hist,&bank,&e);
 assert(track_set_pattern_length(&e,2)==0);
 assert(beetle_edit_commit(&hist,&bank,&e));
 e.transport.edit_step=15;
 assert(beetle_edit_undo(&hist,&bank,&e));
 assert(e.active.length==5 && e.transport.edit_step==4);
 e.transport.edit_step=15;
 assert(beetle_edit_redo(&hist,&bank,&e));
 assert(e.active.length==2 && e.transport.edit_step==1);
 /* Block paste is truncated at length and cannot modify hidden rows. */
 assert(track_set_pattern_length(&e,16)==0);
 e.active.ch[0][0].note=60;e.active.ch[0][1].note=61;e.active.ch[0][3].note=62;
 hist.first=0;hist.last=3;hist.action=0;
 assert(beetle_edit_block_execute(&hist,&bank,&e));
 e.active.ch[0][4].note=78;e.active.ch[0][5].note=79;
 assert(track_set_pattern_length(&e,5)==0);
 hist.first=4;hist.last=15;hist.action=1;
 assert(beetle_edit_block_execute(&hist,&bank,&e));
 assert(e.active.ch[0][4].note==60 && e.active.ch[0][5].note==79);
 /* Core restores a valid stopped cursor even after changing pattern during playback. */
 e.transport.stop_step=12;e.transport.playing=1;
 track_stop(&e);assert(!e.transport.playing && e.transport.edit_step==4);
 puts("PASS V0.62 active-step bounds, pattern switch, undo/redo, block safety, stop");
}
