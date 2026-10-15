/* Regression: keyboard CH6 NOTE2 preserves NOTE1, cursor, instrument and FX.
 * SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "track_core.h"
#include "track_drum.h"
int main(void){
 struct track_engine e;
 track_init(&e);
 e.transport.edit_channel=TRACK_DRUM_CHANNEL;
 e.transport.edit_column=1;
 e.transport.edit_step=5;
 struct track_step *s=&e.active.ch[TRACK_DRUM_CHANNEL][5];
 s->note=4u;s->instrument=2u;s->effect=3u;s->value=77u;
 assert(track_enter_drum_note2(&e,0u)==0);
 assert(s->note==4u && s->chord==1u && e.transport.edit_step==5u);
 assert(s->instrument==2u && s->effect==3u && s->value==77u && e.dirty);
 assert(track_enter_drum_note2(&e,13u)==0 && s->chord==14u && s->note==4u);
 assert(track_enter_drum_note2(&e,14u)==-1 && s->chord==14u);
 e.transport.edit_column=0u;
 assert(track_enter_drum_note2(&e,1u)==-1 && s->chord==14u);
 assert(track_enter_note(&e,1u)==0 && s->note==1u && e.transport.edit_step==6u);
 e.transport.edit_step=5u;e.transport.edit_column=3u;
 assert(track_enter_note(&e,2u)==0 && s->note==2u && s->chord==14u);
 puts("V0.88: CH6 NOTE2/NOTE1 independent input PASS");
 return 0;
}
