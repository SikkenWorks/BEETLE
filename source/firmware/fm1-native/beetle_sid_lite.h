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
static inline int32_t beetle_sid_sample(uint32_t phase,uint32_t step,uint32_t *lfsr,
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
 /* Bounded two-integrator SVF. This is a stable SID-coloured approximation,
  * not a transistor-calibrated MOS6581 filter. Resonance uses DRIVE control. */
 int32_t f=10+(int32_t)filter_cut*110/250; /* Q8, <=120 */
 int32_t damping=220-(int32_t)resonance*130/100; /* Q8, >=90 */
 int32_t hi=input-*lp-(int32_t)(((int64_t)*bp*damping)>>8u);
 int32_t next_bp=*bp+(int32_t)(((int64_t)hi*f)>>8u);
 if(next_bp>28000)next_bp=28000;if(next_bp< -28000)next_bp=-28000;
 int32_t next_lp=*lp+(int32_t)(((int64_t)next_bp*f)>>8u);
 if(next_lp>28000)next_lp=28000;if(next_lp< -28000)next_lp=-28000;
 *lp=next_lp;*bp=next_bp;
 if(filter_mode<34u)return next_lp;
 if(filter_mode<67u)return next_bp;
 return input-next_lp;
}
#endif
