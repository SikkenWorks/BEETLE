/* BEETLE V1.3.5: F3..G5 all 27 debounced note positions are chromatic.
 * Verified portable note-entry engine; physical key-matrix wiring still needs hardware test.
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <assert.h>
#include <stdio.h>
#include "track_core.h"
#include "beetle_keyboard_octave.h"
static struct track_engine e;
int main(void){
 for(int key=0;key<27;key++){
  track_init(&e);e.transport.edit_channel=0u;e.transport.edit_step=0u;
  uint8_t pitch=beetle_keyboard_pitch((uint8_t)key,0);
  assert(pitch==53+key);assert(track_enter_note(&e,pitch)==0);
  assert(e.active.ch[0][0].note==pitch && e.transport.edit_step==1u);
  assert(e.active.ch[1][0].note==TRACK_NOTE_EMPTY);
  assert(beetle_keyboard_pitch((uint8_t)key,1)==pitch+12);
 }
 assert(beetle_keyboard_pitch(1,0)==54); /* Black F#3 */
 assert(beetle_keyboard_pitch(25,0)==78); /* Black F#5, except SEQ+clear-lane */
 puts("PASS V1.3.5: 27 semitone note entries, black-key sharps, independent lanes and octaves");
 return 0;
}
