/* BEETLE V0.32: original V0.31 preservation + targeted ETCH V1.4 improvements. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "track_chip.h"
#include "track_core.h"
#include "track_song.h"
#include "track_mixer.h"
#include "track_project_store.h"
#include "track_journal.h"
#include "track_swing.h"
static struct track_store_project project, decoded;
static uint8_t encoded[TRACK_STORE_MAX];
static struct track_chip_engine chip;
static int16_t pcm[4096];
static unsigned count_active(uint8_t ch){unsigned count=0;for(unsigned j=0;j<chip.voice_count;j++)if(chip.voices[j].active && chip.voices[j].channel==ch && !chip.voices[j].released)count++;return count;}
static uint64_t energy(void){uint64_t energy=0;for(int k=0;k<4096;k++) {int32_t s=pcm[k];energy+=(uint32_t)(s<0?-s:s);}return energy;}
int main(int argc,char **argv){
 assert(TRACK_PROJECTS==12 && TRACK_STORE_VERSION==4 && TRACK_JOURNAL_SLOTS==12);
 track_chip_init(&chip);
 struct track_va_patch *p=&chip.patches[0];
 assert(track_va_patch_valid(p));
 p->coarse=0;p->fine=0;p->coarse_b=12;p->fine_b=35;p->pulse_width=10;p->pulse_width_b=90;
 assert(track_va_patch_valid(p));
 chip.osc_type[0]=TRACK_CHIP_VA_OSC2;
 assert(track_chip_note(&chip,0,0,60,100));
 assert(chip.voices[0].step2>chip.voices[0].step);
 assert(track_chip_note(&chip,1,0,66,100));
 assert(count_active(0)==1&&count_active(1)==1);
 for(unsigned j=0;j<80;j++){
  track_chip_stop_channel(&chip,0);
  assert(track_chip_note(&chip,0,0,(uint8_t)(60+j%12),110));
  memset(pcm,0,sizeof(pcm));track_chip_render(&chip,NULL,pcm,1024);
  assert(energy()!=0);
  assert(count_active(0)==1 && count_active(1)==1);
  assert(track_chip_health(&chip));
 }
 track_chip_stop_channel(&chip,0);track_chip_render(&chip,NULL,pcm,1024);
 assert(count_active(0)==0&&count_active(1)==1);
 /* A silent EMPTY step leaves that existing note untouched. */
 track_chip_begin_step(&chip);track_chip_render(&chip,NULL,pcm,1024);
 assert(count_active(1)==1);
 /* Independent PW and shape matter audibly, not merely on UI/flash. */
 track_chip_init(&chip);chip.osc_type[0]=TRACK_CHIP_VA_OSC2;
 chip.patches[0].wave_b=4;chip.patches[0].mix=100;chip.patches[0].morph=0;
 chip.patches[0].pulse_width_b=12;assert(track_chip_note(&chip,0,0,60,127));
 track_chip_render(&chip,NULL,pcm,2048);uint64_t a=energy();int16_t snapshot[4096];memcpy(snapshot,pcm,sizeof(pcm));
 track_chip_init(&chip);chip.osc_type[0]=TRACK_CHIP_VA_OSC2;
 chip.patches[0].wave_b=4;chip.patches[0].mix=100;chip.patches[0].morph=0;
 chip.patches[0].pulse_width_b=78;assert(track_chip_note(&chip,0,0,60,127));
 track_chip_render(&chip,NULL,pcm,2048);assert(a>0 && energy()>0 && memcmp(snapshot,pcm,sizeof(pcm))!=0);
 struct track_engine e;track_init(&e);
 memset(&project,0,sizeof(project));track_song_init(&project.song,&e);track_mixer_init(&project.mixer);
 project.slot=11;project.bpm=172;project.mixer.ch[2].swing=1;
 for(unsigned j=0;j<4;j++){project.instruments[j].type=j;project.instruments[j].volume=70;project.instruments[j].duty=50;project.instruments[j].cutoff=7000;track_va_patch_default(&project.instruments[j].va);}
 project.instruments[0].va.coarse=-4;project.instruments[0].va.coarse_b=17;
 project.instruments[0].va.fine=23;project.instruments[0].va.fine_b=-40;
 project.instruments[0].va.pulse_width=7;project.instruments[0].va.pulse_width_b=83;
 project.song.patterns[47].ch[0][15].note=TRACK_NOTE_STOP;
 project.song.patterns[47].ch[1][9].effect=TRACK_FX_VOLUME;
 project.song.patterns[47].ch[1][9].value=0;
 project.song.patterns[47].ch[3][15].note=13;
 uint32_t used=0;assert(track_store_encode(&project,encoded,sizeof(encoded),&used)==0);
 assert(used<TRACK_JOURNAL_BANK_BYTES-TRACK_JOURNAL_HEADER_BYTES);
 assert(track_store_decode(&decoded,encoded,used)==0);
 assert(decoded.slot==11&&decoded.bpm==172&&decoded.mixer.ch[2].swing==1);
 assert(decoded.instruments[0].va.coarse_b==17&&decoded.instruments[0].va.fine_b==-40&&decoded.instruments[0].va.pulse_width_b==83);
 assert(decoded.song.patterns[47].ch[0][15].note==TRACK_NOTE_STOP);
 assert(decoded.song.patterns[47].ch[1][9].effect==TRACK_FX_VOLUME);
 if(argc!=2){fprintf(stderr,"usage: test_beetle_v032 legacy-v031-project-v3.bin\n");return 2;}
 FILE *f=fopen(argv[1],"rb");assert(f);
 size_t size=fread(encoded,1,sizeof(encoded),f);fclose(f);
 assert(size>10000&&encoded[4]==3);
 assert(track_store_decode(&decoded,encoded,(uint32_t)size)==0);
 assert(decoded.slot==11&&decoded.bpm==134&&decoded.mixer.ch[2].swing==1);
 assert(decoded.song.patterns[47].ch[0][5].note==72);
 assert(decoded.song.patterns[47].ch[1][10].value==31);
 assert(decoded.instruments[0].va.coarse_b==9);
 assert(decoded.instruments[0].va.fine_b==-25);
 assert(decoded.instruments[0].va.pulse_width_b==32);
 puts("PASS BEETLE V0.32: independent VA A/B, lane retrigger/STP, V4 lossless roundtrip, 12 slots, V3 import");return 0;
}
