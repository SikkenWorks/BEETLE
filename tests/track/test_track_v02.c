#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "track_core.h"
#include "track_name.h"
#include "track_navigation.h"

static void names(void) {
 struct track_name_editor ed;char out[TRACK_NAME_MAX+1];
 for(unsigned slot=0;slot<4;slot++){
  track_name_begin(&ed,0);
  track_name_finish(&ed,slot,out);
  char want[]="Project 1"; want[8]=(char)('1'+slot);
  assert(strcmp(out,want)==0);
 }
 track_name_begin(&ed,0);
 assert(ed.cursor==0 && ed.glyph_index==0);
 track_name_change(&ed,26); /* Z = index 26 */
 assert(ed.name[0]=='Z'&&ed.remembered_index==26);
 track_name_move(&ed,1);
 assert(ed.cursor==1&&ed.glyph_index==26&&!ed.touched[1]);
 track_name_change(&ed,1); /* next glyph after Z is 0 */
 assert(ed.name[1]=='0'&&ed.glyph_index==27);
 track_name_move(&ed,1);
 assert(ed.glyph_index==27);
 track_name_change(&ed,1);
 assert(ed.name[2]=='1');
 track_name_move(&ed,-2);
 assert(ed.cursor==0&&ed.name[0]=='Z'&&ed.glyph_index==26);
 track_name_finish(&ed,1,out);
 assert(strcmp(out,"Z01")==0);
 track_name_begin(&ed,0);
 track_name_change(&ed,-1);
 assert(ed.glyph_index==track_name_charset_size()-1);
 track_name_change(&ed,1);
 assert(ed.glyph_index==0);
 track_name_move(&ed,-40);assert(ed.cursor==0);
 track_name_move(&ed,120);assert(ed.cursor==TRACK_NAME_MAX-1);
}
static void ui_and_transport(void) {
 struct track_engine e;track_init(&e);
 assert(e.active.length==16);
 for(unsigned i=0;i<16;i++)assert(track_step_display_number((uint8_t)i)==i);
 assert(track_column_move(0,-1)==0);
 assert(track_column_move(0,1)==1); /* 0 NOTE, not STEP */
 assert(track_column_move(3,20)==3);
 assert(track_row_move(0,-99)==0);
 assert(track_row_move(15,99)==15);
 e.transport.edit_step=7;e.transport.edit_column=2;
 track_rotate_select(&e,1);assert(e.active.ch[0][7].chord==1);
 e.transport.edit_column=0;
 track_rotate_select(&e,1);assert(e.transport.edit_channel==1);
 e.transport.edit_channel=0;e.transport.edit_step=12;
 track_play(&e);assert(e.transport.play_step==0&&e.transport.stop_step==12);
 for(unsigned i=0;i<15;i++)track_tick(&e);
 assert(e.transport.play_step==15);
 track_tick(&e);assert(e.transport.play_step==0);
 track_stop(&e);assert(e.transport.edit_step==12);
 assert(track_home_action(1499,0)==0);
 assert(track_home_action(1499,1)==1);
 assert(track_home_action(1500,1)==2);
 assert(track_is_lit(2)==1&&track_is_lit(10)==0&&track_is_lit(13)==1);
}
static void codec(void){
 struct track_engine e;track_init(&e);
 assert(track_set_note(&e,1,0,60)==0);
 assert(track_set_note(&e,1,16,60)==-1);
 unsigned char out[TRACK_PATTERN_BYTES];
 assert(track_pattern_encode(&e.active,out,sizeof(out))==TRACK_PATTERN_BYTES);
 struct track_pattern decoded;
 assert(track_pattern_decode(&decoded,out,sizeof(out))==0);
 assert(decoded.ch[1][0].note==60 && decoded.ch[1][15].note==255);
 out[2]=0;assert(track_pattern_decode(&decoded,out,sizeof(out))==-1);
}
int main(void){names();ui_and_transport();codec();puts("TRACK V0.2: native model, cursor and naming tests PASS");return 0;}
