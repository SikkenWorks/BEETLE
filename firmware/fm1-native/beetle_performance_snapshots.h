/* BEETLE V0.67: four fixed-size performance mute snapshots.
 * Stored per project; recall shares the existing exact audio-boundary mute queue.
 * No allocation or blocking operations in the audio callback.
 * SPDX-License-Identifier: GPL-3.0-only */
#ifndef BEETLE_PERFORMANCE_SNAPSHOTS_H
#define BEETLE_PERFORMANCE_SNAPSHOTS_H
#include "beetle_quantized_mute.h"
#include <stdint.h>
#define BEETLE_SNAPSHOT_SLOTS 4u
#define BEETLE_SNAPSHOT_MASK ((1u<<TRACK_CHANNELS)-1u)
struct beetle_performance_snapshots {uint8_t valid; uint8_t masks[BEETLE_SNAPSHOT_SLOTS];};
static inline void beetle_snapshots_init(struct beetle_performance_snapshots *s){
 s->valid=0u;for(uint8_t i=0;i<BEETLE_SNAPSHOT_SLOTS;i++)s->masks[i]=0u;
}
static inline uint8_t beetle_snapshot_mask(const struct track_mixer *m){
 uint8_t bits=0u;for(uint8_t i=0;i<TRACK_CHANNELS;i++)if(m->ch[i].mute)bits|=(uint8_t)(1u<<i);
 return bits;
}
static inline uint8_t beetle_snapshot_store(struct beetle_performance_snapshots *s,uint8_t slot,const struct track_mixer *m){
 if(slot>=BEETLE_SNAPSHOT_SLOTS)return 0u;
 uint8_t bit=(uint8_t)(1u<<slot),mask=beetle_snapshot_mask(m);
 uint8_t changed=(uint8_t)(!(s->valid&bit)||s->masks[slot]!=mask);
 s->valid|=bit;s->masks[slot]=mask;return changed;
}
/* Undefined snapshots never alter existing queued or live mutes. For defined
 * snapshots the target mask replaces every queued lane state atomically.
 * A return of 1 also reports cancellation of a pending change. */
static inline uint8_t beetle_snapshot_recall(const struct beetle_performance_snapshots *s,uint8_t slot,struct beetle_mute_queue *q,struct track_mixer *m,uint8_t playing){
 if(slot>=BEETLE_SNAPSHOT_SLOTS||!(s->valid&(1u<<slot)))return 0u;
 uint8_t mask=s->masks[slot],old_pending=q->pending,old_values=q->values;
 if(playing&&m->quantize_mutes){
  beetle_mute_cancel(q);
  for(uint8_t i=0;i<TRACK_CHANNELS;i++)beetle_mute_arm(q,m,i,(uint8_t)((mask>>i)&1u));
  return (uint8_t)(old_pending!=q->pending||old_values!=q->values);
 }
 beetle_mute_cancel(q);
 uint8_t changed=(uint8_t)(old_pending!=0u);
 for(uint8_t i=0;i<TRACK_CHANNELS;i++){
  uint8_t next=(uint8_t)((mask>>i)&1u);
  if(m->ch[i].mute!=next){m->ch[i].mute=next;changed=1u;}
 }
 return changed;
}
#endif
