/* BEETLE V0.38 host regressions: swing, 6-lane clipboard data format,
 * drum/master settings, audio and V7 codec. */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "track_core.h"
#include "track_mixer.h"
#include "track_swing.h"
#include "track_song.h"
#include "track_chip.h"
#include "track_drum.h"
#include "track_project_store.h"
#include "track_master_fx.h"
static struct track_store_project saved, loaded;
static uint8_t bytes[TRACK_STORE_MAX];
static struct track_master_fx_state fx;
static uint32_t checksum(const int16_t *x,unsigned n){uint32_t h=2166136261u;for(unsigned i=0;i<n;i++){h=(h^(uint16_t)x[i])*16777619u;}return h;}
static void storage(void){
 struct track_engine t;track_init(&t);memset(&saved,0,sizeof(saved));
 track_song_init(&saved.song,&t);track_mixer_init(&saved.mixer);
 saved.bpm=138;saved.slot=8;memcpy(saved.title,"BEETLE 038",10);
 track_master_default(&saved.master);
 for(unsigned c=0;c<6;c++){
  saved.mixer.ch[c].swing=(uint8_t)(c*20);
  for(unsigned p=0;p<6;p++){
   struct track_step *s=&saved.song.patterns[p].ch[c][p];
   s->note=(uint8_t)(c==TRACK_DRUM_CHANNEL?3:56+c);
   s->effect=TRACK_FX_VOLUME;s->value=(uint8_t)(72+c);
  }
 }
 for(unsigned d=0;d<TRACK_DRUM_COUNT;d++){
  track_drum_timbre_default(&saved.drum[d]);
  saved.drum[d].attack=(uint8_t)d;saved.drum[d].decay=(uint8_t)(30+d);
  saved.drum[d].sustain=(uint8_t)(d*3);saved.drum[d].release=(uint8_t)(70+d);
  saved.drum[d].pitch=(int8_t)((int)d-7);
  saved.drum[d].eq_low=(uint8_t)(38+d);
  saved.drum[d].eq_mid=(uint8_t)(40+d);
  saved.drum[d].eq_high=(uint8_t)(60+d);
  saved.drum[d].gain=(uint8_t)(75+d);
 }
 saved.master.eq[0]=28;saved.master.eq[1]=71;saved.master.eq[2]=5;saved.master.eq[3]=85;
 saved.master.eq_freq[2]=91;saved.master.eq_width[0]=89;
 saved.master.comp_threshold=38;saved.master.comp_ratio=66;saved.master.comp_gain=77;
 saved.master.reverb_wet=31;saved.master.reverb_feedback=29;saved.master.reverb_tone=19;
 saved.master.order[0]=2;saved.master.order[1]=0;saved.master.order[2]=1;
 saved.master.enabled[1]=0;
 uint32_t used=0;assert(track_store_encode(&saved,bytes,sizeof(bytes),&used)==0);
 printf("BEETLE V7 sparse project %u bytes; physical max 12256\n",used);
 assert(used<=12256u);assert(track_store_decode(&loaded,bytes,used)==0);
 assert(loaded.bpm==138&&loaded.slot==8);
 for(unsigned c=0;c<6;c++){
  assert(loaded.mixer.ch[c].swing==c*20);
  for(unsigned p=0;p<6;p++)assert(memcmp(&saved.song.patterns[p].ch[c][p],&loaded.song.patterns[p].ch[c][p],sizeof(struct track_step))==0);
 }
 for(unsigned d=0;d<TRACK_DRUM_COUNT;d++)assert(memcmp(&saved.drum[d],&loaded.drum[d],sizeof(struct track_drum_timbre))==0);
 assert(memcmp(&saved.master,&loaded.master,sizeof(saved.master))==0);
 assert(track_swing_delay(4000u,0)==0u);
 assert(track_swing_delay(4000u,100)>track_swing_delay(4000u,50));
 assert(track_swing_delay(4000u,100)<2000u);
 saved.mixer.row=0;saved.mixer.col=4;saved.mixer.ch[0].swing=0;
 assert(track_mixer_change(&saved.mixer,3));assert(saved.mixer.ch[0].swing==6);
 assert(track_mixer_change(&saved.mixer,-100));assert(saved.mixer.ch[0].swing==0);
 puts("BEETLE V0.38 V7 storage, swing (0-100) PASS");
}
static void audio(void){
 struct track_master_settings c;track_master_default(&c);
 int16_t dry[2048*2],processed[2048*2];
 for(unsigned i=0;i<2048;i++){dry[i*2]=(int16_t)((i%70)*128-4200);dry[i*2+1]=(int16_t)((i%120)*76-3800);}
 memcpy(processed,dry,sizeof(dry));
 memset(&fx,0,sizeof(fx));track_master_process(&fx,&c,processed,2048);
 assert(checksum(dry,4096)!=checksum(processed,4096));
 uint32_t before=checksum(processed,4096);
 memset(&fx,0,sizeof(fx));c.order[0]=2;c.order[1]=0;c.order[2]=1;c.reverb_tone=0;
 memcpy(processed,dry,sizeof(dry));track_master_process(&fx,&c,processed,2048);
 assert(checksum(processed,4096)!=before);
 c.enabled[0]=0;c.enabled[1]=0;c.enabled[2]=0;
 memcpy(processed,dry,sizeof(dry));track_master_process(&fx,&c,processed,2048);
 assert(checksum(processed,4096)==checksum(dry,4096));
 struct track_drum_engine d;struct track_mixer m;track_mixer_init(&m);track_drum_init(&d);
 d.components[0].pitch=-12;d.components[0].gain=90;d.components[0].attack=5;
 assert(track_drum_trigger(&d,0));int16_t out[1024*2]={0};track_drum_render(&d,&m,out,1024);
 assert(checksum(out,2048)!=checksum(dry,4096));
 struct track_chip_engine s;track_chip_init(&s);track_chip_fx_reset();
 assert(track_chip_note(&s,0,0,60,100));
 s.fx_type[0]=6;s.fx_value[0]=80;
 int16_t data[256*2]={0};track_chip_render(&s,&m,data,256);
 assert(checksum(data,512)!=0);
 puts("BEETLE V0.38 master 3-chain DSP, DRUM, VIBRATO no injected tone PASS");
}
int main(void){storage();audio();return 0;}
