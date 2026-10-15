/* BEETLE V0.83: all synthesis types must respond to live octave punch,
 * including notes that are already sustaining and NES noise clock. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "track_chip.h"
int main(void){
 for(uint8_t type=0;type<TRACK_CHIP_TYPES;type++){
  struct track_chip_engine s;track_chip_init(&s);
  s.osc_type[0]=type;s.env_sustain[0]=100u;
  assert(track_chip_note(&s,0u,0u,60u,110u));
  struct track_chip_voice *v=&s.voices[s.last_voice_index];
  uint8_t orig_note=v->note,orig_type=v->type;
  uint32_t original=(type==TRACK_CHIP_NES_NOISE)?v->step2:v->step;
  track_chip_set_punch_octave(&s,(1u<<5u));
  assert(s.punch_octave==12 && v->note==orig_note && v->type==orig_type);
  uint32_t up=(type==TRACK_CHIP_NES_NOISE)?v->step2:v->step;
  assert(up!=original);
  track_chip_set_punch_octave(&s,(1u<<6u));
  uint32_t down=(type==TRACK_CHIP_NES_NOISE)?v->step2:v->step;
  assert(s.punch_octave==-12 && down!=up && down!=original);
  track_chip_set_punch_octave(&s,(1u<<5u)|(1u<<6u));
  assert(s.punch_octave==0);
  assert(((type==TRACK_CHIP_NES_NOISE)?v->step2:v->step)==original);
  track_chip_set_punch_octave(&s,0u);
  assert(s.punch_octave==0 && v->note==orig_note && v->type==orig_type);
 }
 puts("PASS V0.83: all 12 synth types pitch up/down live, held notes restored");
 return 0;
}
