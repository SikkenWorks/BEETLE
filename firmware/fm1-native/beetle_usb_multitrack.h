/* SPDX-License-Identifier: GPL-3.0-only
 * BEETLE native UAC1 six-channel capture: exact, bounded fixed-point maths.
 * All channels share ONE interpolation phase and ONE USB sample clock.
 */
#ifndef BEETLE_USB_MULTITRACK_H
#define BEETLE_USB_MULTITRACK_H
#include <stdint.h>
#define BEETLE_UAC_MT_ONE 65536u
/* Exact signed Q16 linear interpolation without a 64-bit multiply.
 * diff is in [-65535,65535] for any pair of PCM16 samples. Each product
 * (diff * byte) fits comfortably inside signed 32-bit. For negative deltas,
 * GCC's arithmetic right shift agrees with the previous >>16 expression.
 * The decomposition is mathematically exact, not 8-bit approximation.
 */
static inline int16_t beetle_uac_mt_lerp(int16_t a,int16_t b,uint16_t fraction){
 int32_t diff=(int32_t)b-(int32_t)a;
 int32_t hi=diff*(int32_t)(fraction>>8);
 int32_t lo=(diff*(int32_t)(fraction&255u))>>8;
 int32_t sample=(int32_t)a+((hi+lo)>>8);
 return (int16_t)sample;
}
/* Smooth measured ring fill at one observation PER AUDIO BLOCK, not at each
 * USB packet. This rejects the 44/45 USB packet phase jitter from the SRC
 * control loop. fill_q8 is signed to avoid unsigned underflow. */
static inline int32_t beetle_uac_mt_filter_fill(int32_t fill_q8,uint32_t frames){
 int32_t observed=(int32_t)frames*256;
 return fill_q8+(observed-fill_q8)/16;
}
/* Bound the corrective change to +/-0.098% instead of legacy +/-0.6%.
 * No division or multiplication wider than 32 bits in the USB ISR.
 * One call per audio render; convergence is intentionally gradual.
 * A stable near-nominal 44.1 kHz codec needs only a few tens of Q16 units.
 */
static inline uint32_t beetle_uac_mt_next_step(uint32_t current,int32_t fill_q8,uint32_t target){
 int32_t error=(fill_q8-(int32_t)target*256)/256;
 if(error>32)error=32;
 if(error< -32)error=-32;
 int32_t wanted=(int32_t)BEETLE_UAC_MT_ONE+2*error;
 int32_t delta=wanted-(int32_t)current;
 if(delta>0)current+=(uint32_t)((delta+63)/64);
 else if(delta<0)current-=(uint32_t)((-delta+63)/64);
 return current;
}
#endif
