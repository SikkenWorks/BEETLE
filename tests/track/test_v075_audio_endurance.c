/* BEETLE V0.75: 60 seconds of deterministic host DSP under heavy workload.
 * This is NOT real-time JieLi cycle timing or an FM1 output quality test.
 * GPL-3.0-only. */
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "track_chip.h"
#include "track_drum.h"
#include "track_mixer.h"
#include "track_master_fx.h"
static struct track_chip_engine chip;
static struct track_drum_engine drums;
static struct track_mixer mixer;
static struct track_master_fx_state fx;
static struct track_master_settings params;
static int16_t pcm[2*128];
int main(void){
 const uint32_t total=44100u*60u;
 track_chip_init(&chip);track_drum_init(&drums);track_mixer_init(&mixer);
 track_master_dsp_init(&fx);track_master_default(&params);
 params.comp_threshold=40u;params.comp_ratio=65u;
 params.reverb_wet=42u;params.reverb_feedback=50u;
 params.eq[0]=65u;params.eq[3]=65u;
 for(uint8_t inst=0;inst<4u;inst++){
  chip.osc_type[inst]=(uint8_t)(TRACK_CHIP_SID_6581+inst);
  chip.env_attack[inst]=1u;chip.env_sustain[inst]=75u;
 }
 for(uint8_t v=0;v<8;v++)assert(track_chip_note(&chip,(uint8_t)(v%5u),(uint8_t)(v%4u),(uint8_t)(40u+v*4u),110u));
 uint64_t energy=0;uint32_t saturated=0,nonzero=0, blocks=0;
 for(uint32_t produced=0;produced<total;){
  const uint32_t n=total-produced<128u?total-produced:128u;
  if(blocks%19u==0u)(void)track_drum_trigger_pair_tuned(&drums,(uint8_t)((blocks/19u)%TRACK_DRUM_COUNT),1u,(int8_t)((blocks%13u)-6));
  if(blocks%121u==0u)(void)track_chip_note(&chip,(uint8_t)((blocks/121u)%TRACK_SYNTH_CHANNELS),(uint8_t)((blocks/121u)%4u),(uint8_t)(42u+(blocks/121u)%35u),110u);
  if(blocks%487u==0u)track_chip_release_channel(&chip,(uint8_t)((blocks/487u)%TRACK_SYNTH_CHANNELS));
  memset(pcm,0,sizeof pcm);
  track_chip_render(&chip,&mixer,pcm,n);
  track_drum_render(&drums,&mixer,pcm,n);
  const uint8_t punch=(uint8_t)((blocks/207u)%4u==0u?1u<<4u:0u);
  track_chip_process_punch(&chip,punch,pcm,n);
  track_master_process(&fx,&params,pcm,n);
  assert(track_chip_health(&chip));assert(track_master_state_intact(&fx));
  for(uint32_t k=0;k<2*n;k++){
   int32_t s=pcm[k];energy+=(uint64_t)((int64_t)s*s);
   if(s)nonzero++;
   if(s==32767||s==-32768)saturated++;
  }
  produced+=n;blocks++;
 }
 assert(energy>100000000u);assert(nonzero>100000u);
 printf("PASS V0.75 DSP endurance: %u frames (60 seconds at 44.1 kHz), %u blocks, nonzero=%u, peak-saturated=%u, energy=%llu (HOST ONLY)\n",total,blocks,nonzero,saturated,(unsigned long long)energy);
 return 0;
}
