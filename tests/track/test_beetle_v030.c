/* BEETLE V0.30: STOP, VOLUME 0..100, indefinite sustain, 16bit PCM & MASTER. */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "track_core.h"
#include "track_chip.h"
#include "track_drum.h"
#include "track_mixer.h"
#include "track_output_gain.h"
static struct track_engine state;
static struct track_chip_engine chip;
static struct track_drum_engine drums;
static struct track_mixer mixer;
static int16_t pcm[2048*2];
static uint64_t energy(void){uint64_t n=0;for(unsigned k=0;k<4096;k++){int32_t v=pcm[k];n+=(uint32_t)(v<0?-v:v);}return n;}
int main(void){
 track_init(&state);track_mixer_init(&mixer);track_chip_init(&chip);track_drum_init(&drums);
 assert(state.transport.bpm==120);
 assert(TRACK_FX_COUNT==11 && TRACK_FX_VOLUME==10 && TRACK_NOTE_STOP==254 && TRACK_NOTE_EMPTY==255);
 assert(track_note_scroll(127,1,0)==255);
 assert(track_note_scroll(255,1,0)==254);
 assert(track_note_scroll(254,1,0)==0);
 assert(track_note_scroll(0,-1,0)==254);
 assert(track_note_scroll(13,1,1)==255);
 assert(track_note_scroll(255,1,1)==254);
 assert(track_note_scroll(254,1,1)==0);
 assert(track_set_note(&state,0,0,254)==0);
 assert(track_set_note(&state,3,0,254)==0);
 assert(track_enter_note(&state,254)==0);
 assert(state.transport.edit_step==1);
 assert(track_adjust_octave(&state,12)==0); /* EMPTY on next step */
 state.active.ch[0][0].effect=TRACK_FX_VOLUME;state.active.ch[0][0].value=0;
 uint8_t bytes[TRACK_PATTERN_BYTES];struct track_pattern roundtrip;
 assert(track_pattern_encode(&state.active,bytes,sizeof(bytes))==TRACK_PATTERN_BYTES);
 assert(track_pattern_decode(&roundtrip,bytes,sizeof(bytes))==0);
 assert(roundtrip.ch[0][0].note==254 && roundtrip.ch[0][0].effect==10 && roundtrip.ch[0][0].value==0);
 assert(track_chip_note(&chip,0,0,60,94));
 memset(pcm,0,sizeof(pcm));track_chip_render(&chip,&mixer,pcm,2048);
 assert(energy()>30000u);
 assert(chip.voices[0].active && !chip.voices[0].released);
 for(unsigned k=0;k<16;k++){track_chip_begin_step(&chip);track_chip_render(&chip,&mixer,pcm,2048);}
 assert(chip.voices[0].active && !chip.voices[0].released); /* 0..100% sustain */
 /* Automating VOLUME on a later empty step must also mute held notes,
  * without releasing them. Returning to 100 must restore the same note. */
 chip.step_gain[0]=0;
 memset(pcm,0,sizeof(pcm));track_chip_render(&chip,&mixer,pcm,2048);
 assert(energy()==0 && chip.voices[0].active);
 chip.step_gain[0]=100;
 memset(pcm,0,sizeof(pcm));track_chip_render(&chip,&mixer,pcm,2048);
 assert(energy()>30000u);
 track_chip_release_channel(&chip,0);
 assert(chip.voices[0].released);
 for(unsigned k=0;k<20;k++)track_chip_render(&chip,&mixer,pcm,2048);
 assert(!chip.voices[0].active);
 assert(strcmp(track_drum_name(TRACK_NOTE_STOP),"STP")==0);
 assert(track_drum_trigger_pair_percent(&drums,0,255,0)==0);
 memset(pcm,0,sizeof(pcm));track_drum_render(&drums,&mixer,pcm,2048);
 assert(energy()==0);
 assert(track_drum_trigger_pair_percent(&drums,0,255,100)==1);
 memset(pcm,0,sizeof(pcm));track_drum_render(&drums,&mixer,pcm,2048);
 uint64_t full=energy();assert(full>10000);
 track_drum_stop(&drums);
 assert(track_drum_trigger_pair_percent(&drums,0,255,50)==1);
 memset(pcm,0,sizeof(pcm));track_drum_render(&drums,&mixer,pcm,2048);
 uint64_t half=energy();assert(half>0 && half<full);
 int32_t gain=32767<<16;
 assert(track_master_apply(21000,0,&gain)==0 && gain==0);
 int16_t low=track_master_apply(21000,8192,&gain);
 assert(low>=0 && low<=21000/4+1); /* never overshoot physical knob */
 for(unsigned k=0;k<1000;k++)low=track_master_apply(21000,8192,&gain);
 assert(low>0&&low<=21000/4+1);
 assert(track_master_apply(-21000,0,&gain)==0);
 puts("BEETLE V0.30: note STP, scroll, project roundtrip, sustained sound, release, per-step drum gain, master cap PASS");
 return 0;
}
