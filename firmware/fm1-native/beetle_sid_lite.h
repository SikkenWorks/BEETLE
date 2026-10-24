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
/* Per-render-chunk SID coefficients. The 12 oscillator families share the
 * same DSP; SID uses dynamic cutoff, resonance and pulse widths that are
 * constant for the current chunk. Cache these outside the 44.1 kHz loop. */
struct beetle_sid_prepared {
 uint8_t shape_a,shape_b,is8580,mode,chip;
 uint8_t mix;
 uint16_t threshold_a,threshold_b;
 int32_t filter_coefficient,damping;
 uint8_t combine,sync,ring,phase_lock;
};
static inline struct beetle_sid_prepared beetle_sid_prepare(uint8_t chip,
 uint8_t wave_a,uint8_t wave_b,uint8_t width_a,uint8_t width_b,
 uint8_t mix,uint8_t filter_cut,uint8_t resonance,uint8_t filter_mode,
 uint8_t combine,uint8_t sync,uint8_t ring){
 struct beetle_sid_prepared x;
 x.shape_a=wave_a;x.shape_b=wave_b;x.chip=chip;x.mode=filter_mode;
 x.mix=mix;x.combine=combine;x.sync=sync;x.ring=ring;x.phase_lock=0u;x.threshold_a=(uint16_t)((uint32_t)width_a*4095u/100u);
 x.threshold_b=(uint16_t)((uint32_t)width_b*4095u/100u);
 x.is8580=(uint8_t)(chip==BEETLE_SID_8580||chip==BEETLE_SID_NOISE);
 /* V1.4.31: The experimental 6581 curve introduced in V1.4.30
  * drove the stock cutoff 96 to coefficient ~97 instead of ~37, making
  * 6581/DIRTY/BASS much too resonant/phasey on older saved patches.
  * Restore the stable, nonlinear 6581 mapping from V1.4.29. This is done
  * once per render chunk; no additional per-sample DSP cost. The clean 8580
  * curve is retained as it sounded in V1.4.30. Cutoff may exceed 100 in
  * legacy projects and is deliberately not truncated for 6581. */
 if(!x.is8580){
  int32_t c=(int32_t)filter_cut;
  x.filter_coefficient=9+(c*c*31)/10000;
  if(x.filter_coefficient>124)x.filter_coefficient=124;
  x.damping=204-(int32_t)resonance*119/100;
  if(x.damping<74)x.damping=74;
 }else{
  static const uint8_t curve8580[17]={7,12,17,22,27,32,37,42,47,53,59,65,71,78,86,95,105};
  if(filter_cut>100u)filter_cut=100u;
  uint32_t idx=(uint32_t)filter_cut*16u;
  uint32_t cell=idx/100u,frac=idx%100u;
  x.filter_coefficient=(int32_t)curve8580[cell];
  if(cell<16u)x.filter_coefficient+=(int32_t)(((uint32_t)(curve8580[cell+1u]-curve8580[cell])*frac+50u)/100u);
  x.damping=228-(int32_t)resonance*135/100;
  if(x.damping<80)x.damping=80;
 }
 return x;
}
static inline int32_t beetle_sid_cached_wave(uint8_t shape,uint32_t phase,
 uint16_t threshold,uint32_t noise){
 if(shape==2u||shape==4u)return (phase>>20u)<threshold?4095:0;
 return beetle_sid_wave(shape,phase,0u,noise);
}
static inline int32_t beetle_sid_sample_prepared(const struct beetle_sid_prepared *x,
 uint32_t phase,uint32_t step,uint32_t phase_b,uint32_t *lfsr,int32_t *lp,int32_t *bp){
 uint32_t last=phase-step;
 if(((last^phase)&phase&(1u<<27u))!=0u){
  uint32_t fb=((*lfsr>>22u)^(*lfsr>>17u))&1u;
  *lfsr=((*lfsr<<1u)|fb)&0x7fffffu;
  if(!*lfsr)*lfsr=0x7ffff8u;
 }
 int32_t a=beetle_sid_cached_wave(x->shape_a,phase,x->threshold_a,*lfsr);
 int32_t b=beetle_sid_cached_wave(x->shape_b,phase_b,x->threshold_b,*lfsr);
 /* On SID, oscillator B genuinely has its own phase and pitch. The optional
  * combined DAC is a masked nonlinear approximation for SAW/TRI/PULSE;
  * drive the existing (formerly unused SID) FM DEPTH knob as COMBINE.
  * Default 0 keeps legacy patches' linear oscillator mix. */
 int32_t base=(a*(100-(int32_t)x->mix)+b*(int32_t)x->mix)/100;
 if(x->ring && x->shape_a==1u){
  /* Ring modulation of a triangle by oscillator B's high phase bit. */
  if(phase_b&0x80000000u)a=4095-a;
  base=(a*(100-(int32_t)x->mix)+b*(int32_t)x->mix)/100;
 }
 if(x->combine && x->shape_a!=3u && x->shape_b!=3u){
  int32_t combined=a&b;
  /* Combined waves tend to lose amplitude in real SID DACs. Recenter
   * their output before interpolation to avoid a huge DC step. */
  combined=beetle_sid_clamp((combined-1024)*2+2048,4095);
  if(combined<0)combined=0;
  base=(base*(100-(int32_t)x->combine)+combined*(int32_t)x->combine)/100;
 }
 int32_t input=(base-2048)*6;
 if(!x->is8580)input=beetle_sid_6581_nonlinear(input);
 int32_t f=x->filter_coefficient,damping=x->damping;
 /* Exact max products: 28000*228 = 6,384,000 and 28000*105 =
  * 2,940,000, both far below INT32_MAX. No int64 multiply in the ISR. */
 int32_t hi=input-*lp-((*bp*damping)>>8u);
 hi=beetle_sid_clamp(hi,28000);
 int32_t next_bp=*bp+((hi*f)>>8u);
 next_bp=beetle_sid_clamp(next_bp,28000);
 int32_t next_lp=*lp+((next_bp*f)>>8u);
 next_lp=beetle_sid_clamp(next_lp,28000);
 *lp=next_lp;*bp=next_bp;
 int32_t output=x->mode<34u?next_lp:(x->mode<67u?next_bp:input-next_lp);
 if(!x->is8580)output=(beetle_sid_6581_nonlinear(output)*115)/100;
 return beetle_sid_clamp(output,16000);
}
static inline int32_t beetle_sid_sample(uint8_t chip,uint32_t phase,uint32_t step,uint32_t *lfsr,
 int32_t *lp,int32_t *bp,uint8_t wave_a,uint8_t wave_b,uint8_t width,
 uint8_t width_b,uint8_t mix,uint8_t filter_cut,uint8_t resonance,uint8_t filter_mode){
 struct beetle_sid_prepared x=beetle_sid_prepare(chip,wave_a,wave_b,width,width_b,
                                                mix,filter_cut,resonance,filter_mode,0u,0u,0u);
 return beetle_sid_sample_prepared(&x,phase,step,phase,lfsr,lp,bp);
}
#endif
