/* SPDX-License-Identifier: GPL-3.0-only
 * BEETLE V1.2 oscillator calibration. Measured with the original FM1 C
 * renderer: C4, sustain=100, velocity=110, clean patch, no MASTER FX.
 * These fixed gains reduce RMS jumps when browsing instrument/wave types.
 * They are deliberately STATIC, not a per-note AGC: no attack pumping,
 * distortion of envelopes, or modulation-dependent volume chasing.
 * Drive, velocity, mixer, ADSR and user-defined gains are unaffected.
 */
#ifndef BEETLE_WAVE_BALANCE_H
#define BEETLE_WAVE_BALANCE_H
#include <stdint.h>
static inline uint16_t beetle_shape_gain_q8(uint8_t shape){
 static const uint16_t gain[5]={323u,314u,185u,213u,367u};
 return shape<5u?gain[shape]:256u;
}
static inline uint16_t beetle_type_gain_q8(uint8_t type){
 /* RETRO PULSE,WAVE, VA A/B, SID variants, NES variants. */
 static const uint16_t gain[12]={105u,245u,256u,345u,410u,403u,
                                   631u,347u,525u,214u,181u,244u};
 return type<12u?gain[type]:256u;
}
static inline int32_t beetle_level_q8(int32_t v,uint16_t q8){
 return (int32_t)(((int64_t)v*(int32_t)q8)/256);
}
#endif
