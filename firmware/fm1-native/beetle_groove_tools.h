/* BEETLE V0.64: allocation-free, pattern-length-bounded step groove tools.
 * Only groove bits change; notes, NOTE2, effects, and dynamics are preserved.
 * SPDX-License-Identifier: GPL-3.0-only */
#ifndef BEETLE_GROOVE_TOOLS_H
#define BEETLE_GROOVE_TOOLS_H
#include "track_core.h"
#include <stdint.h>
struct beetle_groove_clip {uint8_t value,valid;};
static inline void beetle_groove_copy(struct beetle_groove_clip *clip,const struct track_step *step){
 clip->value=step->groove&127u;clip->valid=1u;
}
static inline uint8_t beetle_groove_paste(const struct beetle_groove_clip *clip,struct track_step *step){
 if(!clip->valid)return 0u;
 uint8_t value=clip->value&127u;
 if(step->groove==value)return 0u;
 step->groove=value;return 1u;
}
static inline uint8_t beetle_groove_fill(struct track_pattern *pat,uint8_t lane,uint8_t groove){
 if(!pat||lane>=TRACK_CHANNELS)return 0u;
 uint8_t length=pat->length;
 if(length<1u||length>TRACK_STEPS)length=TRACK_STEPS;
 uint8_t changed=0u;
 for(uint8_t row=0u;row<length;row++){
  struct track_step *step=&pat->ch[lane][row];
  if(step->groove!=(groove&127u)){step->groove=groove&127u;changed=1u;}
 }
 return changed;
}
static inline uint8_t beetle_groove_clear(struct track_pattern *pat,uint8_t lane){
 return beetle_groove_fill(pat,lane,0u);
}
#endif
