/* RC1.0.8: exact lane clear and SEQ+rightmost BLACK gesture regression. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "track_core.h"
#include "track_ui_controls.h"
int main(void){
 struct track_engine e;
 for(unsigned lane=0;lane<TRACK_CHANNELS;lane++){
  track_init(&e);
  e.active.length=4u;
  e.transport.edit_channel=lane;
  e.transport.edit_step=2u;
  e.pattern_slot=7u;
  for(unsigned ch=0;ch<TRACK_CHANNELS;ch++)for(unsigned row=0;row<TRACK_STEPS;row++){
   struct track_step *s=&e.active.ch[ch][row];
   s->note=(uint8_t)(60u+ch);
   s->chord=(ch==(TRACK_CHANNELS-1u))?4u:2u;
   s->instrument=3u;
   s->effect=5u;
   s->value=99u;
   s->dynamics=0x94u;
   s->groove=17u;
  }
  for(unsigned row=0;row<TRACK_STEPS;row++){
   e.active.note2_fx[row].effect=8u;
   e.active.note2_fx[row].value=75u;
  }
  struct track_pattern before=e.active;
  assert(track_clear_lane(&e,(uint8_t)lane)==0);
  assert(e.dirty==1u && e.active.length==4u && e.transport.edit_step==2u && e.pattern_slot==7u);
  for(unsigned ch=0;ch<TRACK_CHANNELS;ch++)for(unsigned row=0;row<TRACK_STEPS;row++){
   const struct track_step *s=&e.active.ch[ch][row];
   if(ch!=lane){assert(memcmp(s,&before.ch[ch][row],sizeof(*s))==0);continue;}
   assert(s->note==TRACK_NOTE_EMPTY && s->chord==0u && s->effect==0u && s->value==50u && s->dynamics==0u && s->groove==0u);
  }
  for(unsigned row=0;row<TRACK_STEPS;row++){
   if(lane==(TRACK_CHANNELS-1u)){assert(e.active.note2_fx[row].effect==0u && e.active.note2_fx[row].value==0u);}
   else assert(memcmp(&e.active.note2_fx[row],&before.note2_fx[row],sizeof(struct track_note2_fx))==0);
  }
 }
 assert(track_clear_lane(NULL,0u)==-1);
 track_init(&e);assert(track_clear_lane(&e,6u)==-1);
 assert(BEETLE_CLEAR_LANE_KEY==25u);
 const uint32_t edge=1ul<<BEETLE_CLEAR_LANE_KEY;
 assert(beetle_clear_lane_chord(1u,1u,0u,0u,edge));
 assert(!beetle_clear_lane_chord(0u,1u,0u,0u,edge));
 assert(!beetle_clear_lane_chord(1u,0u,0u,0u,edge));
 assert(!beetle_clear_lane_chord(1u,1u,1u,0u,edge));
 assert(!beetle_clear_lane_chord(1u,1u,0u,1u,edge));
 assert(!beetle_clear_lane_chord(1u,1u,0u,0u,(1ul<<13)));
 puts("RC1.0.8 clear-lane / all 6 channels / NOTE2 FX / modifier gating OK");
 return 0;
}
