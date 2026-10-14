#include <assert.h>
#include <stdio.h>
#include "track_chip.h"
int main(void){
 struct track_chip_engine s;track_chip_init(&s);
 for(unsigned i=0;i<8;i++)assert(track_chip_note(&s,0,0,60+i,100));
 assert(!track_chip_note(&s,0,0,80,100));
 unsigned first=s.last_voice_index;
 assert(first==7);
 s.voices[3].active=0;
 assert(track_chip_note(&s,0,0,80,100));
 assert(s.last_voice_index==3);
 assert(s.voices[3].active);
 assert(!track_chip_note(&s,0,0,85,100));
 for(unsigned n=0;n<100;n++){
  s.voices[3].active=0;
  assert(track_chip_note(&s,0,0,40+(n%40),100));
  assert(s.last_voice_index==3);
 }
 puts("TRACK V0.13 EDIT poly audition voice recycling PASS");return 0;
}
