/* Host reconstruction of saved MOUNTAINKING Song Chain for level/performance diagnostics.
 * Does not reproduce full native interrupt path or prove hardware safety. */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "track_project_store.h"
#include "track_core.h"
#include "track_chip.h"
#include "track_drum.h"
#include "track_song.h"
#include "track_mixer.h"
#include "track_master_fx.h"
static struct track_store_project p;
static struct track_chip_engine chip;
static struct track_drum_engine drum;
static struct track_master_fx_state master;
static struct track_engine engine;
static uint8_t raw[TRACK_STORE_MAX];
static int16_t pcm[256];
static uint64_t ns(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (uint64_t)t.tv_sec*1000000000u+t.tv_nsec;}
int main(int argc,char**argv){
 if(argc!=2)return 2;FILE*f=fopen(argv[1],"rb");if(!f)return 2;size_t n=fread(raw,1,sizeof raw,f);fclose(f);
 if(track_store_decode(&p,raw,n))return 3;
 track_chip_init(&chip);track_drum_init(&drum);track_master_dsp_init(&master);track_chip_fx_reset();
 for(unsigned i=0;i<TRACK_CHIP_SLOTS;i++){const struct track_store_instrument *s=&p.instruments[i];chip.osc_type[i]=s->type;chip.env_attack[i]=s->attack;chip.env_decay[i]=s->decay;chip.env_sustain[i]=s->sustain;chip.env_release[i]=s->release;chip.patches[i]=s->va;}
 memcpy(drum.components,p.drum,sizeof(drum.components));
 track_init(&engine);engine.transport.bpm=p.bpm;engine.harmony=p.harmony;
 struct track_song_bank *song=&p.song;track_song_start(song,&engine);
 for(unsigned pos=0;pos<song->count;pos++){
  unsigned peak=0,clip=0,near=0,active_max=0,drum_max=0;
  uint64_t time=0,max_us=0;uint32_t frames_total=0;
  for(unsigned step=0;step<engine.active.length;step++){
   for(unsigned c=0;c<TRACK_SYNTH_CHANNELS;c++){
    struct track_step *s=&engine.active.ch[c][step];
    if(s->note==TRACK_NOTE_STOP)track_chip_release_channel(&chip,c);
    else if(s->note<128){track_chip_stop_channel(&chip,c);uint8_t pitches[5];int nn=track_chord_keylocked(s->note,s->chord,&p.harmony,pitches);for(int j=0;j<nn;j++)track_chip_note(&chip,c,s->instrument,pitches[j],track_step_velocity(s));}
   }
   struct track_step *d=&engine.active.ch[TRACK_DRUM_CHANNEL][step];
   if(d->note<TRACK_DRUM_NOTE_COUNT)track_drum_trigger_pair(&drum,d->note,d->chord>0&&d->chord<=TRACK_DRUM_COUNT?d->chord-1:TRACK_NOTE_EMPTY);
   else if(d->chord>0&&d->chord<=TRACK_DRUM_COUNT)track_drum_trigger(&drum,d->chord-1);
   int samples=661500/(engine.transport.bpm?engine.transport.bpm:120);
   while(samples>0){unsigned count=samples>128?128:samples;
    uint64_t st=ns();track_chip_render(&chip,&p.mixer,pcm,count);track_drum_render(&drum,&p.mixer,pcm,count);track_chip_process_punch(&chip,0,pcm,count);track_master_process(&master,&p.master,pcm,count);uint64_t dt=ns()-st;time+=dt;if(dt>max_us)max_us=dt;
    for(int i=0;i<(int)count*2;i++){unsigned v=pcm[i]<0?-pcm[i]:pcm[i];if(v>peak)peak=v;if(v>=32760)clip++;if(v>30500)near++;}
    unsigned av=0,dv=0;for(unsigned i=0;i<chip.voice_count;i++)if(chip.voices[i].active)av++;
    for(unsigned i=0;i<TRACK_DRUM_POLY;i++)if(drum.voices[i].active)dv++;
    if(av>active_max)active_max=av;if(dv>drum_max)drum_max=dv;
    frames_total+=count;samples-=count;
   }
  }
  printf("POSITION %02u peak=%5u near30k=%6u clip=%6u maxsynth=%u maxdrum=%u avgDSP=%.3f%% HOST max128us=%.1f\n",pos+1,peak,near,clip,active_max,drum_max,(double)time/(frames_total*1e9/44100)*100.,(double)max_us/1000.);
  if(pos+1<song->count){song->position=pos; song->repeat_index=0;track_song_on_wrap(song,&engine);}
 }
 return 0;
}
