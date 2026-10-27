/* SPDX-License-Identifier: GPL-3.0-only
 * BEETLE hardware master freshness + physical DAC safety policy.
 * Software fail-closed: stale/invalid ADC may NEVER hold a prior high gain.
 */
#ifndef BEETLE_AUDIO_SAFETY_H
#define BEETLE_AUDIO_SAFETY_H
#include <stdint.h>
#define BEETLE_MASTER_ADC_MAX 1023
#define BEETLE_MASTER_STALE_MS 80u
#define BEETLE_MASTER_RISE_PER_ADC_POLL 2048u
static inline uint16_t beetle_master_from_adc(int32_t adc){
 if(adc<0 || adc>BEETLE_MASTER_ADC_MAX)return 0u;
 return (uint16_t)((uint32_t)adc*32767u/BEETLE_MASTER_ADC_MAX);
}
/* Drop instantly, but rate-limit any upward ADC spike to a bounded step.
 * The 20ms foreground polling cadence requires ~0.32s for a full rise. */
static inline uint16_t beetle_master_publish_step(uint16_t previous,int32_t adc){
 if(previous>32767u)return 0u;
 uint16_t target=beetle_master_from_adc(adc);
 if(target<=previous)return target;
 const uint32_t limit=(uint32_t)previous+BEETLE_MASTER_RISE_PER_ADC_POLL;
 return (uint16_t)(target<limit?target:limit);
}
static inline uint16_t beetle_master_safe_gain(uint16_t published,uint32_t now_ms,uint32_t last_valid_ms){
 /* Unsigned elapsed time also handles the 32-bit millisecond rollover. */
 if(published>32767u || (uint32_t)(now_ms-last_valid_ms)>BEETLE_MASTER_STALE_MS)return 0u;
 return published;
}
#endif
