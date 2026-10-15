/* BEETLE V0.66 - fixed-capacity live quantized mute queue.
 * Changes to the mixer during transport playback are armed for the NEXT
 * pattern boundary. No heap or callbacks; queue remains transient. V0.66 serializes only the ON/OFF preference.
 * SPDX-License-Identifier: GPL-3.0-only */
#ifndef BEETLE_QUANTIZED_MUTE_H
#define BEETLE_QUANTIZED_MUTE_H
#include "track_mixer.h"
#include <stdint.h>
struct beetle_mute_queue {uint8_t pending,values;};
static inline void beetle_mute_cancel(struct beetle_mute_queue *q){q->pending=0u;q->values=0u;}
static inline uint8_t beetle_mute_next(const struct beetle_mute_queue *q,const struct track_mixer *mix,uint8_t lane){
 if(lane>=TRACK_CHANNELS)return 0u;
 return (q->pending&(1u<<lane))?(uint8_t)((q->values>>lane)&1u):mix->ch[lane].mute;
}
static inline uint8_t beetle_mute_arm(struct beetle_mute_queue *q,const struct track_mixer *mix,uint8_t lane,uint8_t value){
 if(lane>=TRACK_CHANNELS)return 0u;
 const uint8_t bit=(uint8_t)(1u<<lane);value=value?1u:0u;
 const uint8_t before=beetle_mute_next(q,mix,lane);
 if(value==mix->ch[lane].mute){q->pending&=(uint8_t)~bit;q->values&=(uint8_t)~bit;}
 else{q->pending|=bit;if(value)q->values|=bit;else q->values&=(uint8_t)~bit;}
 return before!=value;
}
static inline uint8_t beetle_mute_toggle(struct beetle_mute_queue *q,const struct track_mixer *mix,uint8_t lane){
 if(lane>=TRACK_CHANNELS)return 0u;
 return beetle_mute_arm(q,mix,lane,(uint8_t)!beetle_mute_next(q,mix,lane));
}
static inline uint8_t beetle_mute_commit(struct beetle_mute_queue *q,struct track_mixer *mix){
 uint8_t changed=0u;
 for(uint8_t c=0u;c<TRACK_CHANNELS;c++)if(q->pending&(1u<<c)){
  const uint8_t target=(uint8_t)((q->values>>c)&1u);
  if(mix->ch[c].mute!=target){mix->ch[c].mute=target;changed=1u;}
 }
 beetle_mute_cancel(q);return changed;
}
#endif
