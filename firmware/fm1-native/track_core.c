/* TRACK V0.1 portable music state, based on ETCH's harmony interval ordering.
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "track_core.h"
#include "track_drum.h"
#include <string.h>
static const uint8_t scale[9][7]={
 {0,2,4,5,7,9,11},{0,2,3,5,7,9,10},{0,1,3,5,7,8,10},
 {0,2,4,6,7,9,11},{0,2,4,5,7,9,10},{0,2,3,5,7,8,10},
 {0,1,3,5,6,8,10},{0,2,3,5,7,8,11},{0,2,3,5,7,9,11}};
static const uint8_t chord[TRACK_CHORD_COUNT][6]={
 {1,0},{3,0,4,7},{3,0,3,7},{3,0,3,6},{3,0,4,8},{3,0,2,7},
 {3,0,5,7},{4,0,4,7,11},{4,0,3,7,10},{4,0,4,7,10},
 {4,0,4,7,14},{5,0,3,7,10,14}};
static uint8_t bounded(uint8_t n,uint8_t cap){return n<cap?n:(uint8_t)(cap-1u);}
void track_init(struct track_engine *e){if(!e)return;memset(e,0,sizeof(*e));e->transport.bpm=120u;e->active.length=16u;for(unsigned c=0;c<TRACK_CHANNELS;c++)for(unsigned st=0;st<TRACK_STEPS;st++){e->active.ch[c][st].note=255u;e->active.ch[c][st].instrument=(uint8_t)(c==TRACK_DRUM_CHANNEL?3u:c);e->active.ch[c][st].value=50u;}}
/* Scroll order: note 0..last, EMPTY, STP, then loop. Cope with stale data. */
uint8_t track_note_scroll(uint8_t note,int32_t delta,uint8_t drum){
 const int32_t count=drum?TRACK_DRUM_NOTE_COUNT:128;
 const int32_t total=count+2;
 int32_t at=note==TRACK_NOTE_EMPTY?count:note==TRACK_NOTE_STOP?count+1:(note<count?note:count);
 at=((at+(delta%total))%total+total)%total;
 return (uint8_t)(at==count?TRACK_NOTE_EMPTY:at==count+1?TRACK_NOTE_STOP:at);
}
uint8_t track_lane_has_data(const struct track_pattern *pattern,uint8_t channel){
 if(!pattern||channel>=TRACK_CHANNELS)return 0u;
 for(uint8_t row=0u;row<TRACK_STEPS;row++){
  const struct track_step *step=&pattern->ch[channel][row];
  /* STP is meaningful data. FX and drum NOTE2/chord count even without NOTE. */
  if(step->note!=TRACK_NOTE_EMPTY||step->chord!=0u||step->effect!=0u)return 1u;
 }
 return 0u;
}
int track_set_note(struct track_engine *e,uint8_t channel,uint8_t st,uint8_t note){if(!e||channel>=TRACK_CHANNELS||st>=TRACK_STEPS||(note>127u&&note!=TRACK_NOTE_STOP&&note!=TRACK_NOTE_EMPTY))return -1;e->active.ch[channel][st].note=note;e->dirty=1u;return 0;}
/* Clear the entire step, not merely the pitch. Keep the cursor where it is.
 * Default instrument matches track_init() for each synth/drum lane. */
int track_clear_step(struct track_engine *e,uint8_t channel,uint8_t step){
 if(!e||channel>=TRACK_CHANNELS||step>=TRACK_STEPS)return -1;
 struct track_step *s=&e->active.ch[channel][step];
 s->note=TRACK_NOTE_EMPTY;s->instrument=(uint8_t)(channel==TRACK_DRUM_CHANNEL?3u:channel);
 s->chord=0u;s->effect=0u;s->value=50u;s->dynamics=0u;s->groove=0u;
  e->dirty=1u;return 0;
}
/* V1.3.3: commit block changes as one history action in the caller.
 * A range is inclusive and bounded to the visible pattern length.
 * Note-only instrument edits never create ghost notes on EMPTY/STOP rows. */
int track_set_instrument_range(struct track_engine *e,uint8_t channel,uint8_t first,uint8_t last,uint8_t instrument){
 if(!e||channel>=TRACK_SYNTH_CHANNELS||instrument>=TRACK_INSTRUMENT_SLOTS)return 0;
 uint8_t lo=first<last?first:last,hi=first>last?first:last;
 if(lo>=e->active.length)return 0;if(hi>=e->active.length)hi=(uint8_t)(e->active.length-1u);
 int changes=0;
 for(uint8_t i=lo;i<=hi;i++){
  struct track_step *step=&e->active.ch[channel][i];
  if(step->note>=TRACK_NOTE_STOP||step->instrument==instrument)continue;
  step->instrument=instrument;changes++;
 }
 if(changes)e->dirty=1u;
 return changes;
}
int track_clear_step_range(struct track_engine *e,uint8_t channel,uint8_t first,uint8_t last){
 if(!e||channel>=TRACK_CHANNELS)return 0;
 uint8_t lo=first<last?first:last,hi=first>last?first:last;
 if(lo>=e->active.length)return 0;if(hi>=e->active.length)hi=(uint8_t)(e->active.length-1u);
 int count=0;
 for(uint8_t i=lo;i<=hi;i++){
  (void)track_clear_step(e,channel,i);
  if(channel==TRACK_DRUM_CHANNEL){
   e->active.note2_fx[i].effect=0u;e->active.note2_fx[i].value=0u;
  }
  count++;
 }
 return count;
}
/* Clear a complete lane including inactive rows, to prevent notes returning when
 * pattern length is increased. Do not modify other channels, metadata, or song. */
int track_clear_lane(struct track_engine *e,uint8_t channel){
 if(!e||channel>=TRACK_CHANNELS)return -1;
 for(uint8_t row=0u;row<TRACK_STEPS;row++){
  (void)track_clear_step(e,channel,row);
  if(channel==TRACK_DRUM_CHANNEL){
   e->active.note2_fx[row].effect=0u;
   e->active.note2_fx[row].value=0u;
  }
 }
 e->dirty=1u;
 return 0;
}
/* Hardware keyboard note entry ALWAYS targets NOTE, regardless of the selected
 * INST/CHD/FX edit column, preserving those other cell values. Advance once
 * per accepted note; on the last active row wrap to 00. */
int track_enter_note(struct track_engine *e,uint8_t note){
 if(!e || e->transport.edit_channel>=TRACK_CHANNELS ||
    e->transport.edit_step>=TRACK_STEPS)return -1;
 if(e->transport.edit_channel==TRACK_DRUM_CHANNEL && note>=TRACK_DRUM_NOTE_COUNT && note!=TRACK_NOTE_STOP && note!=TRACK_NOTE_EMPTY)return -1;
 if(track_set_note(e,e->transport.edit_channel,e->transport.edit_step,note)!=0)return -1;
 e->transport.edit_step=(uint8_t)((e->transport.edit_step+1u)%(e->active.length>=1u&&e->active.length<=TRACK_STEPS?e->active.length:TRACK_STEPS));
 return 0;
}
/* Dedicated CH6 NOTE2 keyboard input; NOTE1 and the step cursor stay unchanged. */
int track_enter_drum_note2(struct track_engine *e,uint8_t drum_note){
 if(!e || e->transport.edit_channel!=TRACK_DRUM_CHANNEL ||
    e->transport.edit_column!=1u ||
    e->transport.edit_step>=TRACK_STEPS ||
    drum_note>=TRACK_DRUM_NOTE_COUNT)return -1;
 track_note2_set(&e->active.ch[TRACK_DRUM_CHANNEL][e->transport.edit_step],drum_note);
 e->dirty=1u;
 return 0;
}
/* A held SEL range and a single focused note use the exact same atomic
 * transposition path. One semitone delta is applied to EACH stored root note.
 * Do not touch empty/STP cells, chord shapes, other lanes or instrument IDs.
 * Clamp instead of wrapping beyond MIDI 0..127. */
int track_transpose_note_range(struct track_engine *e,uint8_t channel,uint8_t first,uint8_t last,int8_t semitones){
 if(!e||channel>=TRACK_SYNTH_CHANNELS||first>=TRACK_STEPS||last>=TRACK_STEPS)return -1;
 if(first>last){uint8_t tmp=first;first=last;last=tmp;}
 int changed=0;
 for(uint8_t row=first;row<=last;row++){
  struct track_step *st=&e->active.ch[channel][row];
  if(st->note>=TRACK_NOTE_STOP)continue;
  int n=(int)st->note+(int)semitones;
  if(n<0)n=0;if(n>127)n=127;
  if(n!=st->note){st->note=(uint8_t)n;changed++;}
 }
 if(changed)e->dirty=1u;
 return changed;
}
int track_adjust_octave(struct track_engine *e,int8_t semitones){if(!e||e->transport.edit_channel>=TRACK_SYNTH_CHANNELS||e->transport.edit_step>=16u)return -1;struct track_step *s=&e->active.ch[e->transport.edit_channel][e->transport.edit_step];if(s->note>=TRACK_NOTE_STOP)return 0;int v=(int)s->note+(int)semitones;if(v<0)v=0;if(v>127)v=127;if(s->note==(uint8_t)v)return 0;s->note=(uint8_t)v;e->dirty=1u;return 1;}
void track_rotate_select(struct track_engine *e,int8_t direction){if(!e||!direction)return;if(e->transport.edit_column==2u && e->transport.edit_channel!=TRACK_DRUM_CHANNEL){struct track_step *s=&e->active.ch[e->transport.edit_channel][e->transport.edit_step];s->chord=(uint8_t)((s->chord+TRACK_CHORD_COUNT+(direction>0?1u:TRACK_CHORD_COUNT-1u))%TRACK_CHORD_COUNT);e->dirty=1u;}else{e->transport.edit_channel=(uint8_t)((e->transport.edit_channel+TRACK_CHANNELS+(direction>0?1u:TRACK_CHANNELS-1u))%TRACK_CHANNELS);}}
void track_set_channel(struct track_engine *e,uint8_t ch){if(e&&ch<TRACK_CHANNELS)e->transport.edit_channel=ch;}
void track_set_bpm(struct track_engine *e,uint16_t bpm){if(!e)return;if(bpm<30u)bpm=30u;if(bpm>300u)bpm=300u;e->transport.bpm=bpm;e->dirty=1u;}
int track_set_pattern_length(struct track_engine *e,uint8_t length){
 if(!e||length<1u||length>TRACK_STEPS||e->transport.playing)return -1;
 e->active.length=length;
 if(e->transport.edit_step>=length)e->transport.edit_step=(uint8_t)(length-1u);
 e->dirty=1u;return 0;
}
void track_play(struct track_engine *e){if(!e)return;e->transport.stop_step=e->transport.edit_step;e->transport.play_step=0u;e->transport.playing=1u;}
void track_stop(struct track_engine *e){
 if(!e)return;
 e->transport.playing=0u;
 uint8_t len=(e->active.length>=1u && e->active.length<=TRACK_STEPS)?e->active.length:TRACK_STEPS;
 e->transport.edit_step=e->transport.stop_step<len?e->transport.stop_step:(uint8_t)(len-1u);
 e->punch_mask=0u;
}
int track_tick(struct track_engine *e){if(!e||!e->transport.playing)return 0;uint8_t length=e->active.length?bounded(e->active.length,TRACK_STEPS+1u):TRACK_STEPS;e->transport.play_step=(uint8_t)((e->transport.play_step+1u)%length);return 1;}
/* Scale membership is shared by edit-scrolling and playback harmony. */
static uint8_t in_scale(uint8_t n,const struct track_harmony *h){for(uint8_t k=0;k<7u;k++)if(((h->root+scale[h->mode][k])%12u)==n%12u)return 1u;return 0u;}
uint8_t track_quantize(uint8_t note,const struct track_harmony *h){if(!h||note>127u||h->mode>8u||h->root>11u||h->keylock!=2u)return note;if(in_scale(note,h))return note;for(uint8_t d=1u;d<=6u;d++){if(note>=d && in_scale((uint8_t)(note-d),h))return (uint8_t)(note-d);if((uint16_t)note+d<=127u && in_scale((uint8_t)(note+d),h))return (uint8_t)(note+d);}return note;}
uint8_t track_chord(uint8_t root,uint8_t type,uint8_t out[5]){if(!out||root>127u||type>=TRACK_CHORD_COUNT)return 0u;uint8_t n=chord[type][0];for(uint8_t k=0;k<n;k++){uint16_t note=(uint16_t)root+(uint16_t)chord[type][k+1u];out[k]=note>127u?127u:(uint8_t)note;}return n;}
/* Advance by SCALE DEGREES, not semitones. Both special tracker values
 * remain part of the cycle. This function is only used when LOCK is active;
 * GUIDE/OFF, and both drum note fields, preserve their old scroll order. */
uint8_t track_note_scroll_keylocked(uint8_t value,int32_t detents,uint8_t drum,const struct track_harmony *h){
 if(!detents||drum||!h||h->keylock!=2u||h->root>11u||h->mode>8u)
  return track_note_scroll(value,detents,drum);
 int32_t total=2;
 for(uint16_t n=0u;n<128u;n++)if(in_scale((uint8_t)n,h))total++;
 int32_t turns=detents%total;
 if(!turns)return value;
 uint8_t cursor=value;
 while(turns){
  int dir=turns>0?1:-1;
  /* 0..127 -> EMPTY -> STP -> 0; off-scale stored legacy notes are
   * skipped in the requested direction without altering saved patterns. */
  do{
   if(dir>0)cursor=cursor==TRACK_NOTE_STOP?0u:cursor==TRACK_NOTE_EMPTY?TRACK_NOTE_STOP:
     cursor>=127u?TRACK_NOTE_EMPTY:(uint8_t)(cursor+1u);
   else cursor=cursor==0u?TRACK_NOTE_STOP:cursor==TRACK_NOTE_STOP?TRACK_NOTE_EMPTY:
     cursor==TRACK_NOTE_EMPTY?127u:(uint8_t)(cursor-1u);
  }while(cursor<128u&&!in_scale(cursor,h));
  turns-=dir;
 }
 return cursor;
}
/* Chord voicing follows the selected root, preserving the requested chord
 * interval as closely as the scale permits. Prefer a lower note on ties
 * (same as track_quantize). Keep ascending voices whenever MIDI range allows.
 * At the top of the MIDI range, repeat the last in-key pitch rather than
 * emitting a note outside the scale. Never mutate stored CHD or NOTE data. */
uint8_t track_chord_keylocked(uint8_t root,uint8_t type,const struct track_harmony *h,uint8_t out[5]){
 if(!out||root>127u||type>=TRACK_CHORD_COUNT)return 0u;
 if(!h||h->keylock!=2u||h->root>11u||h->mode>8u)return track_chord(root,type,out);
 uint8_t base=track_quantize(root,h);
 uint8_t count=chord[type][0];
 out[0]=base;
 for(uint8_t i=1u;i<count;i++){
  uint16_t intended=(uint16_t)base+chord[type][i+1u];
  if(intended>127u)intended=127u;
  uint16_t minimum=(uint16_t)out[i-1u]+1u;
  int best=-1;
  /* At most 12 semitone pairs inspected per chord voice, even at high
   * BPM. Run inside the 44.1kHz step callback without unbounded scans. */
  if(minimum<=127u){
   uint16_t target=intended<minimum?minimum:intended;
   for(uint16_t distance=0u;distance<=12u;distance++){
    if(target>=minimum+distance && in_scale((uint8_t)(target-distance),h)){
     best=(int)(target-distance);break;
    }
    if(distance && target+distance<=127u && in_scale((uint8_t)(target+distance),h)){
     best=(int)(target+distance);break;
    }
   }
  }
  out[i]=best>=0?(uint8_t)best:out[i-1u];
 }
 return count;
}

void track_punch(struct track_engine *e,uint8_t fx,uint8_t held){if(!e||fx>=TRACK_PUNCH_COUNT)return;if(held)e->punch_mask|=(uint16_t)(1u<<fx);else e->punch_mask&=(uint16_t)~(1u<<fx);}
int track_is_lit(uint8_t button){return button>=2u&&button<14u&&button!=10u;}
uint32_t track_home_action(uint32_t held_ms,uint8_t released){if(held_ms>=1500u)return 2u;return released?1u:0u;}
/* TR version 2 includes dynamics explicitly. Original TR version 1 still
 * decodes with default velocity and legato gate. No architecture byte copies. */
int track_pattern_encode(const struct track_pattern *p,uint8_t *dst,uint32_t cap){
 if(!p||!dst||cap<TRACK_PATTERN_BYTES||p->length<1u||p->length>TRACK_STEPS)return -1;
 dst[0]='T';dst[1]='R';dst[2]=4u;dst[3]=p->length;uint32_t at=4u;
 for(unsigned c=0;c<TRACK_CHANNELS;c++)for(unsigned st=0;st<16u;st++){
  const struct track_step *s=&p->ch[c][st];
  if((s->note>127u&&s->note!=TRACK_NOTE_STOP&&s->note!=TRACK_NOTE_EMPTY)||s->instrument>7u||
     s->chord>=(c==TRACK_DRUM_CHANNEL?16u:TRACK_CHORD_COUNT)||s->effect>=TRACK_FX_COUNT||s->value>100u)return -1;
  dst[at++]=s->note;dst[at++]=(uint8_t)s->instrument;dst[at++]=(uint8_t)s->chord;
  dst[at++]=(uint8_t)s->effect;dst[at++]=(uint8_t)s->value;dst[at++]=(uint8_t)s->dynamics;dst[at++]=(uint8_t)s->groove;
 }
 for(unsigned st=0u;st<TRACK_STEPS;st++){
  if(p->note2_fx[st].effect>=TRACK_FX_COUNT||p->note2_fx[st].value>100u)return -1;
  dst[at++]=p->note2_fx[st].effect;dst[at++]=p->note2_fx[st].value;
 }
 return (int)at;
}
int track_pattern_decode(struct track_pattern *p,const uint8_t *src,uint32_t len){
 if(!p||!src||len<4u||src[0]!='T'||src[1]!='R'||(src[3]<1u||src[3]>TRACK_STEPS))return -1;
 uint8_t version=src[2];
 if((version<1u||version>4u)||len!=(version==1u?TRACK_PATTERN_LEGACY_BYTES:version==2u?TRACK_PATTERN_V2_BYTES:version==3u?TRACK_PATTERN_V3_BYTES:TRACK_PATTERN_BYTES))return -1;
 struct track_pattern next={0};next.length=src[3];uint32_t at=4u;
 for(unsigned c=0;c<TRACK_CHANNELS;c++)for(unsigned st=0;st<16u;st++){
  struct track_step *s=&next.ch[c][st];
  s->note=src[at++];s->instrument=src[at++];s->chord=src[at++];
  s->effect=src[at++];s->value=src[at++];s->dynamics=version>=2u?src[at++]:0u;s->groove=version>=3u?src[at++]:0u;
  if((s->note>127u&&s->note!=TRACK_NOTE_STOP&&s->note!=TRACK_NOTE_EMPTY)||s->instrument>7u||
      s->chord>=(c==TRACK_DRUM_CHANNEL?16u:TRACK_CHORD_COUNT)||s->effect>=TRACK_FX_COUNT||s->value>100u)return -1;
 }
 for(unsigned st=0u;st<TRACK_STEPS;st++){
  next.note2_fx[st].value=version>=4u?src[at+1u]:50u;
  next.note2_fx[st].effect=version>=4u?src[at]:0u;
  if(version>=4u)at+=2u;
  if(next.note2_fx[st].effect>=TRACK_FX_COUNT||next.note2_fx[st].value>100u)return -1;
 }
 memcpy(p,&next,sizeof(next));return 0;
}
