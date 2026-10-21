#include <assert.h>
#include <stdio.h>
#include "beetle_keyboard_octave.h"
#include "native_octave_mode.h"
int main(void){
  assert(beetle_keyboard_pitch(0,0)==53);
  assert(beetle_keyboard_pitch(0,1)==65);
  assert(beetle_keyboard_pitch(7,1)==72);
  assert(beetle_keyboard_pitch(26,3)==115);
  assert(beetle_keyboard_pitch(0,-4)==5);
  assert(beetle_keyboard_shift(-4,-1)==-4);
  assert(beetle_keyboard_shift(3,1)==3);
  assert(beetle_keyboard_shift(0,1)==1);
  struct etch_octave_chord s={0};
  assert(etch_octave_scan(&s,1)==ETCH_OCT_NONE);
  assert(etch_octave_scan(&s,3)==ETCH_OCT_TOGGLE);
  assert(etch_octave_scan(&s,1)==ETCH_OCT_NONE);
  assert(etch_octave_scan(&s,0)==ETCH_OCT_NONE);
  assert(etch_octave_scan(&s,2)==ETCH_OCT_NONE);
  assert(etch_octave_scan(&s,0)==ETCH_OCT_PLUS);
  puts("PASS BEETLE V0.51 octave range, MIDI mapping and simultaneous-key latch");
}
