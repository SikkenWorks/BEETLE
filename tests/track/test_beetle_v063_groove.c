/* V0.63 regression: compact groove, sample-accurate bounded schedule, note2
 * STOP wire roundtrip, migration and corruption rejection. */
#include "track_project_store.h"
#include "track_master_fx.h"
#include "beetle_step_groove.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static struct track_store_project a,b;
static uint8_t buffer[TRACK_STORE_MAX];
int main(int argc,char **argv){
 struct track_engine e;track_init(&e);track_song_init(&a.song,&e);
 track_mixer_init(&a.mixer);track_master_default(&a.master);
 a.slot=1u;a.bpm=120u;
 for(unsigned i=0;i<4;i++){a.instruments[i].volume=65;track_va_patch_default(&a.instruments[i].va);}
 for(unsigned i=0;i<TRACK_DRUM_COUNT;i++)track_drum_timbre_default(&a.drum[i]);
 assert(sizeof(struct track_step)==5u);
 struct track_step *s=&a.song.patterns[0].ch[1][2];s->note=62;
 track_groove_set_micro(s,7);track_groove_set_ratchet(s,4);track_groove_set_probability(s,2);
 track_step_set_vel(s,11u);track_step_set_gate(s,9u);
 assert(s->groove==((2u<<5)|(3u<<3)|7u));
 assert(TRACK_GROOVE_MICRO(s)==7u&&TRACK_GROOVE_RATCHET(s)==4u&&TRACK_GROOVE_PROB(s)==50u);
 assert(track_step_velocity(s)==88u&&track_step_gate(s)==9u);
 a.song.patterns[0].ch[5][4].chord=15u; /* NOTE2 STP, NOT NOTE1 STP */
 a.song.patterns[0].ch[5][4].note=0u;
 uint32_t used=0;assert(track_store_encode(&a,buffer,sizeof(buffer),&used)==0);
 assert(buffer[4]==TRACK_STORE_VERSION&&buffer[5]==0u); /* current version; V12 is migration-tested by V0.66 */
 assert(track_store_decode(&b,buffer,used)==0);
 assert(b.song.patterns[0].ch[1][2].groove==s->groove);
 assert(b.song.patterns[0].ch[1][2].dynamics==s->dynamics);
 assert(b.song.patterns[0].ch[5][4].chord==15u);
 assert(b.song.patterns[0].ch[5][4].note==0u);
 buffer[used-1]^=0x7u;assert(track_store_decode(&b,buffer,used)!=0);
 struct track_pattern pat={0},q={0};pat.length=16u;
 for(unsigned c=0;c<TRACK_CHANNELS;c++)for(unsigned i=0;i<TRACK_STEPS;i++){
  pat.ch[c][i].note=TRACK_NOTE_EMPTY;pat.ch[c][i].value=50u;
  pat.ch[c][i].instrument=(uint8_t)(c==5u?3u:c%4u);
 }
 pat.ch[5][3].chord=15u;pat.ch[1][3].note=67u;pat.ch[1][3].groove=0x4fu;
 uint8_t packed[TRACK_PATTERN_BYTES];
 assert(track_pattern_encode(&pat,packed,sizeof(packed))==(int)sizeof(packed));
 assert(packed[2]==3u && track_pattern_decode(&q,packed,sizeof(packed))==0);
 assert(q.ch[1][3].groove==0x4fu && q.ch[5][3].chord==15u);
 /* V2 portable project: same fields minus last groove byte. */
 uint8_t old[TRACK_PATTERN_V2_BYTES];old[0]='T';old[1]='R';old[2]=2;old[3]=16;
 uint32_t at=4;
 for(unsigned c=0;c<6;c++)for(unsigned i=0;i<16;i++){
  const struct track_step *t=&pat.ch[c][i];
  old[at++]=t->note;old[at++]=(uint8_t)t->instrument;old[at++]=(uint8_t)t->chord;
  old[at++]=(uint8_t)t->effect;old[at++]=(uint8_t)t->value;old[at++]=(uint8_t)t->dynamics;
 }
 assert(at==sizeof(old));assert(track_pattern_decode(&q,old,sizeof(old))==0);
 assert(q.ch[1][3].note==67u && q.ch[1][3].groove==0u);
 if(argc==2){FILE *f=fopen(argv[1],"rb");assert(f);size_t n=fread(buffer,1,sizeof(buffer),f);fclose(f);
  assert(track_store_decode(&b,buffer,(uint32_t)n)==0);
  assert(b.song.patterns[4].ch[1][7].note==60u && b.song.patterns[4].ch[1][7].groove==0u);
 }
 struct beetle_groove_clock g={0};beetle_groove_reset(&g);
 struct track_pattern active=pat;active.ch[0][0].note=65u;
 for(unsigned c=0;c<6;c++)active.ch[c][0].groove=0u;
 track_groove_set_micro(&active.ch[0][0],7);
 track_groove_set_ratchet(&active.ch[0][0],4);
 uint32_t swing[6]={0,0,0,0,0,0};
 beetle_groove_schedule(&g,&active,0u,5512u,swing);
 assert(beetle_groove_due(&g,0u)==0x3eu);
 uint32_t when=g.lane[0].at;assert(when==5512u*7u/64u);
 for(int i=0;i<4;i++){
  assert(beetle_groove_wait(&g,when-1u,10000u)==1u);
  assert(beetle_groove_due(&g,when)&1u);
  when=g.lane[0].at;
 }
 assert(g.lane[0].remaining==0u);
 /* Probability remains within 25..100%; no allocation or randomized timing. */
 track_groove_set_probability(&active.ch[0][0],3u);
 unsigned hits=0;for(int i=0;i<512;i++){
  beetle_groove_schedule(&g,&active,0u,5512u,swing);
  if(!g.lane[0].skip_note)hits++;
 }
 assert(hits>80u&&hits<180u);
 printf("PASS BEETLE V0.63 5-byte groove / V12 / NOTE2 STP / V10 migration / sample scheduler (%u/512 hits)\n",hits);
 return 0;
}
