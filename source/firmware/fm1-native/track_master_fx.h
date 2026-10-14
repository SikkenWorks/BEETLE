/* BEETLE V0.38 · Fixed-point master bus (EQ -> dynamics -> reverb by default).
 * No allocation, libm, flash I/O or blocking in the audio interrupt.
 * The FM1 hardware MASTER attenuator follows this bus in audio_target.c.
 * SPDX-License-Identifier: GPL-3.0-only */
#ifndef BEETLE_TRACK_MASTER_FX_H
#define BEETLE_TRACK_MASTER_FX_H
#include <stdint.h>
#include "track_project_store.h"
#define TRACK_MASTER_REVERB_FRAMES 1024u
struct track_master_fx_state {
 int32_t crossover[2][4];
 uint32_t envelope;
 int16_t delay_l[TRACK_MASTER_REVERB_FRAMES],delay_r[TRACK_MASTER_REVERB_FRAMES];
 int32_t reverb_lp_l,reverb_lp_r;
 uint16_t delay_pos;
 volatile uint16_t scope_level;
 uint32_t guard;
 uint16_t recover_frames;
 volatile uint16_t recover_count;
};
static inline int32_t master_limit(int32_t v){return v>32767?32767:v< -32768?-32768:v;}
static inline void track_master_default(struct track_master_settings *c){
 for(uint8_t j=0;j<4;j++){c->eq[j]=50u;c->eq_freq[j]=50u;c->eq_width[j]=50u;}
 c->comp_threshold=75u;c->comp_ratio=40u;c->comp_gain=50u;c->comp_attack=28u;
 c->reverb_wet=15u;c->reverb_feedback=32u;c->reverb_tone=50u;c->reverb_spread=50u;
 for(uint8_t j=0;j<3;j++){c->enabled[j]=1u;c->order[j]=j;}
}
#define TRACK_MASTER_GUARD 0xBEE7052Au
static inline void track_master_dsp_init(struct track_master_fx_state *s){
 if(!s)return;
 s->envelope=0u;s->delay_pos=0u;s->reverb_lp_l=s->reverb_lp_r=0;
 s->scope_level=0u;s->guard=TRACK_MASTER_GUARD;
 s->recover_frames=TRACK_MASTER_REVERB_FRAMES;
}
static inline void track_master_process(struct track_master_fx_state *s,
 const struct track_master_settings *c,int16_t *dst,uint32_t frames){
 if(!s||!c||!dst)return;
 /* A corrupt delay pointer or dynamics envelope must never index arbitrary
  * RAM. Degrade to dry processing and refill the ring on the audio thread.
  * The main firmware watchdog still handles true CPU/IRQ deadline faults. */
 /* A cold, zeroed DSP object is safe, including older host test fixtures. */
 if(s->guard==0u && s->delay_pos==0u && s->envelope==0u &&
    s->recover_frames==0u)s->guard=TRACK_MASTER_GUARD;
 if(s->guard!=TRACK_MASTER_GUARD || s->delay_pos>=TRACK_MASTER_REVERB_FRAMES ||
    s->envelope>65536u || s->recover_frames>TRACK_MASTER_REVERB_FRAMES){
  if(s->recover_count<65535u)s->recover_count++;
  track_master_dsp_init(s);
 }
 static const uint8_t freq_base[4]={3u,18u,64u,175u};
 uint16_t coeff[4];int32_t egain[4];uint8_t eq_flat=1u;
 for(uint8_t band=0;band<4u;band++){
  uint32_t freq=(uint32_t)freq_base[band]*(50u+(uint32_t)c->eq_freq[band])/100u;
  if(freq<1u)freq=1u;if(freq>245u)freq=245u;
  coeff[band]=(uint16_t)freq;
  int32_t g=100+((int32_t)c->eq[band]-50)*(25+(int32_t)c->eq_width[band])/50;
  egain[band]=g<25?25:g;
  if(g!=100)eq_flat=0u;
 }
 const uint32_t comp_thr=4500u+(uint32_t)c->comp_threshold*235u;
 const uint32_t attack_div=4u+(uint32_t)c->comp_attack/4u;
 const uint32_t release_div=160u+(uint32_t)c->comp_attack*8u;
 const uint16_t tap_spread=(uint16_t)(16u+(uint32_t)c->reverb_spread*64u/100u);
 const uint16_t fb=(uint16_t)((uint32_t)c->reverb_feedback*60u/100u);
 const uint16_t tone=(uint16_t)(12u+(uint32_t)c->reverb_tone*2u);
 const int32_t wet=(int32_t)c->reverb_wet*45/100;
 for(uint32_t f=0;f<frames;f++){
  if(s->recover_frames){
   /* 1024 bounded frames clear both reverb paths without a large ISR memset. */
   uint16_t pos=s->delay_pos;
   s->delay_l[pos]=s->delay_r[pos]=0;
   s->delay_pos=(uint16_t)((pos+1u)&(TRACK_MASTER_REVERB_FRAMES-1u));
   s->recover_frames--;s->envelope=0u;
   continue;
  }
  int32_t l=dst[2u*f],r=dst[2u*f+1u];
  for(uint8_t fx=0u;fx<3u;fx++){
   uint8_t which=c->order[fx];if(which>2u||!c->enabled[which])continue;
   if(which==0u){
    if(eq_flat)continue;
    for(uint8_t channel=0u;channel<2u;channel++){
     int32_t input=channel?r:l,previous=0,output=0;
     for(uint8_t band=0u;band<4u;band++){
      int32_t *state=&s->crossover[channel][band];
      *state+=(int32_t)(((int64_t)(input-*state)*coeff[band])>>8u);
      int32_t current=*state-previous;previous=*state;
      output+=(int32_t)(((int64_t)current*egain[band])/100);
     }
     /* Residual high frequencies stay uncolored by the last shelf. */
     output+=input-previous;
     if(channel)r=master_limit(output);else l=master_limit(output);
    }
   }else if(which==1u){
    uint32_t a=(uint32_t)(l<0?-l:l),b=(uint32_t)(r<0?-r:r),peak=a>b?a:b;
    if(peak>s->envelope)s->envelope+=(peak-s->envelope)/attack_div+1u;
    else s->envelope-=(s->envelope+release_div-1u)/release_div;
    uint32_t threshold=comp_thr;
    int32_t gain=256;
    if(s->envelope>threshold){
     uint32_t over=s->envelope-threshold;
     uint32_t reduction=over*((uint32_t)c->comp_ratio+20u)/140u;
     uint32_t denom=s->envelope+reduction;
     gain=denom?(int32_t)(((uint32_t)s->envelope*256u)/denom):256;
    }
    gain=(gain*(75+(int32_t)c->comp_gain/2))/100;
    l=master_limit(l*gain/256);r=master_limit(r*gain/256);
   }else{
    const uint16_t pos=s->delay_pos;
    /* Two quiet, slightly detuned feedback paths, clipped before storage.
     * Keep wet <= 45% so the dry mix stays clear. */
    const uint16_t tap1=(uint16_t)((pos+TRACK_MASTER_REVERB_FRAMES-701u)&(TRACK_MASTER_REVERB_FRAMES-1u));
    const uint16_t tap2=(uint16_t)((pos+TRACK_MASTER_REVERB_FRAMES-(uint16_t)(792u+tap_spread))&(TRACK_MASTER_REVERB_FRAMES-1u));
    int32_t dl=s->delay_l[tap1],dr=s->delay_r[tap2];
    s->reverb_lp_l+=(int32_t)(((int64_t)(dl-s->reverb_lp_l)*tone)>>8);
    s->reverb_lp_r+=(int32_t)(((int64_t)(dr-s->reverb_lp_r)*tone)>>8);
    s->delay_l[pos]=(int16_t)master_limit((l/2)+(s->reverb_lp_r*(int32_t)fb)/100);
    s->delay_r[pos]=(int16_t)master_limit((r/2)+(s->reverb_lp_l*(int32_t)fb)/100);
    s->delay_pos=(uint16_t)((pos+1u)&(TRACK_MASTER_REVERB_FRAMES-1u));
    l=master_limit((l*(100-wet)+dl*wet)/100);
    r=master_limit((r*(100-wet)+dr*wet)/100);
   }
  }
  dst[2u*f]=(int16_t)l;dst[2u*f+1u]=(int16_t)r;
  uint32_t peak=(uint32_t)((l<0?-l:l)>(r<0?-r:r)?(l<0?-l:l):(r<0?-r:r));
  if(peak>s->scope_level)s->scope_level=(uint16_t)peak;
  else if(s->scope_level)s->scope_level--;
 }
}
#endif
