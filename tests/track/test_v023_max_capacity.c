#include <stdio.h>
#include <string.h>
#include "track_journal.h"
static struct track_store_project p;
static struct track_engine e;
static uint8_t scratch[TRACK_STORE_MAX];
int main(void){
 track_init(&e);memset(&p,0,sizeof(p));track_song_init(&p.song,&e);track_mixer_init(&p.mixer);
 p.song.count=TRACK_SONG_ENTRIES;p.song.loop=1;p.slot=3;p.bpm=120;
 for(unsigned i=0;i<p.song.count;i++){p.song.chain[i].pattern=i%TRACK_PATTERNS;p.song.chain[i].repeats=16;}
 for(unsigned pattern=0;pattern<TRACK_PATTERNS;pattern++){
  for(unsigned ch=0;ch<4;ch++)for(unsigned step=0;step<TRACK_STEPS;step++){
   struct track_step *s=&p.song.patterns[pattern].ch[ch][step];
   s->note=ch==3?3:60;s->instrument=ch;s->chord=0;s->effect=7;s->value=100;
  }
  struct track_song_clip *clip=&p.song.clips[pattern];clip->count=TRACK_SONG_FX_EVENTS;
  for(unsigned ev=0;ev<TRACK_SONG_FX_EVENTS;ev++){
   clip->data[ev]=(struct track_song_event){(uint8_t)(ev%16),(uint8_t)(ev%8),(uint8_t)(ev%2)};
  }
 }
 for(unsigned i=0;i<4;i++){struct track_store_instrument*v=&p.instruments[i];
  v->type=i;v->duty=50;v->volume=75;v->cutoff=7000;v->attack=1;v->decay=20;
  track_va_patch_default(&v->va);
 }
 unsigned len=0;int result=track_store_encode(&p,scratch,sizeof(scratch),&len);
 if(result || len + TRACK_JOURNAL_HEADER_BYTES > TRACK_JOURNAL_BANK_BYTES){
  printf("FAIL: result=%d payload=%u (max bank=%u)\n",result,len,TRACK_JOURNAL_BANK_BYTES);return 1;
 }
 printf("PASS: worst-case 48 patterns, 64 song entries, 24 punch events per pattern, four VA patches = %u B in %u B bank\n",len,TRACK_JOURNAL_BANK_BYTES);
 return 0;
}
