/* SPDX-License-Identifier: GPL-3.0-only
 * BEETLE SID (reSID-inspired, not the original cycle-exact reSID library).
 * An original fixed-point 44.1 kHz SID voice suitable for an embedded DSP.
 * Inspirations: Dag Lem's MOS6581 phase/noise architecture from
 * https://github.com/libsidplayfp/resid (GPL-2.0-or-later).
 * This file contains NO code copied from upstream reSID.
 * Voices use BEETLE's existing ADSR, note allocation and channel FX.
 * No allocation, floating point, shared global oscillator state or division
 * by runtime quantities within the sample loop.
 */
#ifndef BEETLE_SID_LITE_H
#define BEETLE_SID_LITE_H
#include <stdint.h>
/* 32-bit phase represents a 24-bit SID accumulator with eight fractional bits.
 * Bits 31..20 are the 12-bit DAC output, bit 27 is SID noise clock bit 19. */
static inline int32_t beetle_sid_wave(uint8_t shape,uint32_t phase,uint8_t width,uint32_t noise){
 uint32_t ph=phase>>20u;
 switch(shape){
 case 0u:return (int32_t)ph; /* saw */
 case 1u:{uint32_t t=phase>>19u;return (int32_t)((t&0x1000u)?(0x1fffu-t):t);} /* triangle */
 case 2u:case 4u:{uint32_t pw=(uint32_t)width*4095u/100u;
  return ph<pw?4095:0;}
 case 3u:{ /* The noise shift register is clocked separately from the phase. */
  uint32_t n=noise;
  return (((n>>22u)&1u)<<11u)|(((n>>20u)&1u)<<10u)|
    (((n>>16u)&1u)<<9u)|(((n>>13u)&1u)<<8u)|
    (((n>>11u)&1u)<<7u)|(((n>>7u)&1u)<<6u)|
    (((n>>4u)&1u)<<5u)|(((n>>2u)&1u)<<4u);
 }
 default:return 2048;
 }
}
/* Distinct, bounded SID-inspired analogue stages. Both use 12-bit SID-style
 * digital waves, but the 6581 uses a non-linear cutoff response, asymmetric
 * transistor-like predrive and stronger low-frequency colouring. The 8580
 * is intentionally more linear, with a brighter cutoff mapping. These are
 * *approximations*, NOT cycle-accurate reSID MOS chip models.
 *
 * Inputs are validated by the project codec (0..100 for most controls).
 * All arithmetic is fixed-point; constant divisors are compile-time; no
 * allocations, float, tables in RAM, or potentially unbounded feedback.
 */
#define BEETLE_SID_6581 4u
#define BEETLE_SID_8580 5u
#define BEETLE_SID_DIRTY 6u
#define BEETLE_SID_BASS 7u
#define BEETLE_SID_NOISE 8u
static inline int32_t beetle_sid_clamp(int32_t x,int32_t bound){
 return x>bound?bound:(x < -bound?-bound:x);
}
static inline int32_t beetle_sid_6581_nonlinear(int32_t x){
 /* Asymmetric, smooth-ish knees; the response is *not* a simple gain boost.
  * Subtracting a small DC bias after saturation keeps the idle input quiet. */
 x=(x*11)/10+500;
 if(x>5500)x=5500+(x-5500)/4;
 if(x< -4700)x=-4700+(x+4700)/3;
 return beetle_sid_clamp(x-500,11500);
}
static inline int32_t beetle_sid_sample(uint8_t chip,uint32_t phase,uint32_t step,uint32_t *lfsr,
 int32_t *lp,int32_t *bp,uint8_t wave_a,uint8_t wave_b,uint8_t width,
 uint8_t width_b,uint8_t mix,uint8_t filter_cut,uint8_t resonance,uint8_t filter_mode){
 uint32_t last=phase-step;
 if(((last^phase)&phase&(1u<<27u))!=0u){
  uint32_t fb=((*lfsr>>22u)^(*lfsr>>17u))&1u;
  *lfsr=((*lfsr<<1u)|fb)&0x7fffffu;
  if(!*lfsr)*lfsr=0x7ffff8u;
 }
 int32_t a=beetle_sid_wave(wave_a,phase,width,*lfsr);
 int32_t b=beetle_sid_wave(wave_b,phase,width_b,*lfsr);
 int32_t input=((a*(100-(int32_t)mix)+b*(int32_t)mix)/100-2048)*6;
 const uint8_t is8580=(chip==BEETLE_SID_8580||chip==BEETLE_SID_NOISE);
 if(!is8580) input=beetle_sid_6581_nonlinear(input);
 /* 6581: distinctly non-linear cutoff, darker at mid-settings; 8580:
  * smooth, near-linear cutoff and cleaner resonance. Bound Q8 integrator
  * coefficient below 128 at the maximum UI cutoff. */
 int32_t f=is8580 ? 12+(int32_t)filter_cut*84/125
                   : 9+(int32_t)filter_cut*(int32_t)filter_cut*31/10000;
 if(f>124)f=124;
 int32_t damping=is8580 ? 227-(int32_t)resonance*152/100
                        : 204-(int32_t)resonance*119/100;
 /* Keep the feedback bounded even with pathological knob positions. */
 if(damping<74)damping=74;
 int32_t hi=input-*lp-(int32_t)(((int64_t)*bp*damping)>>8u);
 hi=beetle_sid_clamp(hi,28000);
 int32_t next_bp=*bp+(int32_t)(((int64_t)hi*f)>>8u);
 next_bp=beetle_sid_clamp(next_bp,28000);
 int32_t next_lp=*lp+(int32_t)(((int64_t)next_bp*f)>>8u);
 next_lp=beetle_sid_clamp(next_lp,28000);
 *lp=next_lp;*bp=next_bp;
 int32_t output=filter_mode<34u?next_lp:(filter_mode<67u?next_bp:input-next_lp);
 /* Output coloration changes timbre, not merely loudness. 6581 asymmetry
  * survives filtering; 8580 remains free of this additional saturation. */
 if(!is8580){
  /* Compensate ~1.2 dB attenuation from the 6581 nonlinear stages so that
   * a character change is not mistaken for a simple level change. */
  output=(beetle_sid_6581_nonlinear(output)*115)/100;
 }
 return beetle_sid_clamp(output,16000);
}
#endif
