#ifndef BEETLE_LIVE_MIXER_H
#define BEETLE_LIVE_MIXER_H
#include <stdint.h>
/* RC1: transient Live Mixer overlay. Never writes solo/mute into project mixer.
 * A SONG OFF transition clears the entire live performance state. */
struct beetle_live_switch {uint8_t overridden,values,pending,pending_values;};
static inline void beetle_live_clear(struct beetle_live_switch *q){q->overridden=q->values=q->pending=q->pending_values=0u;}
static inline uint8_t beetle_live_value(const struct beetle_live_switch *q,uint8_t lane,uint8_t underlying){
 const uint8_t b=(uint8_t)(1u<<lane);
 return (q->overridden&b)?(uint8_t)((q->values&b)!=0u):underlying;
}
static inline uint8_t beetle_live_next(const struct beetle_live_switch *q,uint8_t lane,uint8_t underlying){
 const uint8_t b=(uint8_t)(1u<<lane);
 return (q->pending&b)?(uint8_t)((q->pending_values&b)!=0u):beetle_live_value(q,lane,underlying);
}
static inline uint8_t beetle_live_set(struct beetle_live_switch *q,uint8_t lane,uint8_t underlying,uint8_t desired,uint8_t quantized){
 const uint8_t b=(uint8_t)(1u<<lane);
 desired=desired?1u:0u;
 const uint8_t old=beetle_live_next(q,lane,underlying);
 if(quantized){
  const uint8_t active=beetle_live_value(q,lane,underlying);
  if(desired==active){q->pending&=(uint8_t)~b;q->pending_values&=(uint8_t)~b;}
  else{q->pending|=b;if(desired)q->pending_values|=b;else q->pending_values&=(uint8_t)~b;}
 }else{
  q->pending&=(uint8_t)~b;q->overridden|=b;
  if(desired)q->values|=b;else q->values&=(uint8_t)~b;
 }
 return old!=desired;
}
static inline uint8_t beetle_live_toggle(struct beetle_live_switch *q,uint8_t lane,uint8_t underlying,uint8_t quantized){
 return beetle_live_set(q,lane,underlying,(uint8_t)!beetle_live_next(q,lane,underlying),quantized);
}
static inline void beetle_live_commit(struct beetle_live_switch *q){
 q->values=(uint8_t)((q->values&~q->pending)|(q->pending_values&q->pending));
 q->overridden|=q->pending;q->pending=0u;q->pending_values=0u;
}
#endif
