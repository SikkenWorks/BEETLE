/* BEETLE V0.72: host-only RC audio hardening and bounded groove load.
 * Exercises real native DSP code, NOT physical JieLi timing or analogue output.
 * SPDX-License-Identifier: GPL-3.0-only */
#include "track_chip.h"
#include "track_drum.h"
#include "track_mixer.h"
#include "track_master_fx.h"
#include "beetle_step_groove.h"
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static struct track_chip_engine chip;
static struct track_drum_engine drum;
static struct track_master_fx_state master,corrupt,clean;
static struct track_mixer mix;
static struct track_master_settings settings,raw,clamped;
static int16_t stereo[2u*128u],other[2u*128u];
static struct track_pattern pattern;
static struct track_chip_engine chip_b;
static struct track_drum_engine drum_b;
static struct track_master_fx_state master_b;
static int16_t rendered_a[256],rendered_b[256];
static uint64_t energy;
static uint32_t clips;
static void test_master_corruption(void){
 track_master_default(&settings);track_master_dsp_init(&master);
 memset(stereo,0,sizeof(stereo));
 /* A transient RAM corruption must recover in bounded time without risking
  * 32-bit signed overflow in the low-pass/filter arithmetic. */
 master.reverb_lp_l=INT32_MAX;
 master.crossover[0][3]=INT32_MIN;
 track_master_process(&master,&settings,stereo,128u);
 assert(master.recover_count==1u && master.guard==TRACK_MASTER_GUARD);
 assert(master.crossover[0][3]==0 && master.reverb_lp_l==0);
 for(unsigned k=0;k<8u;k++)track_master_process(&master,&settings,stereo,128u);
 assert(master.recover_frames==0u);
 assert(master.recover_count==1u);
 /* A parameter corrupted to 255 must sound identical to the maximum
  * allowed 100, not turn a reverb mixer negative or its filter unstable. */
 track_master_default(&raw);raw.eq[0]=255u;raw.eq_freq[1]=255u;
 raw.eq_width[2]=255u;raw.comp_threshold=255u;raw.comp_ratio=255u;
 raw.comp_gain=255u;raw.comp_attack=255u;raw.reverb_wet=255u;
 raw.reverb_feedback=255u;raw.reverb_tone=255u;raw.reverb_spread=255u;
 clamped=raw;
 for(unsigned i=0;i<4u;i++){
  clamped.eq[i]=track_master_safe_percent(clamped.eq[i]);
  clamped.eq_freq[i]=track_master_safe_percent(clamped.eq_freq[i]);
  clamped.eq_width[i]=track_master_safe_percent(clamped.eq_width[i]);
 }
 clamped.comp_threshold=clamped.comp_ratio=clamped.comp_gain=clamped.comp_attack=100u;
 clamped.reverb_wet=clamped.reverb_feedback=clamped.reverb_tone=clamped.reverb_spread=100u;
 memset(&corrupt,0,sizeof(corrupt));memset(&clean,0,sizeof(clean));
 track_master_dsp_init(&corrupt);track_master_dsp_init(&clean);
 for(unsigned round=0;round<60;round++){
  for(unsigned i=0;i<256;i++){
   stereo[i]=(int16_t)((int32_t)((i*1307u+round*271u)%40000u)-20000);
   other[i]=stereo[i];
  }
  track_master_process(&corrupt,&raw,stereo,128u);
  track_master_process(&clean,&clamped,other,128u);
  assert(memcmp(stereo,other,sizeof(stereo))==0);
  assert(track_master_state_intact(&corrupt));
 }
}
static void test_groove(void){
 memset(&pattern,0,sizeof(pattern));
 for(unsigned c=0;c<TRACK_CHANNELS;c++)for(unsigned i=0;i<TRACK_STEPS;i++){
  pattern.ch[c][i].note=60u;track_groove_set_micro(&pattern.ch[c][i],7);
  track_groove_set_ratchet(&pattern.ch[c][i],4);
 }
 struct beetle_groove_clock clock;beetle_groove_reset(&clock);
 for(uint32_t bpm=30u;bpm<=300u;bpm++){
  const uint32_t interval=661500u/bpm;
  uint32_t swing[TRACK_CHANNELS]={0};
  for(unsigned c=0;c<TRACK_CHANNELS;c++)swing[c]=(c&1u)?interval*3u/4u:0u;
  beetle_groove_schedule(&clock,&pattern,(uint8_t)(bpm%16u),interval,swing);
  uint32_t at=0u;uint32_t emitted[TRACK_CHANNELS]={0};
  uint32_t loops=0u;
  while(at<interval){
   assert(++loops<70u); /* Never stall the real-time ISR. */
   uint8_t due=beetle_groove_due(&clock,at);
   for(unsigned c=0;c<TRACK_CHANNELS;c++)if(due&(1u<<c))emitted[c]++;
   uint32_t wait=beetle_groove_wait(&clock,at,interval-at);
   assert(wait>0u && wait<=interval-at);
   at+=wait;
  }
  for(unsigned c=0;c<TRACK_CHANNELS;c++)assert(emitted[c]==4u);
 }
}
/* Audio DMA may call the renderer with shortened chunks at step/gate
 * boundaries. The engine must not depend on the number of calls when the
 * total number of rendered samples stays unchanged. */
static void test_chunk_parity(void){
 track_chip_init(&chip);track_drum_init(&drum);track_mixer_init(&mix);
 track_master_default(&settings);track_master_dsp_init(&master);
 settings.eq[1]=65u;settings.reverb_wet=35u;
 assert(track_chip_note(&chip,0u,0u,60u,110u));
 assert(track_chip_note(&chip,1u,1u,52u,95u));
 assert(track_drum_trigger(&drum,2u));
 chip_b=chip;drum_b=drum;master_b=master;
 for(unsigned round=0;round<30u;round++){
  memset(rendered_a,0,sizeof rendered_a);
  memset(rendered_b,0,sizeof rendered_b);
  track_chip_render(&chip,&mix,rendered_a,128u);
  track_drum_render(&drum,&mix,rendered_a,128u);
  track_chip_process_punch(&chip,1u<<4u,rendered_a,128u);
  track_master_process(&master,&settings,rendered_a,128u);
  for(unsigned at=0;at<128;at+=32){
   track_chip_render(&chip_b,&mix,rendered_b+2u*at,32u);
   track_drum_render(&drum_b,&mix,rendered_b+2u*at,32u);
   track_chip_process_punch(&chip_b,1u<<4u,rendered_b+2u*at,32u);
   track_master_process(&master_b,&settings,rendered_b+2u*at,32u);
  }
  assert(memcmp(rendered_a,rendered_b,sizeof rendered_a)==0);
 }
}
static void test_heavy_mix(void){
 track_chip_init(&chip);track_drum_init(&drum);track_mixer_init(&mix);
 track_master_default(&settings);track_master_dsp_init(&master);
 settings.comp_threshold=38u;settings.comp_ratio=72u;settings.comp_attack=25u;
 settings.reverb_wet=42u;settings.reverb_feedback=55u;
 settings.eq[0]=65u;settings.eq[3]=60u;
 for(unsigned inst=0;inst<4;inst++){
  chip.osc_type[inst]=(uint8_t)(TRACK_CHIP_SID_6581+inst);
  chip.env_attack[inst]=1u;chip.env_sustain[inst]=85u;
  chip.patches[inst].drive=12u;
 }
 for(unsigned i=0;i<8;i++)assert(track_chip_note(&chip,(uint8_t)(i%5),(uint8_t)(i%4),(uint8_t)(40+i*5),110u));
 assert(chip.voice_count==TRACK_CHIP_VOICES);
 for(unsigned j=0;j<14;j++)(void)track_drum_trigger(&drum,(uint8_t)j);
 for(unsigned round=0;round<500;round++){
  if(round%11u==0u){
   (void)track_drum_trigger_pair_tuned(&drum,(uint8_t)((round/11u)%14u),2u,(int8_t)((round%23u)-11));
   if(round%33u==0u)(void)track_chip_note(&chip,(uint8_t)((round/33u)%5u),0u,60u,100u);
  }
  memset(stereo,0,sizeof(stereo));
  track_chip_render(&chip,&mix,stereo,128u);
  track_drum_render(&drum,&mix,stereo,128u);
  track_chip_process_punch(&chip,(round/25u)&1u?1u<<4u:0u,stereo,128u);
  track_master_process(&master,&settings,stereo,128u);
  assert(track_chip_health(&chip));
  assert(track_master_state_intact(&master));
  for(unsigned k=0;k<256;k++){
   int32_t sample=stereo[k];energy+=(uint64_t)((int64_t)sample*sample);
   if(sample>=32767||sample<=-32768)clips++;
  }
 }
 assert(energy>100000000u);
 /* This is a software clipping indicator, NOT an analogue FM1 guarantee. */
 assert(clips<30000u);
}
int main(void){
 test_master_corruption();test_groove();test_chunk_parity();test_heavy_mix();
 printf("PASS V0.72: DSP state recovery, malformed parameters, 271 tempo schedules, chunk parity, 64000 native stereo frames; energy=%llu clipped=%u\n",
  (unsigned long long)energy,clips);
 return 0;
}
