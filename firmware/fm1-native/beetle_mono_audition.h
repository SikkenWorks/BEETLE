/* SPDX-License-Identifier: GPL-3.0-only
 * BEETLE original monophonic EDIT keyboard arbitration.
 * Main-thread-only, zero allocation, no audio/flash/LCD calls.
 * Notes are pressed immediately; release is stable after a short low interval.
 */
#ifndef BEETLE_MONO_AUDITION_H
#define BEETLE_MONO_AUDITION_H
#include <stdint.h>
#define BEETLE_AUDITION_KEYS 27u
#define BEETLE_AUDITION_RELEASE_MS 8u
#define BEETLE_AUDITION_NONE 255u
struct beetle_mono_audition {
 uint32_t stable,last_ms;
 uint8_t current,initialized;
};
struct beetle_mono_action {uint8_t kind,key;}; /* 0 none, 1 note on, 2 release */
static inline void beetle_mono_reset(struct beetle_mono_audition *s){
 uint8_t *bytes=(uint8_t*)s;for(uint32_t i=0u;i<sizeof(*s);i++)bytes[i]=0u;
 s->current=BEETLE_AUDITION_NONE;
}
static inline struct beetle_mono_action beetle_mono_scan(struct beetle_mono_audition *s,uint32_t physical,uint32_t now,
 uint8_t priority[BEETLE_AUDITION_KEYS],uint8_t release_age[BEETLE_AUDITION_KEYS]){
 physical &= (1u<<BEETLE_AUDITION_KEYS)-1u;
 uint32_t elapsed=0u;
 if(s->initialized){elapsed=(uint32_t)(now-s->last_ms);if(elapsed>255u)elapsed=255u;}
 else s->initialized=1u;
 s->last_ms=now;
 const uint32_t new_keys=physical&~s->stable;
 for(uint8_t k=0u;k<BEETLE_AUDITION_KEYS;k++){
  const uint32_t bit=1u<<k;
  if(physical&bit){
   release_age[k]=0u;
   if(new_keys&bit){
    /* Age the held keys, so the newest key has priority without a
     * wrapping counter or allocating 108 bytes of 32-bit serials. */
    for(uint8_t other=0u;other<BEETLE_AUDITION_KEYS;other++)
     if((s->stable&(1u<<other))&&priority[other])priority[other]--;
    s->stable|=bit;priority[k]=255u;
   }
  }else if(s->stable&bit){
   uint32_t age=(uint32_t)release_age[k]+elapsed;
   release_age[k]=(uint8_t)(age>255u?255u:age);
   if(age>=BEETLE_AUDITION_RELEASE_MS){
    s->stable&=~bit;priority[k]=0u;release_age[k]=0u;
   }
  }
 }
 uint8_t winner=BEETLE_AUDITION_NONE;uint8_t latest=0u;
 for(uint8_t k=0u;k<BEETLE_AUDITION_KEYS;k++)if(s->stable&(1u<<k)){
  if(winner==BEETLE_AUDITION_NONE||priority[k]>latest){latest=priority[k];winner=k;}
 }
 if(winner==s->current)return (struct beetle_mono_action){0u,winner};
 s->current=winner;
 return (struct beetle_mono_action){winner==BEETLE_AUDITION_NONE?2u:1u,winner};
}

#endif
