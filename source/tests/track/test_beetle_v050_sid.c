/* SPDX-License-Identifier: GPL-3.0-only -- RESID-LITE DSP & persistence regression */
#include "track_chip.h"
#include "track_song.h"
#include "track_project_store.h"
#include "track_mixer.h"
#include "track_drum.h"
#include "track_master_fx.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static struct track_chip_engine synth;
static struct track_store_project saved,loaded;
static int16_t out[2048];
static uint8_t bytes[TRACK_STORE_MAX];
static uint64_t energy(const int16_t *v,unsigned n){uint64_t x=0;for(unsigned k=0;k<n;k++)x+=(uint64_t)((int32_t)v[k]*(int32_t)v[k]);return x;}
int main(void){
 struct track_mixer mix;track_mixer_init(&mix);
 track_chip_init(&synth);
 synth.osc_type[0]=TRACK_CHIP_RESID_LITE;
 synth.env_attack[0]=1;
 struct track_va_patch *p=&synth.patches[0];
 p->wave_a=0u;p->wave_b=3u;p->mix=0u;p->drive=30u;p->morph=10u;
 assert(track_chip_note(&synth,0u,0u,60u,112u));
 memset(out,0,sizeof(out));track_chip_render(&synth,&mix,out,1024u);
 uint64_t saw=energy(out,2048u);
 fprintf(stderr,"saw energy=%llu health=%d max=%d voice=%u stage=%u env=%u step=%u last=%d cut=%u sf=%u\n",(unsigned long long)saw,track_chip_health(&synth),out[100],synth.voices[0].active,synth.voices[0].env_stage,synth.voices[0].envelope,synth.voices[0].step,synth.voices[0].last_level,synth.cut_after[0],synth.fx_samples[0]); assert(saw>1000000ULL && track_chip_health(&synth));
 track_chip_silence(&synth);synth.osc_type[0]=TRACK_CHIP_RESID_LITE;
 p->wave_a=3u;p->mix=0u;
 assert(track_chip_note(&synth,0u,0u,60u,112u));
 memset(out,0,sizeof(out));track_chip_render(&synth,&mix,out,1024u);
 uint64_t noise=energy(out,2048u);
 assert(noise>1000000ULL && saw!=noise);
 for(unsigned t=0;t<6;t++){
  track_chip_note_off(&synth,0u,60u);
  memset(out,0,sizeof(out));track_chip_render(&synth,&mix,out,1024u);
  assert(track_chip_health(&synth));
 }
 struct track_engine core;track_init(&core);track_song_init(&saved.song,&core);
 track_mixer_init(&saved.mixer);track_master_default(&saved.master);
 saved.bpm=120u;saved.slot=0u;
 for(unsigned i=0;i<4u;i++){track_va_patch_default(&saved.instruments[i].va);saved.instruments[i].volume=75u;}
 for(unsigned i=0;i<TRACK_DRUM_COUNT;i++)track_drum_component_default(&saved.drum[i],i);
 saved.instruments[0].type=TRACK_CHIP_RESID_LITE;
 saved.instruments[0].va.wave_a=3u;saved.instruments[0].va.wave_b=2u;
 saved.instruments[0].va.drive=62u;saved.instruments[0].va.morph=49u;
 uint32_t used=0;
 assert(track_store_encode(&saved,bytes,sizeof bytes,&used)==0);
 assert(track_store_decode(&loaded,bytes,used)==0);
 assert(loaded.instruments[0].type==TRACK_CHIP_RESID_LITE);
 assert(loaded.instruments[0].va.wave_a==3u && loaded.instruments[0].va.drive==62u);
 puts("PASS V0.50 SID voice: finite, playable saw+noise, ADSR release, project round-trip");
 return 0;
}
