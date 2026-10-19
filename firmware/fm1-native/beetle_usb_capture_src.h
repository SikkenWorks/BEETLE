/* SPDX-License-Identifier: GPL-3.0-only
 * Fixed-rate UAC1 capture clock bridge (44.1 kHz USB host, independent
 * nominally 44.1 kHz FM1 codec clock). No allocations or float in IRQ.
 * A bounded, slowly varying fractional read position avoids packet-size
 * modulation, duplicate PCM frames and hard jumps for clock drift.
 */
#ifndef BEETLE_USB_CAPTURE_SRC_H
#define BEETLE_USB_CAPTURE_SRC_H
#include <stdint.h>
#define BEETLE_UAC_SRC_ONE 65536u
static inline uint32_t beetle_usb_src_next_step(uint32_t current,uint32_t minimum_fill,uint32_t target_fill){
 int32_t error=(int32_t)minimum_fill-(int32_t)target_fill;
 int32_t wanted,delta;
 if(error>64)error=64;
 if(error< -64)error=-64;
 wanted=(int32_t)BEETLE_UAC_SRC_ONE+6*error;
 /* +/- 0.6% tuning range. One-pole lowpass protects pitch continuity.
  * The modulo rate is still precisely 44,100 USB frames per second. */
 delta=wanted-(int32_t)current;
 if(delta>0)current+=(uint32_t)((delta+31)/32);
 else if(delta<0)current-=(uint32_t)((-delta+31)/32);
 return current;
}
static inline uint32_t beetle_usb_src_lerp(uint32_t a,uint32_t b,uint16_t frac){
 /* Round to 8-bit interpolant to keep ISR work down on pi32v2.
  * Error <= 1/256 of the difference between consecutive samples. */
 int32_t x=(int16_t)(a&65535u),y=(int16_t)(b&65535u);
 int32_t xx=(int16_t)(a>>16u),yy=(int16_t)(b>>16u);
 int32_t q=(int32_t)(frac>>8u);
 int32_t left=x+((y-x)*q+128)/256;
 int32_t right=xx+((yy-xx)*q+128)/256;
 if(left>32767)left=32767;else if(left< -32768)left=-32768;
 if(right>32767)right=32767;else if(right< -32768)right=-32768;
 return (uint32_t)(uint16_t)left|((uint32_t)(uint16_t)right<<16u);
}
#endif
