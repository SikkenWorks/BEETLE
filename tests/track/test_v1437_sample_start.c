/* BEETLE V1.4.37: global usersample attack must not be consumed during
 * asynchronous flash-prefetch, including NOTE3 on the first pattern step.
 * GPL-3.0-only. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "track_drum.h"
#include "track_mixer.h"
#include "beetle_bd4_live.h"

int16_t *beetle_stem_capture;
uint32_t beetle_stem_offset;
static int ready;
void beetle_bd4_live_stop(uint8_t voice){(void)voice;}
void beetle_bd4_live_request(uint8_t voice,uint8_t slot,uint32_t serial){(void)voice;(void)slot;(void)serial;}
const char *beetle_bd4_live_label(uint8_t slot){(void)slot;return 0;}
int beetle_bd4_live_available(uint8_t slot){(void)slot;return 1;}
void beetle_bd4_live_choke_all(void){}
int beetle_bd4_live_next(uint8_t voice,uint32_t serial,int16_t *pcm){
 (void)voice;(void)serial;
 if(!ready){*pcm=0;return 0;}
 *pcm=9000;
 return 1;
}
uint8_t beetle_bd4_live_start_pending(void){return !ready;}
int main(void){
 struct track_drum_engine d;struct track_mixer mix;int16_t pcm[128*2];
 track_drum_init(&d);track_mixer_init(&mix);
 /* CH6 NOTE3 user slot U01. */
 assert(track_drum_trigger(&d,TRACK_DRUM_COUNT)==1);
 struct track_drum_voice *v=0;
 for(unsigned i=0;i<TRACK_DRUM_POLY;i++)if(d.voices[i].active){v=&d.voices[i];break;}
 assert(v && v->id>=32u);
 uint32_t original_phase=v->phase,original_age=v->age,original_env=v->env_q24;
 memset(pcm,0,sizeof pcm);
 track_drum_render(&d,&mix,pcm,128);
 /* Without this fix: phase would be 128*32768 and age 128 BEFORE audio. */
 assert(v->active && v->phase==original_phase && v->age==original_age && v->env_q24==original_env);
 for(unsigned i=0;i<256;i++)assert(((int16_t *)pcm)[i]==0);
 assert(beetle_bd4_live_start_pending());
 ready=1;
 assert(!beetle_bd4_live_start_pending());
 memset(pcm,0,sizeof pcm);
 track_drum_render(&d,&mix,pcm,128);
 assert(v->active && v->age==128 && v->phase>0 && v->env_q24<original_env);
 int has_audio=0;for(unsigned i=0;i<256;i++)if(pcm[i])has_audio=1;
 assert(has_audio);
 puts("PASS V1.4.37 NOTE3 first-step sample retains PCM attack and envelope through delayed main-loop prefetch");
 return 0;
}
