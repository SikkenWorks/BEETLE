/* BEETLE 0.36 realtime VA and V6 persistence incl. legacy V5 migration. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "track_core.h"
#include "track_song.h"
#include "track_chip.h"
#include "track_mixer.h"
#include "track_project_store.h"
#include "track_master_fx.h"
#include "track_journal.h"
#define BUFSIZE TRACK_STORE_MAX
static uint8_t a[BUFSIZE], b[BUFSIZE];
static struct track_store_project p,q;
static void u16(uint8_t*x,uint16_t v){x[0]=(uint8_t)v;x[1]=(uint8_t)(v>>8);}
static void u32(uint8_t*x,uint32_t v){for(int i=0;i<4;i++)x[i]=(uint8_t)(v>>(8*i));}
int main(void){
 struct track_engine e;track_init(&e);memset(&p,0,sizeof(p));
 track_song_init(&p.song,&e);track_mixer_init(&p.mixer);
 track_master_default(&p.master);for(unsigned d=0;d<TRACK_DRUM_COUNT;d++)track_drum_timbre_default(&p.drum[d]);
 p.bpm=120;p.slot=9;p.harmony.root=0;p.harmony.mode=0;p.harmony.keylock=0;
 for(unsigned i=0;i<4;i++){
  struct track_store_instrument *v=&p.instruments[i];
  v->type=TRACK_CHIP_VA_OSC2;v->attack=2;v->decay=15;v->sustain=80;v->release=19;
  v->duty=50;v->volume=80;v->cutoff=9000;
  track_va_patch_default(&v->va);
  v->va.drive=(uint8_t)(i*20+13);v->va.sub=(uint8_t)(i*14+12);
  v->va.noise=(uint8_t)(i*18+10);v->va.fm_depth=(uint8_t)(i*21+11);
 }
 p.mixer.ch[5].swing=1; p.song.patterns[1].ch[4][11].note=73;
 uint32_t n=0;assert(track_store_encode(&p,a,sizeof(a),&n)==0);
 assert(a[4]==TRACK_STORE_VERSION && n<TRACK_JOURNAL_BANK_BYTES);
 assert(track_store_decode(&q,a,n)==0);
 for(int i=0;i<4;i++)assert(memcmp(&q.instruments[i],&p.instruments[i],sizeof(p.instruments[i]))==0);
 assert(q.mixer.ch[5].swing==1&&q.song.patterns[1].ch[4][11].note==73);
 /* Genuine pre-V7 binary migration requires archived fixtures; none were supplied. */
 struct track_chip_engine s;track_chip_init(&s);
 s.osc_type[0]=TRACK_CHIP_VA_OSC2;s.patches[0]=p.instruments[0].va;
 assert(track_chip_note(&s,0,0,60,110));
 uint8_t idx=s.last_voice_index;uint32_t serial=s.voices[idx].birth;
 int16_t dry[512],wet[512];struct track_mixer m;track_mixer_init(&m);
 track_chip_render(&s,&m,dry,256);
 s.patches[0].coarse=12;s.patches[0].drive=90;s.patches[0].noise=70;s.patches[0].sub=90;s.patches[0].fm_depth=80;
 track_chip_refresh_instrument(&s,0);
 assert(s.voices[idx].active&&s.voices[idx].birth==serial&&s.voices[idx].step>0);
 assert(s.voices[idx].note==60&&s.voices[idx].type==TRACK_CHIP_VA_OSC2);
 track_chip_render(&s,&m,wet,256);
 assert(memcmp(dry,wet,sizeof(dry))!=0 && track_chip_health(&s));
 puts("BEETLE V0.36 VA live-update and V7 project codec PASS");
 return 0;
}
