/* BEETLE V0.68 - four non-destructive, six-lane live mute groups.
 * Group membership is project data; pending recall is transient and shares
 * the existing audio-boundary mute queue. No allocation, no audio ISR edits.
 * SPDX-License-Identifier: GPL-3.0-only */
#ifndef BEETLE_MUTE_GROUPS_H
#define BEETLE_MUTE_GROUPS_H
#include "beetle_quantized_mute.h"
#include <stdint.h>
#define BEETLE_GROUP_SLOTS 4u
#define BEETLE_GROUP_MASK ((1u<<TRACK_CHANNELS)-1u)
struct beetle_mute_groups {uint8_t masks[BEETLE_GROUP_SLOTS];};
static inline void beetle_groups_init(struct beetle_mute_groups *g){
 for(uint8_t i=0u;i<BEETLE_GROUP_SLOTS;i++)g->masks[i]=0u;
}
static inline uint8_t beetle_group_member_toggle(struct beetle_mute_groups *g,uint8_t slot,uint8_t lane){
 if(slot>=BEETLE_GROUP_SLOTS||lane>=TRACK_CHANNELS)return 0u;
 g->masks[slot]^=(uint8_t)(1u<<lane);return 1u;
}
/* If all members are currently (or queued to become) muted, unmute.
 * Otherwise mute all members. Changes to lanes outside the group, including
 * queued mutes from another control, remain completely untouched. */
static inline uint8_t beetle_group_toggle(const struct beetle_mute_groups *g,uint8_t slot,
 struct beetle_mute_queue *q,struct track_mixer *mix,uint8_t playing){
 if(slot>=BEETLE_GROUP_SLOTS)return 0u;
 uint8_t group=g->masks[slot] & BEETLE_GROUP_MASK;
 if(!group)return 0u;
 uint8_t all_muted=1u,changed=0u;
 for(uint8_t c=0u;c<TRACK_CHANNELS;c++)if(group&(1u<<c))
  if(!beetle_mute_next(q,mix,c))all_muted=0u;
 uint8_t target=(uint8_t)!all_muted;
 for(uint8_t c=0u;c<TRACK_CHANNELS;c++)if(group&(1u<<c)){
  uint8_t bit=(uint8_t)(1u<<c);
  if(playing && mix->quantize_mutes){
   uint8_t oldp=q->pending,oldv=q->values;
   beetle_mute_arm(q,mix,c,target);
   if(oldp!=q->pending||oldv!=q->values)changed=1u;
  }else{
   if(q->pending&bit){q->pending&=(uint8_t)~bit;q->values&=(uint8_t)~bit;changed=1u;}
   if(mix->ch[c].mute!=target){mix->ch[c].mute=target;changed=1u;}
  }
 }
 return changed;
}
#endif
