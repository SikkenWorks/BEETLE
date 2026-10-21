/* RC1.0.11 SONG CHAINS: native codec migration, lane isolation, row defaults. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "track_project_store.h"
#include "track_master_fx.h"
#include "beetle_instrument_library.h"
static struct track_store_project p,q;
static uint8_t wire[TRACK_STORE_V4_MAX],old[TRACK_STORE_V4_MAX];
static void init(void){
 memset(&p,0,sizeof p);
 struct track_engine e;track_init(&e);track_song_init(&p.song,&e);
 track_mixer_init(&p.mixer);track_master_default(&p.master);
 for(int i=0;i<4;i++){p.instruments[i].volume=65;p.instruments[i].duty=50;p.instruments[i].cutoff=7000;p.instruments[i].type=i;track_va_patch_default(&p.instruments[i].va);}
 for(int i=0;i<TRACK_DRUM_COUNT;i++)track_drum_timbre_default(&p.drum[i]);
 beetle_library_init(&p.library);p.bpm=120;p.slot=0;
 memcpy(p.title,"CHAINS TEST ",12);
}
int main(void){
 struct track_engine e;track_init(&e);init();
 assert(p.song.chain[0].pattern==0u&&p.song.chain[0].lane[5]==0u);
 assert(track_song_insert(&p.song,0)==0);
 assert(p.song.chain[1].pattern==1u&&p.song.chain[1].repeats==1u);
 for(int c=0;c<6;c++)assert(p.song.chain[1].lane[c]==1u);
 assert(track_song_change(&p.song,1,5,-1)==0);assert(p.song.chain[1].lane[5]==0u);
 assert(track_song_change(&p.song,1,0,-2)==0);assert(p.song.chain[1].lane[0]==TRACK_SONG_SILENT);
 assert(track_song_change(&p.song,1,6,3)==0&&p.song.chain[1].repeats==4u);
 p.song.chain[0].lane[0]=2u;p.song.chain[0].lane[5]=3u;
 p.song.patterns[2].ch[0][0].note=60u;
 p.song.patterns[3].ch[5][0].note=2u;
 p.song.patterns[3].ch[5][0].chord=4u;
 p.song.patterns[3].note2_fx[0].effect=4u;p.song.patterns[3].note2_fx[0].value=73u;
 p.song.patterns[3].length=4u;
 p.song.enabled=1u;struct track_pattern before=p.song.patterns[2];
 e.active.ch[0][0].note=70u;e.pattern_slot=0u;
 track_song_start(&p.song,&e);
 assert(e.active.ch[0][0].note==60u&&e.active.ch[5][0].note==2u);
 assert(e.active.ch[5][0].chord==4u&&e.active.note2_fx[0].effect==4u);
 assert(e.active.ch[5][4].note==2u&&e.active.note2_fx[4].value==73u);
 assert(memcmp(&before,&p.song.patterns[2],sizeof before)==0);
 assert(track_song_on_wrap(&p.song,&e)==1&&p.song.position==1);
 assert(e.active.ch[0][0].note==TRACK_NOTE_EMPTY); /* CH1 -- */
 track_song_restore(&p.song,&e);
 assert(e.active.ch[0][0].note==70u&&p.song.patterns[0].ch[0][0].note==70u);
 uint32_t n=0;assert(track_store_encode(&p,wire,sizeof wire,&n)==0);
 assert(wire[4]==19u&&track_store_decode(&q,wire,n)==0);
 assert(q.song.chain[0].lane[0]==2u&&q.song.chain[0].lane[5]==3u);
 assert(q.song.chain[1].lane[0]==255u&&q.song.chain[1].repeats==4u);
 assert(q.song.patterns[3].note2_fx[0].value==73u);
 /* Construct a byte-accurate historical V18 image by dropping only V19 lane refs. */
 uint32_t head=24+12+1+3+2; /* title,slot,harmony,bpm,count/flags */
 assert(wire[head]==2u); /* song count */
 uint32_t songat=head+3;
 memcpy(old,wire,songat);
 for(unsigned r=0;r<p.song.count;r++)memcpy(old+songat+2*r,wire+songat+8*r,2);
 uint32_t oldend=songat+2*p.song.count;
 memcpy(old+oldend,wire+songat+8*p.song.count,n-(songat+8*p.song.count));
 uint32_t oldn=n-6*p.song.count;
 old[4]=18;old[8]=(uint8_t)oldn;old[9]=(uint8_t)(oldn>>8);old[10]=(uint8_t)(oldn>>16);old[11]=(uint8_t)(oldn>>24);
 uint32_t c=track_store_crc32(old+24,oldn-24);for(unsigned j=0;j<4;j++)old[12+j]=(uint8_t)(c>>(j*8));
 c=track_store_crc32(old,16);for(unsigned j=0;j<4;j++)old[16+j]=(uint8_t)(c>>(j*8));
 assert(track_store_decode(&q,old,oldn)==0);
 for(unsigned r=0;r<p.song.count;r++)for(int ch=0;ch<6;ch++)assert(q.song.chain[r].lane[ch]==q.song.chain[r].pattern);
 puts("PASS V19 song chains: row defaults, mixed lanes, --, NOTE2 FX, repeat, source isolation, V18 migration");
 return 0;
}
