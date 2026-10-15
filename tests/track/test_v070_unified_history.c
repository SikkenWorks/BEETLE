/* SPDX-License-Identifier: GPL-3.0-only: BEETLE 0.70 mixed-domain Undo/Redo. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "beetle_pattern_edit.h"
static struct track_engine e;
static struct track_song_bank song;
static struct beetle_pattern_editor editor;
static struct beetle_pattern_image pending,whole;
static struct track_step block[TRACK_STEPS];
static unsigned count;
static void change_int(uint8_t kind,uint8_t slot,int *n,int v){
 assert(beetle_edit_begin_bytes(&editor,kind,slot,n,sizeof(*n)));
 *n=v;
 assert(beetle_edit_commit_bytes(&editor,kind,slot,n,sizeof(*n)));
 count++;
}
static void move_backward(uint8_t kind,uint8_t slot,int *n,int old){
 assert(editor.cursor);uint8_t pos=(uint8_t)(editor.cursor-1u);
 assert(editor.history[pos].kind==kind && editor.history[pos].pattern_index==slot);
 assert(beetle_edit_swap_bytes(&editor,pos,n,sizeof(*n)));
 editor.cursor--;assert(*n==old);
}
static void move_forward(uint8_t kind,uint8_t slot,int *n,int now){
 assert(editor.cursor<editor.count);uint8_t pos=editor.cursor;
 assert(editor.history[pos].kind==kind && editor.history[pos].pattern_index==slot);
 assert(beetle_edit_swap_bytes(&editor,pos,n,sizeof(*n)));
 editor.cursor++;assert(*n==now);
}
int main(void){
 track_init(&e);track_song_init(&song,&e);
 editor.pending=&pending;editor.whole_clip=&whole;editor.block=block;beetle_edit_reset(&editor);
 beetle_edit_begin(&editor,&song,&e);e.active.ch[1][0].note=67;
 assert(beetle_edit_commit(&editor,&song,&e));
 int mx=10,inst=20,seq=30,master=40,lib=50;
 change_int(BEETLE_EDIT_MIXER,0,&mx,11);
 change_int(BEETLE_EDIT_INSTRUMENT,2,&inst,21);
 change_int(BEETLE_EDIT_SONG,0,&seq,31);
 change_int(BEETLE_EDIT_MASTER,0,&master,41);
 change_int(BEETLE_EDIT_LIBRARY,4,&lib,51);
 assert(editor.count==6u&&editor.cursor==6u);
 move_backward(BEETLE_EDIT_LIBRARY,4,&lib,50);
 move_backward(BEETLE_EDIT_MASTER,0,&master,40);
 move_backward(BEETLE_EDIT_SONG,0,&seq,30);
 move_backward(BEETLE_EDIT_INSTRUMENT,2,&inst,20);
 move_backward(BEETLE_EDIT_MIXER,0,&mx,10);
 assert(beetle_edit_undo(&editor,&song,&e));assert(e.active.ch[1][0].note==TRACK_NOTE_EMPTY);
 assert(beetle_edit_redo(&editor,&song,&e));assert(e.active.ch[1][0].note==67);
 move_forward(BEETLE_EDIT_MIXER,0,&mx,11);
 move_forward(BEETLE_EDIT_INSTRUMENT,2,&inst,21);
 move_forward(BEETLE_EDIT_SONG,0,&seq,31);
 move_forward(BEETLE_EDIT_MASTER,0,&master,41);
 move_forward(BEETLE_EDIT_LIBRARY,4,&lib,51);
 move_backward(BEETLE_EDIT_LIBRARY,4,&lib,50);
 change_int(BEETLE_EDIT_LIBRARY,6,&lib,77);
 assert(editor.cursor==editor.count && !beetle_edit_redo(&editor,&song,&e));
 beetle_edit_reset(&editor);assert(editor.count==0 && editor.cursor==0);
 /* Compact capacity, never allocates a second six-entry history in RAM. */
 assert(sizeof(editor.history)/sizeof(editor.history[0])==6);
 assert(sizeof(editor.history[0].state)>=sizeof(int));
 puts("PASS V0.70 six-entry mixed-domain history, chronological undo/redo, redo invalidation, reset");
 return 0;
}
