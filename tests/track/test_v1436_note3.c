/* BEETLE V1.4.36 — NOTE3 and no-step-advance regression, GPL-3.0-only. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "track_project_store.h"
#include "beetle_instrument_library.h"
#include "track_master_fx.h"
static struct track_store_project p,q;
static uint8_t blob[TRACK_STORE_V4_MAX];
int main(int argc,char **argv){
 struct track_engine e;track_init(&e);
 e.transport.edit_channel=0;e.transport.edit_step=7;
 assert(track_enter_note(&e,60)==0&&e.transport.edit_step==7);
 assert(track_enter_note(&e,64)==0&&e.transport.edit_step==7);
 e.transport.edit_channel=TRACK_DRUM_CHANNEL;e.transport.edit_step=8;e.transport.edit_column=2;
 for(int id=0;id<27;id++){
  assert(track_enter_drum_note3(&e,(uint8_t)id)==0);
  assert(e.active.note3[8]==id && e.transport.edit_step==8);
  uint8_t out[TRACK_PATTERN_BYTES];struct track_pattern copy;
  int n=track_pattern_encode(&e.active,out,sizeof out);
  assert(n==TRACK_PATTERN_BYTES&&out[2]==5);
  assert(track_pattern_decode(&copy,out,(uint32_t)n)==0&&copy.note3[8]==id);
  out[2]=4;
  assert(track_pattern_decode(&copy,out,TRACK_PATTERN_V4_BYTES)==0);
  for(unsigned k=0;k<16;k++)assert(copy.note3[k]==TRACK_NOTE_EMPTY);
 }
 e.active.note3[8]=TRACK_NOTE_STOP;
 assert(track_clear_step(&e,5,8)==0&&e.active.note3[8]==TRACK_NOTE_EMPTY);
 track_song_init(&p.song,&e);track_mixer_init(&p.mixer);track_master_default(&p.master);
 beetle_library_init(&p.library);for(int k=0;k<TRACK_DRUM_COUNT;k++)track_drum_timbre_default(&p.drum[k]);
 for(int k=0;k<8;k++){p.instruments[k].volume=65;p.instruments[k].duty=50;p.instruments[k].cutoff=7000;track_va_patch_default(&p.instruments[k].va);}
 p.bpm=120;p.drum_note3_enabled=1;memcpy(p.title,"NOTE3 TEST  ",12);
 p.song.patterns[0].ch[5][1].note=6;
 track_note2_set(&p.song.patterns[0].ch[5][1],20);
 p.song.patterns[0].note3[1]=TRACK_NOTE_STOP;
 p.song.patterns[0].note3[2]=26;
 p.song.patterns[0].ch[5][1].effect=3;
 p.song.patterns[0].note2_fx[1].effect=4;
 uint32_t n=0;assert(track_store_encode(&p,blob,sizeof blob,&n)==0);
 assert(n<TRACK_STORE_V4_MAX && blob[4]==26);
 assert(track_store_decode(&q,blob,n)==0);
 assert(q.drum_note3_enabled==1);
 assert(q.song.patterns[0].ch[5][1].note==6);
 assert(track_note2_id(&q.song.patterns[0].ch[5][1])==20);
 assert(q.song.patterns[0].note3[1]==TRACK_NOTE_STOP&&q.song.patterns[0].note3[2]==26);
 assert(q.song.patterns[0].ch[5][1].effect==3 && q.song.patterns[0].note2_fx[1].effect==4);
 q.drum_note3_enabled=0;assert(track_store_encode(&q,blob,sizeof blob,&n)==0);
 memset(&q,0,sizeof q);assert(track_store_decode(&q,blob,n)==0);
 assert(q.drum_note3_enabled==0 && q.song.patterns[0].note3[2]==26); /* preserve hidden data */
 if(argc>1){FILE*f=fopen(argv[1],"rb");assert(f);size_t cnt=fread(blob,1,sizeof blob,f);fclose(f);
  assert(track_store_decode(&q,blob,(uint32_t)cnt)==0);assert(q.drum_note3_enabled==0);
  for(unsigned r=0;r<TRACK_PATTERNS;r++)for(unsigned st=0;st<TRACK_STEPS;st++)assert(q.song.patterns[r].note3[st]==TRACK_NOTE_EMPTY);
 }
 puts("PASS V1.4.36 native: NOTE cursor stable, 27 drum IDs, independent NOTE3/STP, TR5/V4 migration, V26 persistence, mode retention, legacy V25");return 0;
}
