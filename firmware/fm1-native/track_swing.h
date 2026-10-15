/* BEETLE: selective 16th-note swing. 25% of the 16th step delayed on odd
 * steps ONLY for opted-in lanes. Even steps and all other lanes remain
 * on-grid; clock/BPM, song-wrap and step length are not changed. */
#ifndef TRACK_SWING_H
#define TRACK_SWING_H
#include <stdint.h>
#include "track_mixer.h"
static inline uint8_t track_swing_mask(const struct track_mixer *m,uint8_t step){
 if(!m||(step&1u)==0u)return 0u;
 uint8_t mask=0u;
 for(uint8_t ch=0u;ch<TRACK_MIXER_CHANNELS;ch++)if(m->ch[ch].swing)mask|=(uint8_t)(1u<<ch);
 return mask;
}
static inline uint32_t track_swing_delay(uint32_t interval,uint8_t percent){
 if(percent>100u)percent=100u;return (uint32_t)(((uint64_t)interval*45u*percent)/10000u);
}
#endif
