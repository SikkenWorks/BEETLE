/* SPDX-License-Identifier: GPL-3.0-only: hardware independent tracker regression */
#include <assert.h>
#include <stdio.h>
#include "track_core.h"
int main(void){
 struct track_engine t;track_init(&t);
 for(unsigned column=0;column<4;column++){
  t.transport.edit_channel=1;t.transport.edit_column=(uint8_t)column;t.transport.edit_step=0;
  struct track_step *s=&t.active.ch[1][0];s->chord=7;s->effect=4;s->value=62;s->instrument=3;
  assert(track_enter_note(&t,(uint8_t)(60+column))==0);
  assert(s->note==60+column && s->instrument==3 && s->chord==7 && s->effect==4 && s->value==62);
  assert(t.transport.edit_step==1);
 }
 t.transport.edit_channel=5;t.transport.edit_column=3;t.transport.edit_step=0;
 t.active.ch[5][0].chord=2;
 assert(track_enter_note(&t,4)==0 && t.active.ch[5][0].note==4 && t.active.ch[5][0].chord==2);
 t.transport.edit_channel=1;t.transport.edit_step=0;t.transport.edit_column=3;
 t.active.ch[1][0].note=64;
 assert(track_adjust_octave(&t,12)==1 && t.active.ch[1][0].note==76);
 assert(track_adjust_octave(&t,-12)==1 && t.active.ch[1][0].note==64);
 t.active.ch[1][0].note=254;assert(track_adjust_octave(&t,12)==0 && t.active.ch[1][0].note==254);
 t.active.ch[1][0].note=255;assert(track_adjust_octave(&t,-12)==0 && t.active.ch[1][0].note==255);
 t.active.ch[1][0].note=127;assert(track_adjust_octave(&t,12)==0 && t.active.ch[1][0].note==127);
 t.active.ch[1][0].note=0;assert(track_adjust_octave(&t,-12)==0 && t.active.ch[1][0].note==0);
 t.transport.edit_channel=5;t.active.ch[5][0].note=4;
 assert(track_adjust_octave(&t,12)==-1 && t.active.ch[5][0].note==4);
 puts("V0.85 tracker note entry and synth octave PASS");
 return 0;
}
