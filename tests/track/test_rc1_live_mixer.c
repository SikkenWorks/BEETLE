#include <stdio.h>
#include <assert.h>
#include "beetle_live_mixer.h"
#include "beetle_drum_step_stop.h"
#include "track_mixer.h"
int main(void){
 struct beetle_live_switch m={0},s={0};
 assert(!beetle_live_value(&m,0,0));
 assert(beetle_live_toggle(&m,0,0,1));
 assert(beetle_live_next(&m,0,0)==1 && beetle_live_value(&m,0,0)==0);
 assert(beetle_live_toggle(&m,0,0,1));
 assert(beetle_live_next(&m,0,0)==0 && !m.pending);
 assert(beetle_live_toggle(&m,0,0,1));
 beetle_live_commit(&m);
 assert(beetle_live_value(&m,0,0)==1 && !m.pending);
 assert(beetle_live_toggle(&s,2,0,1));
 assert(beetle_live_next(&s,2,0) && !beetle_live_value(&s,2,0));
 beetle_live_commit(&s);
 assert(beetle_live_value(&s,2,0));
 beetle_live_clear(&s);
 assert(!beetle_live_value(&s,2,0));
 /* Q OFF toggles apply immediately without influencing the other queue. */
 assert(beetle_live_toggle(&m,3,0,0));
 assert(beetle_live_value(&m,3,0)==1);
 struct track_step drum={5,0,7,0,50};
 beetle_stop_focused_note(&drum,TRACK_DRUM_CHANNEL,1);
 assert(drum.note==5 && drum.chord==15);
 beetle_stop_focused_note(&drum,TRACK_DRUM_CHANNEL,0);
 assert(drum.note==TRACK_NOTE_STOP && drum.chord==15); /* NOTE2 STP stays */
 struct track_step twin={5,0,7,0,50};
 beetle_stop_focused_note(&twin,TRACK_DRUM_CHANNEL,0);
 assert(twin.note==TRACK_NOTE_STOP && twin.chord==7); /* preserve populated NOTE2 */
 beetle_stop_focused_note(&twin,TRACK_DRUM_CHANNEL,1);
 assert(twin.note==TRACK_NOTE_STOP && twin.chord==15); /* NOTE1 STP stays */
 struct track_step empty_two={2,0,0,0,50};
 beetle_stop_focused_note(&empty_two,TRACK_DRUM_CHANNEL,0);
 assert(empty_two.note==TRACK_NOTE_STOP && empty_two.chord==0);
 struct track_step synth={60,0,5,0,50};
 beetle_stop_focused_note(&synth,0,0);
 assert(synth.note==TRACK_NOTE_STOP && synth.chord==0);
 puts("PASS 1.01 RC1 Q mute/solo and independent drum NOTE1/NOTE2 ENV STOP");
 return 0;
}
