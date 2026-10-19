/* SPDX-License-Identifier: GPL-3.0-only
 * A bounded power-style polyphonic headroom curve in Q8. One audible voice
 * stays at unity; chords preserve energy without old near-linear attenuation.
 * Worst-case 8 simultaneously full-drive voices remain below hard saturation.
 */
#ifndef BEETLE_VOICE_HEADROOM_H
#define BEETLE_VOICE_HEADROOM_H
#include <stdint.h>
static inline uint16_t beetle_voice_divisor(uint8_t voices){
 static const uint16_t q8[9]={256u,256u,352u,432u,512u,576u,640u,704u,768u};
 return q8[voices>8u?8u:voices];
}
#endif
