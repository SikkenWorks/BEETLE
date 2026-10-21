/* Replays the supplied NUGGIE V22 pattern 01, including configurable consecutive loops and
 * simultaneous chord/SID bass/drums. A HOST throughput and PCM parity
 * measurement, NOT proof of FM1 deadline behavior. GPL-3.0-only. */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include "track_project_store.h"
#include "track_core.h"
#include "track_chip.h"
#include "track_drum.h"
#include "track_mixer.h"
#include "track_master_fx.h"
static struct track_store_project project;
static struct track_chip_engine chip;
static struct track_drum_engine drums;
static struct track_master_fx_state master;
static uint8_t raw[TRACK_STORE_MAX];
static int16_t pcm[256];
static uint32_t digest=2166136261u;
static uint64_t now_ns(void){struct timespec ts;clock_gettime(CLOCK_MONOTONIC,&ts);return (uint64_t)ts.tv_sec*1000000000u+ts.tv_nsec;}
static void fold(unsigned frames){for(unsigned i=0;i<frames*2u;i++){digest^=(uint16_t)pcm[i];digest*=16777619u;}}
static void set_patch(unsigned i){const struct track_store_instrument*p=&project.instruments[i];chip.osc_type[i]=p->type;chip.env_attack[i]=p->attack;chip.env_decay[i]=p->decay;chip.env_sustain[i]=p->sustain;chip.env_release[i]=p->release;chip.patches[i]=p->va;}
static void step(unsigned t,uint8_t synth_type_override){const struct track_pattern*pat=&project.song.patterns[0];
 for(unsigned ch=0;ch<5;ch++){
  const struct track_step*st=&pat->ch[ch][t];
  if(st->note==TRACK_NOTE_EMPTY)continue;
  if(st->note==TRACK_NOTE_STOP){track_chip_release_channel(&chip,ch);continue;}
  track_chip_stop_channel(&chip,ch);
  uint8_t pitch[5];unsigned n=track_chord_keylocked(st->note,st->chord,&project.harmony,pitch);
  for(unsigned j=0;j<n;j++)if(chip.voice_count<9)track_chip_note(&chip,ch,st->instrument,pitch[j],94);
 }
 const struct track_step*d=&pat->ch[5][t];
 if(d->note<TRACK_DRUM_NOTE_COUNT){
  if(d->chord>0&&d->chord<15)track_drum_trigger_pair(&drums,d->note,(uint8_t)(d->chord-1u));
  else track_drum_trigger(&drums,d->note);
 }
 (void)synth_type_override;
}
int main(int argc,char**argv){if(argc<3||argc>4){fprintf(stderr,"usage: %s project.raw override_CH2_type (-1=as stored) [loops: 2..128]\n",argv[0]);return 2;}
 FILE*f=fopen(argv[1],"rb");if(!f)return 2;size_t n=fread(raw,1,sizeof raw,f);fclose(f);
 assert(track_store_decode(&project,raw,n)==0);int override=atoi(argv[2]);
 unsigned repetitions=argc==4?(unsigned)atoi(argv[3]):2u;
 if(repetitions<2u||repetitions>128u){fputs("loops must be between 2 and 128\n",stderr);return 2;}
 track_chip_init(&chip);track_drum_init(&drums);track_chip_fx_reset();track_master_dsp_init(&master);
 for(unsigned i=0;i<8;i++)set_patch(i);
 if(override>=0)chip.osc_type[2]=(uint8_t)override;
 memcpy(drums.components,project.drum,sizeof(drums.components));
 uint64_t peak=0,per_loop[2]={0};uint32_t blocks=0;unsigned maximum_voices=0,maximum_drums=0;
 for(unsigned loop=0;loop<repetitions;loop++){
  uint64_t start_loop=now_ns();
  for(unsigned row=0;row<16;row++){
   step(row,(uint8_t)override);
   unsigned frames=5512;
   while(frames){unsigned count=frames>128?128:frames;
    uint64_t begin=now_ns();
    track_chip_render(&chip,&project.mixer,pcm,count);
    track_drum_render(&drums,&project.mixer,pcm,count);
    track_master_process(&master,&project.master,pcm,count);
    uint64_t delta=now_ns()-begin;if(delta>peak)peak=delta;
    frames-=count;blocks++;fold(count);
    unsigned av=0,dv=0;for(unsigned i=0;i<chip.voice_count;i++)if(chip.voices[i].active)av++;
    for(unsigned i=0;i<TRACK_DRUM_POLY;i++)if(drums.voices[i].active)dv++;
    if(av>maximum_voices)maximum_voices=av;if(dv>maximum_drums)maximum_drums=dv;
   }
  }
  if(loop<2u)per_loop[loop]=now_ns()-start_loop;
 }
 printf("ch2=%2d  loops=%u loop1_ms=%8.3f loop2_ms=%8.3f maxblock_us=%7.2f maxsynth=%u maxdrums=%u FNV=%08x\n",override,repetitions,(double)per_loop[0]/1e6,(double)per_loop[1]/1e6,(double)peak/1000,maximum_voices,maximum_drums,digest);
 return 0;
}
