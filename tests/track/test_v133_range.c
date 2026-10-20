/* BEETLE V1.3.3: inclusive block editing, no ghost notes, no cross-lane edits. */
#include <assert.h>
#include <stdio.h>
#include "track_core.h"
static struct track_engine tr;
int main(void){
 track_init(&tr);tr.active.length=12;
 for(unsigned i=0;i<TRACK_STEPS;i++){
  tr.active.ch[0][i].note=TRACK_NOTE_EMPTY;
  tr.active.ch[0][i].instrument=0;
  tr.active.ch[1][i].note=42;
  tr.active.ch[1][i].instrument=7;
 }
 tr.active.ch[0][2].note=60;
 tr.active.ch[0][3].note=TRACK_NOTE_STOP;
 tr.active.ch[0][4].note=62;
 tr.active.ch[0][5].note=66;
 tr.active.ch[0][6].note=TRACK_NOTE_EMPTY;
 assert(track_set_instrument_range(&tr,0,6,2,6)==3);
 assert(tr.active.ch[0][2].instrument==6);
 assert(tr.active.ch[0][3].instrument==0 && tr.active.ch[0][3].note==TRACK_NOTE_STOP);
 assert(tr.active.ch[0][4].instrument==6 && tr.active.ch[0][5].instrument==6);
 assert(tr.active.ch[0][6].instrument==0 && tr.active.ch[0][6].note==TRACK_NOTE_EMPTY);
 assert(tr.active.ch[1][4].instrument==7);
 assert(track_set_instrument_range(&tr,5u,2,6,3)==0);
 assert(track_set_instrument_range(&tr,0,2,6,8)==0);
 assert(track_clear_step_range(&tr,0,4,2)==3);
 for(unsigned i=2;i<=4;i++)assert(tr.active.ch[0][i].note==TRACK_NOTE_EMPTY);
 assert(tr.active.ch[0][5].note==66 && tr.active.ch[1][2].note==42);
 tr.active.note2_fx[4].effect=3;
 tr.active.ch[5][4].note=0;
 assert(track_clear_step_range(&tr,5,4,4)==1);
 assert(tr.active.note2_fx[4].effect==0 && tr.active.ch[5][4].note==TRACK_NOTE_EMPTY);
 tr.active.ch[0][11].note=90;
 tr.active.ch[0][12].note=91;
 assert(track_clear_step_range(&tr,0,11,15)==1);
 assert(tr.active.ch[0][11].note==TRACK_NOTE_EMPTY && tr.active.ch[0][12].note==91);
 puts("PASS V1.3.3: range instrument assign, ghost prevention, delete, drums, boundaries, lane isolation");return 0;
}
