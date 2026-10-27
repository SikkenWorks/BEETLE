/* V1.4.33 CHD encoder audition policy: same shapes, no persisted side effects. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "beetle_chord_scroll_preview.h"
int main(void){
 struct track_step st={0};
 struct track_harmony h={0};
 uint8_t notes[5]={0};
 const uint8_t expected[12]={1,3,3,3,3,3,3,4,4,4,4,5};
 st.note=60u;st.instrument=2u;st.effect=7u;st.value=81u;
 for(uint8_t i=0u;i<TRACK_CHORD_COUNT;i++){
  st.chord=i;
  struct track_step before=st;
  uint8_t count=beetle_chord_scroll_pitches(&st,&h,2u,0u,notes);
  assert(count==expected[i]);assert(notes[0]==60u);
  assert(memcmp(&st,&before,sizeof(st))==0); /* no writes to tracker */
  if(i==1u){assert(notes[1]==64u && notes[2]==67u);}
  if(i==2u){assert(notes[1]==63u && notes[2]==67u);}
  assert(beetle_chord_scroll_pitches(&st,&h,5u,0u,notes)==0u); /* drum */
  assert(beetle_chord_scroll_pitches(&st,&h,2u,1u,notes)==0u); /* PLAY */
 }
 st.note=TRACK_NOTE_EMPTY;assert(beetle_chord_scroll_pitches(&st,&h,1u,0u,notes)==0u);
 st.note=TRACK_NOTE_STOP; assert(beetle_chord_scroll_pitches(&st,&h,1u,0u,notes)==0u);
 st.note=60u;st.chord=15u;assert(beetle_chord_scroll_pitches(&st,&h,1u,0u,notes)==0u);
 st.chord=11u;h.root=0u;h.mode=0u;h.keylock=2u;
 assert(beetle_chord_scroll_pitches(&st,&h,1u,0u,notes)==5u);
 for(uint8_t i=1u;i<5u;i++)assert(notes[i]>=notes[i-1]);
 puts("PASS V1.4.33 chord audition (all 12 shapes, keylock, ignore empty/STP/drums/PLAY, no writes)");
}
