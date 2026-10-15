/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "beetle_pattern_edit.h"
static struct track_engine engine;
static struct track_song_bank song;
static struct beetle_pattern_editor history;
static struct beetle_pattern_image pending,whole;
static struct track_step block[TRACK_STEPS];
static void setnote(uint8_t at,uint8_t pitch){
 beetle_edit_begin(&history,&song,&engine);
 engine.active.ch[0][at].note=pitch;
 assert(beetle_edit_commit(&history,&song,&engine));
}
int main(void){
 track_init(&engine);track_song_init(&song,&engine);history.pending=&pending;history.whole_clip=&whole;history.block=block;beetle_edit_reset(&history);
 assert(!beetle_edit_undo(&history,&song,&engine));
 setnote(0,60);setnote(1,62);
 assert(history.count==2 && history.cursor==2);
 assert(beetle_edit_undo(&history,&song,&engine));assert(engine.active.ch[0][1].note==TRACK_NOTE_EMPTY);
 assert(beetle_edit_redo(&history,&song,&engine));assert(engine.active.ch[0][1].note==62);
 assert(beetle_edit_undo(&history,&song,&engine));setnote(1,64);
 assert(!beetle_edit_redo(&history,&song,&engine));
 /* Six-lane pattern plus Punch-In virtual lane copy/paste. */
 for(int c=0;c<TRACK_CHANNELS;c++)engine.active.ch[c][4].note=(uint8_t)(50+c);
 song.clips[0].count=1;song.clips[0].data[0].step=2;song.clips[0].data[0].fx=1;
 beetle_edit_copy_pattern(&history,&song,&engine);
 assert(track_song_switch(&song,&engine,1)==0);
 assert(beetle_edit_paste_pattern(&history,&song,&engine));
 for(int c=0;c<TRACK_CHANNELS;c++)assert(engine.active.ch[c][4].note==50+c);
 assert(song.clips[1].count==1 && song.clips[1].data[0].fx==1);
 assert(beetle_edit_undo(&history,&song,&engine));
 assert(engine.active.ch[0][4].note==TRACK_NOTE_EMPTY && !song.clips[1].count);
 assert(beetle_edit_redo(&history,&song,&engine));assert(engine.active.ch[5][4].note==55);
 /* Different-pattern snapshots must restore the right pattern. */
 assert(beetle_edit_undo(&history,&song,&engine));
 assert(beetle_edit_undo(&history,&song,&engine));
 assert(engine.pattern_slot==0 && engine.active.ch[0][1].note==TRACK_NOTE_EMPTY);
 assert(beetle_edit_redo(&history,&song,&engine));
 /* Block operations stay within selected lane and preserve outside data. */
 beetle_edit_reset(&history);
 engine.active.ch[0][2].note=60;engine.active.ch[0][3].note=62;
 history.first=2;history.last=3;history.action=0;
 assert(beetle_edit_block_execute(&history,&song,&engine));
 history.first=7;history.last=7;history.action=1;
 assert(beetle_edit_block_execute(&history,&song,&engine));
 assert(engine.active.ch[0][7].note==60 && engine.active.ch[0][8].note==62);
 history.first=7;history.last=8;history.action=3;history.amount=12;
 assert(beetle_edit_block_execute(&history,&song,&engine));
 assert(engine.active.ch[0][7].note==72 && engine.active.ch[0][8].note==74);
 assert(beetle_edit_undo(&history,&song,&engine));assert(engine.active.ch[0][7].note==60);
 assert(beetle_edit_redo(&history,&song,&engine));assert(engine.active.ch[0][7].note==72);
 history.action=2;assert(beetle_edit_block_execute(&history,&song,&engine));
 assert(engine.active.ch[0][7].note==TRACK_NOTE_EMPTY);
 assert(engine.active.ch[0][2].note==60);
 /* Bounded history entries, bounded cap, never writes when nothing changes. */
 beetle_edit_reset(&history);
 for(int i=0;i<10;i++)setnote(10,(uint8_t)(60+i));
 assert(history.count==BEETLE_UNDO_CAP && history.cursor==BEETLE_UNDO_CAP);
 beetle_edit_begin(&history,&song,&engine);
 assert(!beetle_edit_commit(&history,&song,&engine));
 assert(history.count==BEETLE_UNDO_CAP);
 puts("PASS V0.56 edit history, redo branching, cross-pattern undo, FX clipboard, blocks, bounds");
 return 0;
}
