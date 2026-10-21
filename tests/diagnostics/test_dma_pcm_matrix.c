/* GPL-3.0-only: cross-variant sample-perfect regression over every chip type,
 * dual VA waveform, onsets, active tails and drum polyphony. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "track_chip.h"
#include "track_drum.h"
#include "track_mixer.h"
#define N 128u
static struct track_chip_engine chip;
static struct track_drum_engine drum;
static struct track_mixer mix;
static int16_t pcm[N*2];
static uint32_t digest=2166136261u;
static void fold(void){for(unsigned i=0;i<N*2;i++){digest^=(uint16_t)pcm[i];digest*=16777619u;}}
static void render(unsigned blocks){for(unsigned x=0;x<blocks;x++){track_chip_render(&chip,&mix,pcm,N);track_drum_render(&drum,&mix,pcm,N);fold();assert(track_chip_health(&chip));}}
int main(void){track_mixer_init(&mix);
 for(unsigned type=0;type<TRACK_CHIP_TYPES;type++)for(unsigned shape=0;shape<TRACK_VA_SHAPES;shape++){
  track_chip_init(&chip);track_chip_fx_reset();track_drum_init(&drum);
  chip.osc_type[0]=type;chip.patches[0].wave_a=shape;chip.patches[0].wave_b=(shape+1)%TRACK_VA_SHAPES;
  chip.patches[0].pulse_width=(uint8_t)(5+shape*18);chip.patches[0].pulse_width_b=(uint8_t)(95-shape*17);
  chip.patches[0].drive=(uint8_t)((type*9)%100);chip.patches[0].morph=(uint8_t)((type*11)%100);
  chip.patches[0].mix=(uint8_t)((type*7)%100);chip.patches[0].fm_depth=(uint8_t)((type*13)%100);
  chip.cutoff[0]=(uint8_t)(10+type*18);chip.fx_type[0]=(uint8_t)(shape%2?6u:0u);chip.fx_value[0]=38u;
  for(unsigned v=0;v<8;v++)assert(track_chip_note(&chip,(uint8_t)(v%5u),0,(uint8_t)(38+v*6),100));
  for(unsigned d=0;d<6;d++)assert(track_drum_trigger(&drum,(uint8_t)(d*2u)));
  render(8);
  for(unsigned v=0;v<5;v++)track_chip_stop_channel(&chip,(uint8_t)v);
  for(unsigned v=0;v<8;v++)assert(track_chip_note(&chip,(uint8_t)(v%5u),0,(uint8_t)(44+v*5),100));
  render(8);
 }
 printf("PCM_MATRIX_FNV %08x\n",digest);
 return 0;
}
