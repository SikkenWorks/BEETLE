#ifndef BEETLE_DRUM_STEP_STOP_H
#define BEETLE_DRUM_STEP_STOP_H
/* 1.01 RC1: ENV modifies only the focused CH6 drum note.
 * NOTE2 uses chord==15, NOTE1 uses note==TRACK_NOTE_STOP.
 * Both are independent even when the other drum hit is populated. */
#include "track_core.h"
#include "track_drum.h"
static inline void beetle_stop_focused_note(struct track_step *s,uint8_t channel,uint8_t edit_column){
 if(!s)return;
 if(channel==TRACK_DRUM_CHANNEL && edit_column==1u)s->chord=15u;
 else{
  s->note=TRACK_NOTE_STOP;
  /* CH6 NOTE1 and NOTE2 are independent: never erase the NOTE2 chord.
   * Synth lanes keep their existing STOP semantics. */
  if(channel!=TRACK_DRUM_CHANNEL)s->chord=0u;
 }
}
#endif
