/* Heavy-load host-only endurance: not a physical DMA test. GPL-3.0-only. */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "track_chip.h"
#include "track_drum.h"
#include "track_master_fx.h"
#include "track_mixer.h"
#define FRAMES 128u
static struct track_chip_engine chip;
static struct track_drum_engine drum;
static struct track_master_fx_state master;
static struct track_master_settings settings;
static struct track_mixer mix;
static int16_t pcm[FRAMES*2];
int main(void){
 track_chip_init(&chip);track_chip_fx_reset();track_drum_init(&drum);
 track_mixer_init(&mix);track_master_default(&settings);track_master_dsp_init(&master);
 for(unsigned i=0;i<5;i++){chip.fx_type[i]=i==0?3:i==1?4:i==2?5:i==3?6:9;chip.fx_value[i]=60;}
 /* 2 minutes at 44.1 kHz; 41344 consecutive callbacks, full 8 synthvoices
  * plus up to 6 active drums, with periodic new note-on/stop transitions. */
 const unsigned blocks=(44100u*120u+FRAMES-1u)/FRAMES;
 uint32_t nonzero=0,max_synth=0,max_drum=0,hash=2166136261u;
 for(unsigned b=0;b<blocks;b++){
  if(b%172==0){
   for(unsigned ch=0;ch<5;ch++)track_chip_stop_channel(&chip,(uint8_t)ch);
   for(unsigned i=0;i<8;i++){
    const unsigned ty=(b/172u+i)%TRACK_CHIP_TYPES;
    chip.osc_type[i]=ty;
    chip.patches[i].wave_a=(uint8_t)((b/172+i)%TRACK_VA_SHAPES);
    chip.patches[i].wave_b=(uint8_t)((b/172+i+1)%TRACK_VA_SHAPES);
    chip.patches[i].fm_depth=(uint8_t)((b/172+i)%4==0?80:0);
    assert(track_chip_note(&chip,(uint8_t)(i%5),i,(uint8_t)(38+7*i),100));
   }
   for(unsigned i=0;i<TRACK_DRUM_POLY;i++)assert(track_drum_trigger(&drum,(uint8_t)((i+b/172)%TRACK_DRUM_COUNT)));
  }
  track_chip_render(&chip,&mix,pcm,FRAMES);
  track_drum_render(&drum,&mix,pcm,FRAMES);
  track_chip_process_punch(&chip,(b%1024u)<96u?1u<<4:0u,pcm,FRAMES);
  track_master_process(&master,&settings,pcm,FRAMES);
  assert(track_chip_health(&chip));
  for(unsigned i=0;i<FRAMES*2;i++){if(pcm[i])nonzero++;hash^=(uint16_t)pcm[i];hash*=16777619u;}
  unsigned a=0,d=0;for(unsigned i=0;i<chip.voice_count;i++)a+=!!chip.voices[i].active;
  for(unsigned i=0;i<TRACK_DRUM_POLY;i++)d+=!!drum.voices[i].active;
  if(a>max_synth)max_synth=a;if(d>max_drum)max_drum=d;
 }
 assert(nonzero && max_synth==8 && max_drum>=4);
 printf("PASS host 120sec/44100Hz / 12 types / 5 VA shapes / FX / max synth=%u max drums=%u / hash=%08x\n",max_synth,max_drum,hash);
}
