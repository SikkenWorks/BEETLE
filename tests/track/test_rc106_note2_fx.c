/* SPDX-License-Identifier: GPL-3.0-only: V18 independent drum NOTE2 FX + legacy V17. */
#include "track_project_store.h"
#include "track_lane_clipboard.h"
#include "track_master_fx.h"
#include "beetle_instrument_library.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static struct track_store_project p,q;
static uint8_t buf[TRACK_STORE_MAX];
static void set_step(struct track_step *s,uint8_t n,uint8_t fx,uint8_t val){s->note=n;s->effect=fx;s->value=val;}
int main(int argc,char **argv){
 struct track_engine e;track_init(&e);track_song_init(&p.song,&e);
 track_mixer_init(&p.mixer);track_master_default(&p.master);
 p.slot=0;p.bpm=120;
 for(uint8_t i=0;i<4;i++){
  p.instruments[i].volume=65;p.instruments[i].duty=50;
  p.instruments[i].cutoff=7000;p.instruments[i].type=i;
  track_va_patch_default(&p.instruments[i].va);
 }
 for(uint8_t i=0;i<TRACK_DRUM_COUNT;i++)track_drum_timbre_default(&p.drum[i]);
 beetle_library_init(&p.library);
 struct track_pattern *pat=&p.song.patterns[3];
 set_step(&pat->ch[5][2],3u,10u,65u);pat->ch[5][2].chord=4u;
 pat->note2_fx[2].effect=11u;pat->note2_fx[2].value=18u;
 set_step(&pat->ch[5][3],5u,11u,8u);pat->ch[5][3].chord=7u;
 pat->note2_fx[3].effect=10u;pat->note2_fx[3].value=45u;
 uint32_t n=0;assert(track_store_encode(&p,buf,sizeof buf,&n)==0);
 assert(n<TRACK_STORE_V4_MAX&&buf[4]==18u&&buf[5]==0u);
 if(argc>2){FILE*f=fopen(argv[2],"wb");assert(f);assert(fwrite(buf,1,n,f)==n);fclose(f);}
 assert(track_store_decode(&q,buf,n)==0);
 assert(q.song.patterns[3].ch[5][2].effect==10u);
 assert(q.song.patterns[3].note2_fx[2].effect==11u);
 assert(q.song.patterns[3].note2_fx[2].value==18u);
 assert(q.song.patterns[3].note2_fx[3].effect==10u);
 assert(q.song.patterns[3].note2_fx[3].value==45u);
 struct track_pattern rotated=q.song.patterns[3];
 assert(track_lane_rotate(&rotated,5,1,+1));
 assert(rotated.ch[5][2].note==3u&&rotated.ch[5][2].effect==10u);
 assert(rotated.ch[5][2].chord==0u&&rotated.note2_fx[2].effect==0u);
 assert(rotated.ch[5][3].chord==4u&&rotated.note2_fx[3].effect==11u);
 assert(track_lane_rotate(&rotated,5,1,-1));
 assert(rotated.ch[5][2].chord==4u&&rotated.note2_fx[2].value==18u);
 assert(track_lane_rotate(&rotated,5,0,-1));
 assert(rotated.ch[5][1].note==3u&&rotated.ch[5][1].effect==10u);
 assert(rotated.ch[5][1].chord==0u);
 assert(rotated.ch[5][2].chord==4u&&rotated.note2_fx[2].value==18u);
 struct track_lane_clipboard clip={0};
 assert(track_lane_clip_copy_range(&clip,&q.song.patterns[3],5,2,3));
 assert(clip.count==2);
 struct track_pattern pasted=q.song.patterns[1];
 assert(track_lane_clip_paste_at(&clip,&pasted,5,6));
 assert(pasted.ch[5][6].note==3u&&pasted.ch[5][6].chord==4u);
 assert(pasted.note2_fx[6].value==18u);
 assert(pasted.note2_fx[7].value==45u);
 if(argc>1){
  FILE*f=fopen(argv[1],"rb");assert(f);
  assert(fseek(f,0,SEEK_END)==0);long sz=ftell(f);assert(sz>24&&sz<(long)sizeof buf);
  rewind(f);assert(fread(buf,1,sz,f)==(size_t)sz);fclose(f);
  assert(buf[4]==17u);assert(track_store_decode(&q,buf,(uint32_t)sz)==0);
  for(uint8_t r=0;r<TRACK_PATTERNS;r++)for(uint8_t s=0;s<TRACK_STEPS;s++)
   assert(q.song.patterns[r].note2_fx[s].effect==0u);
 }
 if(argc>3){FILE*f=fopen(argv[3],"rb");assert(f);assert(fseek(f,0,SEEK_END)==0);long size=ftell(f);assert(size>24 && size<(long)sizeof buf);rewind(f);assert(fread(buf,1,size,f)==(size_t)size);fclose(f);assert(track_store_decode(&q,buf,(uint32_t)size)==0);assert(q.song.patterns[3].note2_fx[2].effect==11u);assert(q.song.patterns[3].note2_fx[2].value==18u);}
 printf("PASS RC1.0.6 V18 NOTE1/NOTE2-FX, independent rotation, range paste, V17 migration\n");
 return 0;
}
