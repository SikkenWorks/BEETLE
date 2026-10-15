/* BEETLE V0.29: step entry, safe wrap, audio DSP and drum PCM smoke. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "track_core.h"
#include "track_chip.h"
#include "track_drum.h"
#include "track_mixer.h"
static struct track_engine e;
static struct track_chip_engine chip;
static struct track_drum_engine drums;
static struct track_mixer mix;
static int16_t pcm[2048*2];
static uint64_t energy(void){uint64_t v=0;for(unsigned i=0;i<4096u;i++){int32_t x=pcm[i];v+=(uint32_t)(x<0?-x:x);}return v;}
int main(void){
 track_init(&e);track_mixer_init(&mix);track_chip_init(&chip);track_drum_init(&drums);
 assert(e.transport.bpm==120u && e.active.length==16u);
 assert(track_enter_note(&e,60u)==0);assert(e.active.ch[0][0].note==60u && e.transport.edit_step==1u);
 assert(track_enter_note(&e,64u)==0);assert(e.active.ch[0][1].note==64u && e.transport.edit_step==2u);
 e.transport.edit_step=15u;assert(track_enter_note(&e,67u)==0);
 assert(e.active.ch[0][15].note==67u && e.transport.edit_step==0u);
 e.transport.edit_column=1u;assert(track_enter_note(&e,70u)!=0 && e.transport.edit_step==0u);
 e.transport.edit_column=0u;e.transport.edit_channel=3u;
 assert(track_enter_note(&e,0u)==0 && e.active.ch[3][0].note==0u && e.transport.edit_step==1u);
 assert(track_enter_note(&e,14u)!=0 && e.transport.edit_step==1u);
 e.transport.edit_channel=0u;e.transport.edit_step=16u;
 assert(track_enter_note(&e,60u)!=0);
 assert(track_chip_note(&chip,0u,0u,60u,100u));track_chip_render(&chip,&mix,pcm,2048u);
 assert(energy()>100000u);track_chip_note_off(&chip,0u,60u);
 memset(pcm,0,sizeof(pcm));assert(track_drum_trigger(&drums,0u));
 track_drum_render(&drums,&mix,pcm,2048u);assert(energy()>0u);
 puts("BEETLE 0.29: note entry, row advance/wrap, synth PCM, drum PCM PASS");return 0;
}
