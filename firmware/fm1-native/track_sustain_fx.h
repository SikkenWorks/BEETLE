/* BEETLE V0.46: persistent per-lane step FX, GPL-3.0-only. */
#ifndef BEETLE_TRACK_SUSTAIN_FX_H
#define BEETLE_TRACK_SUSTAIN_FX_H
#include "track_chip.h"
#include "track_core.h"
/* Called once for the lane when the step is actually due (after swing).
 * Empty step: preserve all DSP state. STP: release and clear. New note:
 * return to base patch unless that step has an FX. Effect-only step:
 * replace the currently sustained effect WITHOUT re-triggering the note. */
static inline void beetle_step_fx_event(struct track_chip_engine *chip,
 uint8_t lane,const struct track_step *step,uint8_t base_duty,
 uint8_t base_cutoff,uint16_t bpm){
 if(!chip||!step||lane>=TRACK_SYNTH_CHANNELS)return;
 const uint8_t stopped=(uint8_t)(step->note==TRACK_NOTE_STOP);
 const uint8_t new_note=(uint8_t)(step->note<=127u);
 if(!stopped&&!new_note&&!step->effect)return;
 if(stopped||new_note||step->effect){
  chip->fx_type[lane]=0u;chip->fx_value[lane]=50u;
  for(uint8_t j=0u;j<2u;j++){chip->fx_extra_type[lane][j]=0u;chip->fx_extra_value[lane][j]=50u;}
  chip->duty[lane]=base_duty;chip->cutoff[lane]=base_cutoff;
  chip->drive[lane]=0u;chip->cut_after[lane]=0xFFFFFFFFu;
  chip->step_gain[lane]=100u;chip->fx_samples[lane]=0u;
 }
 if(!stopped&&step->effect){
  chip->fx_type[lane]=step->effect;
  chip->fx_value[lane]=step->value;
  if(step->effect==2u)chip->drive[lane]=step->value;
  if(step->effect==8u){
   const uint32_t dur=661500u/(bpm?bpm:120u);
   chip->cut_after[lane]=(uint32_t)(((uint64_t)dur*step->value)/100u);
  }
  if(step->effect==1u)chip->cutoff[lane]=(uint8_t)(4u+(uint32_t)step->value*2u);
  if(step->effect==TRACK_FX_VOLUME)chip->step_gain[lane]=step->value;
 }
 /* An effect-only pitch row retunes the live oscillator(s) in place. */
 if(stopped||step->effect==11u||new_note)track_chip_repitch_channel(chip,lane);
}
/* Apply additional per-step FX in deterministic slot order after FX1.
 * Each extra runs through the same DSP, while static lane parameters (filter,
 * drive, gate, volume, pitch) are composed without replacing the first FX. */
static inline void beetle_step_fx_multi(struct track_chip_engine *chip,
 uint8_t lane,const struct track_extra_fx *a,const struct track_extra_fx *b,uint16_t bpm){
 if(!chip||lane>=TRACK_SYNTH_CHANNELS)return;
 const struct track_extra_fx *entries[2]={a,b};
 uint8_t pitch=0u;
 for(uint8_t j=0u;j<2u;j++){
  const struct track_extra_fx *f=entries[j];if(!f||!f->effect)continue;
  const uint8_t type=track_extra_effect(f),value=f->value;
  chip->fx_extra_type[lane][j]=type;chip->fx_extra_value[lane][j]=value;
  if(type==1u)chip->cutoff[lane]=(uint8_t)(4u+(uint32_t)value*2u);
  else if(type==2u && value>chip->drive[lane])chip->drive[lane]=value;
  else if(type==8u){
   const uint32_t dur=661500u/(bpm?bpm:120u);
   uint32_t end=(uint32_t)(((uint64_t)dur*value)/100u);
   if(end<chip->cut_after[lane])chip->cut_after[lane]=end;
  }else if(type==TRACK_FX_VOLUME){
   chip->step_gain[lane]=(uint8_t)((uint16_t)chip->step_gain[lane]*value/100u);
  }else if(type==11u)pitch=1u;
 }
 if(pitch)track_chip_repitch_channel(chip,lane);
}
#endif
