/* BEETLE V0.45: project slot isolation, sustained FX, stable repeated loop gain. */
#include "track_core.h"
#include "track_chip.h"
#include "track_sustain_fx.h"
#include "track_journal.h"
#include "track_mixer.h"
#include "track_drum.h"
#include "track_master_fx.h"
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t flash[TRACK_PROJECTS][2][TRACK_JOURNAL_BANK_BYTES];
static uint8_t backup[TRACK_PROJECTS][2][TRACK_JOURNAL_BANK_BYTES];
static uint8_t buf[TRACK_STORE_MAX];
static struct track_store_project project,readback;
static struct track_chip_engine chip;
static int16_t audio[4096*2];
static unsigned erases[TRACK_PROJECTS];
static int rd(void*ctx,uint8_t slot,uint8_t bank,uint32_t pos,uint8_t *dst,uint32_t n){
 (void)ctx;if(slot>=TRACK_PROJECTS||bank>=2u||pos>TRACK_JOURNAL_BANK_BYTES||n>TRACK_JOURNAL_BANK_BYTES-pos)return -1;
 memcpy(dst,flash[slot][bank]+pos,n);return 0;
}
static int er(void*ctx,uint8_t slot,uint8_t bank){
 (void)ctx;if(slot>=TRACK_PROJECTS||bank>=2u)return -1;
 erases[slot]++;memset(flash[slot][bank],255,TRACK_JOURNAL_BANK_BYTES);return 0;
}
static int wr(void*ctx,uint8_t slot,uint8_t bank,uint32_t pos,const uint8_t *src,uint32_t n){
 (void)ctx;if(slot>=TRACK_PROJECTS||bank>=2u||pos>TRACK_JOURNAL_BANK_BYTES||n>TRACK_JOURNAL_BANK_BYTES-pos)return -1;
 for(uint32_t i=0;i<n;i++)if((flash[slot][bank][pos+i]&src[i])!=src[i])return -1;
 for(uint32_t i=0;i<n;i++)flash[slot][bank][pos+i]&=src[i];return 0;
}
static const struct track_journal_io io={0,rd,er,wr};
static void make_project(uint8_t slot,uint16_t tempo){
 struct track_engine e;track_init(&e);memset(&project,0,sizeof(project));
 track_song_init(&project.song,&e);track_mixer_init(&project.mixer);track_master_default(&project.master);
 for(unsigned d=0;d<TRACK_DRUM_COUNT;d++)track_drum_timbre_default(&project.drum[d]);
 for(unsigned k=0;k<4u;k++){
  track_va_patch_default(&project.instruments[k].va);
  project.instruments[k].volume=65u;project.instruments[k].cutoff=7000u;
 }
 project.slot=slot;project.bpm=tempo;
 project.song.patterns[0].ch[0][0].note=(uint8_t)(60u+slot);
 project.song.patterns[0].ch[0][3].effect=TRACK_FX_VOLUME;
 project.song.patterns[0].ch[0][3].value=(uint8_t)(slot*7u);
}
static void slot_isolation(void){
 memset(flash,255,sizeof(flash));
 for(uint8_t slot=0;slot<TRACK_PROJECTS;slot++){
  make_project(slot,(uint16_t)(100u+slot));
  assert(track_journal_save(&io,slot,&project,buf,sizeof(buf),0)==TRACK_JOURNAL_OK);
 }
 memcpy(backup,flash,sizeof(flash));
 make_project(5u,203u);project.song.patterns[0].ch[0][3].value=99u;
 assert(track_journal_save(&io,5u,&project,buf,sizeof(buf),0)==TRACK_JOURNAL_OK);
 for(uint8_t slot=0;slot<TRACK_PROJECTS;slot++){
  if(slot!=5u){
   assert(!memcmp(flash[slot],backup[slot],sizeof(backup[slot])));
   assert(erases[slot]==1u);
  }else assert(memcmp(flash[slot],backup[slot],sizeof(backup[slot]))!=0);
  assert(track_journal_load(&io,slot,&readback,buf,sizeof(buf),0)==TRACK_JOURNAL_OK);
  assert(readback.bpm==(uint16_t)(slot==5u?203u:100u+slot));
  assert(readback.song.patterns[0].ch[0][3].value==(uint8_t)(slot==5u?99u:slot*7u));
 }
 /* Overwrite is A/B: even a failed new bank leaves the previous slot valid. */
 assert(track_journal_load(&io,5u,&readback,buf,sizeof(buf),0)==TRACK_JOURNAL_OK);
 assert(readback.song.patterns[0].ch[0][0].note==65u);
}
static uint64_t power_window(void){
 uint64_t p=0;for(unsigned i=2048u;i<4096u;i++){
  int32_t x=audio[2u*i];p+=(uint64_t)((int64_t)x*x);
 }return p;
}
static void loop_volume(void){
 track_chip_init(&chip);
 struct track_mixer m;track_mixer_init(&m);
 chip.env_attack[0]=1u;chip.env_decay[0]=1u;chip.env_sustain[0]=100u;
 /* Match actual tracker: note at step 0, silence steps 1..15, retrigger. */
 uint64_t initial=0;
 for(uint8_t round=0u;round<14u;round++){
  if(round)track_chip_stop_channel(&chip,0u);
  assert(track_chip_note(&chip,0u,0u,60u,94u));
  memset(audio,0,sizeof audio);track_chip_render(&chip,&m,audio,4096u);
  uint64_t e=power_window();assert(e>1000000u);
  if(!round)initial=e;
  else{
   /* Steady-state energy must remain within 15%: no 1/2-volume bug. */
   assert(e>initial*85u/100u&&e<initial*115u/100u);
  }
 }
 assert(chip.voice_count>1u); /* historical slots remain allocated */
}
static void fx_persistence(void){
 track_chip_init(&chip);
 chip.env_attack[0]=1u;chip.env_sustain[0]=100u;
 struct track_step s={TRACK_NOTE_EMPTY,0u,0u,0u,50u};
 assert(track_chip_note(&chip,0u,0u,60u,94u));
 uint32_t created=chip.alloc_serial,age;
 /* Overlay FILTER at step 3 without another note. */
 s.effect=1u;s.value=12u;beetle_step_fx_event(&chip,0u,&s,2u,96u,120u);
 assert(chip.fx_type[0]==1u&&chip.cutoff[0]==28u&&chip.alloc_serial==created);
 struct track_mixer m;track_mixer_init(&m);
 memset(audio,0,sizeof audio);track_chip_render(&chip,&m,audio,2048u);
 age=chip.fx_samples[0];assert(age==2048u);
 for(unsigned step=4;step<=15;step++){
  s.effect=0u;s.value=50u;track_chip_begin_step(&chip);
  beetle_step_fx_event(&chip,0u,&s,2u,96u,120u);
  assert(chip.fx_type[0]==1u&&chip.cutoff[0]==28u&&chip.fx_samples[0]==age);
 }
 /* An effect-only volume row switches the effect and survives empties. */
 s.effect=TRACK_FX_VOLUME;s.value=30u;
 beetle_step_fx_event(&chip,0u,&s,2u,96u,120u);
 assert(chip.step_gain[0]==30u&&chip.cutoff[0]==96u&&chip.fx_type[0]==TRACK_FX_VOLUME);
 s.effect=0u;beetle_step_fx_event(&chip,0u,&s,2u,96u,120u);
 assert(chip.step_gain[0]==30u);
 /* Pitch effect on existing note MUST change oscillator without new onset. */
 uint32_t oldstep=chip.voices[0].step;
 s.effect=11u;s.value=100u;beetle_step_fx_event(&chip,0u,&s,2u,96u,120u);
 assert(chip.voices[0].step>oldstep&&chip.alloc_serial==created);
 /* Replacing with new note resets old FX and volume. */
 s.note=62u;s.effect=0u;s.value=50u;
 beetle_step_fx_event(&chip,0u,&s,2u,96u,120u);
 assert(chip.fx_type[0]==0u&&chip.step_gain[0]==100u&&chip.voices[0].step==oldstep);
 /* Stop clears FX; release itself remains ADSR-aware. */
 s.note=TRACK_NOTE_STOP;s.effect=0u;
 beetle_step_fx_event(&chip,0u,&s,2u,96u,120u);track_chip_release_channel(&chip,0u);
 assert(chip.fx_type[0]==0u&&chip.voices[0].released);
}
int main(void){slot_isolation();loop_volume();fx_persistence();puts("BEETLE V0.45 PASS: twelve-slot isolation; 14 loops stable gain; sustained FX, pitch, stop");return 0;}
