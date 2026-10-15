/* BEETLE V0.36 mixer encoder, 6 black keyboard mute keys, SEL step clear.
 * SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "track_core.h"
#include "track_drum.h"
#include "track_mixer.h"
#include "track_ui_controls.h"
int main(void){
 struct track_mixer m;
 track_mixer_init(&m);
 assert(TRACK_MIXER_CHANNELS==6u);
 for(uint8_t lane=0;lane<6u;lane++){
  m.row=lane;
  for(uint8_t field=2;field<=4;field++){
   m.col=field;
   assert(track_mixer_change(&m,+2)==1);
   if(field==4){ /* V0.38: 0..100 continuous swing, not old ON/OFF. */
    assert(m.ch[lane].swing==4u);
    assert(track_mixer_change(&m,+8)==1);
    assert(m.ch[lane].swing==20u);
    assert(track_mixer_change(&m,-3)==1);
    assert(m.ch[lane].swing==14u);
    assert(track_mixer_change(&m,-12)==1);
    assert(m.ch[lane].swing==0u);
   }else{
    assert(track_mixer_change(&m,+8)==0); /* mute and solo still directional */
    assert((field==2?m.ch[lane].mute:m.ch[lane].solo)==1u);
    assert(track_mixer_change(&m,-3)==1);
    assert(track_mixer_change(&m,-12)==0);
    assert((field==2?m.ch[lane].mute:m.ch[lane].solo)==0u);
   }
  }
 }
 const uint8_t keys[6]={1,3,5,8,10,13};
 for(uint8_t i=0;i<6;i++)assert(beetle_black_mute_lane(keys[i])==(int8_t)i);
 for(uint8_t k=0;k<27;k++){
  uint8_t found=0;
  for(uint8_t i=0;i<6;i++)if(keys[i]==k)found=1;
  if(!found)assert(beetle_black_mute_lane(k)==-1);
 }
 assert(beetle_hold_finished(5999u,1000u)==0u);
 assert(beetle_hold_finished(6000u,1000u)==1u);
 assert(beetle_hold_finished(6001u,1000u)==1u);
 assert(beetle_hold_finished(9u,0u)==0u);
 assert(beetle_hold_finished(4001u,UINT32_MAX-999u)==1u); /* timer wrap */
 assert(beetle_list_move(0u,-3)==0u);
 assert(beetle_list_move(11u,+100)==11u);
 assert(beetle_list_move(5u,-2)==3u);
 struct track_engine e;track_init(&e);
 for(uint8_t lane=0;lane<TRACK_CHANNELS;lane++){
  e.transport.edit_channel=lane;e.transport.edit_step=7u;
  struct track_step *s=&e.active.ch[lane][7];
  s->note=lane==TRACK_DRUM_CHANNEL?4u:61u;
  s->instrument=2u;s->chord=3u;s->effect=TRACK_FX_VOLUME;s->value=9u;
  assert(track_clear_step(&e,lane,7u)==0);
  assert(s->note==TRACK_NOTE_EMPTY&&s->effect==0u&&s->chord==0u&&s->value==50u);
  assert(s->instrument==(lane==TRACK_DRUM_CHANNEL?3u:(lane&3u)));
  assert(e.transport.edit_step==7u);
  assert(e.dirty==1u);
 }
 assert(track_clear_step(&e,TRACK_CHANNELS,0u)==-1);
 assert(track_clear_step(&e,0u,TRACK_STEPS)==-1);
 puts("BEETLE V0.36 UI/mixer/clear/5-sec semantics PASS");
 return 0;
}
