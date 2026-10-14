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
void track_init(struct track_engine *e){if(!e)return;memset(e,0,sizeof(*e));e->transport.bpm=120u;e->active.length=16u;for(unsigned c=0;c<TRACK_CHANNELS;c++)for(unsigned st=0;st<TRACK_STEPS;st++){e->active.ch[c][st].note=255u;e->active.ch[c][st].instrument=(uint8_t)(c==TRACK_DRUM_CHANNEL?3u:(c&3u));e->active.ch[c][st].value=50u;}}
/* Scroll order: note 0..last, EMPTY, STP, then loop. Cope with stale data. */
uint8_t track_note_scroll(uint8_t note,int32_t delta,uint8_t drum){
 const int32_t count=drum?TRACK_DRUM_COUNT:128;
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
 s->note=TRACK_NOTE_EMPTY;s->instrument=(uint8_t)(channel==TRACK_DRUM_CHANNEL?3u:(channel&3u));
 s->chord=0u;s->effect=0u;s->value=50u;e->dirty=1u;return 0;
}
/* Hardware keyboard note entry: advance once per accepted note, even in
 * the drum lane. At the final step wrap to 00 in the CURRENT pattern.
 * K3 pitch adjustments and NOTE2/CHD edits deliberately do not advance. */
int track_enter_note(struct track_engine *e,uint8_t note){
 if(!e || e->transport.edit_channel>=TRACK_CHANNELS ||
    e->transport.edit_step>=TRACK_STEPS || e->transport.edit_column!=0u)return -1;
 if(e->transport.edit_channel==TRACK_DRUM_CHANNEL && note>=TRACK_DRUM_COUNT && note!=TRACK_NOTE_STOP && note!=TRACK_NOTE_EMPTY)return -1;
 if(track_set_note(e,e->transport.edit_channel,e->transport.edit_step,note)!=0)return -1;
 e->transport.edit_step=(uint8_t)((e->transport.edit_step+1u)%TRACK_STEPS);
 return 0;
}
int track_adjust_octave(struct track_engine *e,int8_t semitones){if(!e||e->transport.edit_channel>=TRACK_CHANNELS||e->transport.edit_step>=16u)return -1;struct track_step *s=&e->active.ch[e->transport.edit_channel][e->transport.edit_step];if(s->note>=TRACK_NOTE_STOP)return 0;int v=(int)s->note+(int)semitones;if(v<0)v=0;if(v>127)v=127;s->note=(uint8_t)v;e->dirty=1u;return 1;}
void track_rotate_select(struct track_engine *e,int8_t direction){if(!e||!direction)return;if(e->transport.edit_column==2u && e->transport.edit_channel!=TRACK_DRUM_CHANNEL){struct track_step *s=&e->active.ch[e->transport.edit_channel][e->transport.edit_step];s->chord=(uint8_t)((s->chord+TRACK_CHORD_COUNT+(direction>0?1u:TRACK_CHORD_COUNT-1u))%TRACK_CHORD_COUNT);e->dirty=1u;}else{e->transport.edit_channel=(uint8_t)((e->transport.edit_channel+TRACK_CHANNELS+(direction>0?1u:TRACK_CHANNELS-1u))%TRACK_CHANNELS);}}
void track_set_channel(struct track_engine *e,uint8_t ch){if(e&&ch<TRACK_CHANNELS)e->transport.edit_channel=ch;}
void track_set_bpm(struct track_engine *e,uint16_t bpm){if(!e)return;if(bpm<30u)bpm=30u;if(bpm>300u)bpm=300u;e->transport.bpm=bpm;e->dirty=1u;}
void track_play(struct track_engine *e){if(!e)return;e->transport.stop_step=e->transport.edit_step;e->transport.play_step=0u;e->transport.playing=1u;}
void track_stop(struct track_engine *e){if(!e)return;e->transport.playing=0u;e->transport.edit_step=e->transport.stop_step;e->punch_mask=0u;}
int track_tick(struct track_engine *e){if(!e||!e->transport.playing)return 0;uint8_t length=e->active.length?bounded(e->active.length,TRACK_STEPS+1u):TRACK_STEPS;e->transport.play_step=(uint8_t)((e->transport.play_step+1u)%length);return 1;}
static uint8_t in_scale(uint8_t n,const struct track_harmony *h){for(uint8_t k=0;k<7u;k++)if(((h->root+scale[h->mode][k])%12u)==n%12u)return 1u;return 0u;}
uint8_t track_quantize(uint8_t note,const struct track_harmony *h){if(!h||note>127u||h->mode>8u||h->root>11u||h->keylock!=2u)return note;if(in_scale(note,h))return note;for(uint8_t d=1u;d<=6u;d++){if(note>=d && in_scale((uint8_t)(note-d),h))return (uint8_t)(note-d);if((uint16_t)note+d<=127u && in_scale((uint8_t)(note+d),h))return (uint8_t)(note+d);}return note;}
uint8_t track_chord(uint8_t root,uint8_t type,uint8_t out[5]){if(!out||root>127u||type>=TRACK_CHORD_COUNT)return 0u;uint8_t n=chord[type][0];for(uint8_t k=0;k<n;k++){uint16_t note=(uint16_t)root+(uint16_t)chord[type][k+1u];out[k]=note>127u?127u:(uint8_t)note;}return n;}
void track_punch(struct track_engine *e,uint8_t fx,uint8_t held){if(!e||fx>=TRACK_PUNCH_COUNT)return;if(held)e->punch_mask|=(uint8_t)(1u<<fx);else e->punch_mask&=(uint8_t)~(1u<<fx);}
int track_is_lit(uint8_t button){return button>=2u&&button<14u&&button!=10u;}
uint32_t track_home_action(uint32_t held_ms,uint8_t released){if(held_ms>=1500u)return 2u;return released?1u:0u;}
int track_pattern_encode(const struct track_pattern *p,uint8_t *dst,uint32_t cap){if(!p||!dst||cap<TRACK_PATTERN_BYTES||p->length!=16u)return -1;dst[0]='T';dst[1]='R';dst[2]=1u;dst[3]=p->length;uint32_t at=4u;for(unsigned c=0;c<TRACK_CHANNELS;c++)for(unsigned st=0;st<16u;st++){const struct track_step *s=&p->ch[c][st];if((s->note>127u&&s->note!=TRACK_NOTE_STOP&&s->note!=TRACK_NOTE_EMPTY)||s->instrument>3u||(s->chord>=(c==TRACK_DRUM_CHANNEL?15u:TRACK_CHORD_COUNT))||s->effect>=TRACK_FX_COUNT||s->value>100u)return -1;dst[at++]=s->note;dst[at++]=s->instrument;dst[at++]=s->chord;dst[at++]=s->effect;dst[at++]=s->value;}return (int)at;}
int track_pattern_decode(struct track_pattern *p,const uint8_t *src,uint32_t len){if(!p||!src||len!=TRACK_PATTERN_BYTES||src[0]!='T'||src[1]!='R'||src[2]!=1u||src[3]!=16u)return -1;struct track_pattern next;next.length=src[3];uint32_t at=4u;for(unsigned c=0;c<TRACK_CHANNELS;c++)for(unsigned st=0;st<16u;st++){struct track_step *s=&next.ch[c][st];s->note=src[at++];s->instrument=src[at++];s->chord=src[at++];s->effect=src[at++];s->value=src[at++];if((s->note>127u&&s->note!=TRACK_NOTE_STOP&&s->note!=TRACK_NOTE_EMPTY)||s->instrument>3u||(s->chord>=(c==TRACK_DRUM_CHANNEL?15u:TRACK_CHORD_COUNT))||s->effect>=TRACK_FX_COUNT||s->value>100u)return -1;}memcpy(p,&next,sizeof(next));return 0;}
