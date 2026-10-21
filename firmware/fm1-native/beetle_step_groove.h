/* BEETLE V0.63: bounded, allocation-free ratchet/micro/probability scheduling.
 * SPDX-License-Identifier: GPL-3.0-only */
#ifndef BEETLE_STEP_GROOVE_H
#define BEETLE_STEP_GROOVE_H
#include "track_core.h"
#include <stdint.h>
struct beetle_groove_lane {uint32_t at,spacing;uint8_t remaining,skip_note;};
struct beetle_groove_clock {struct beetle_groove_lane lane[TRACK_CHANNELS];uint32_t random;};
static inline void beetle_groove_reset(struct beetle_groove_clock *g){
 for(uint8_t i=0;i<TRACK_CHANNELS;i++){g->lane[i].remaining=0;g->lane[i].skip_note=0;}
 g->random=0x963A9D7Bu;
}
static inline uint32_t beetle_groove_random(struct beetle_groove_clock *g){
 uint32_t x=g->random?g->random:0x963A9D7Bu;
 x^=x<<13u;x^=x>>17u;x^=x<<5u;g->random=x;return x;
}
static inline void beetle_groove_schedule(struct beetle_groove_clock *g,
 const struct track_pattern *p,uint8_t step,uint32_t duration,
 const uint32_t swing_at[TRACK_CHANNELS]){
 for(uint8_t c=0;c<TRACK_CHANNELS;c++){
  struct beetle_groove_lane *v=&g->lane[c];
  const struct track_step *s=&p->ch[c][step];
  const uint8_t repeats=TRACK_GROOVE_RATCHET(s);
  uint32_t offset=swing_at[c]+duration*TRACK_GROOVE_MICRO(s)/64u;
  if(offset>=duration)offset=duration-1u;
  v->at=offset;v->spacing=(duration-offset)/repeats;
  if(!v->spacing)v->spacing=1u;
  /* Effects/STOP must still run once even when probability skips the hit. */
  v->remaining=(s->note==TRACK_NOTE_EMPTY && s->chord==0u)?1u:repeats;
  uint8_t chance=TRACK_GROOVE_PROB(s);
  v->skip_note=(uint8_t)((beetle_groove_random(g)%100u)>=chance);
  if(v->skip_note)v->remaining=1u;
 }
}
static inline uint8_t beetle_groove_due(struct beetle_groove_clock *g,uint32_t clock){
 uint8_t mask=0u;
 for(uint8_t c=0;c<TRACK_CHANNELS;c++){
  struct beetle_groove_lane *v=&g->lane[c];
  if(v->remaining && clock>=v->at){
   mask|=(uint8_t)(1u<<c);v->remaining--;
   v->at+=v->spacing;
  }
 }
 return mask;
}
static inline uint32_t beetle_groove_wait(const struct beetle_groove_clock *g,uint32_t clock,uint32_t maximum){
 for(uint8_t c=0;c<TRACK_CHANNELS;c++){
  const struct beetle_groove_lane *v=&g->lane[c];
  if(v->remaining && v->at>clock && v->at-clock<maximum)maximum=v->at-clock;
 }
 return maximum;
}
#endif
