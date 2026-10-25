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
 struct track_engine e;track_init(&e);init();for(unsigned i=0;i<13;i++)p.punch_param[i]=(uint8_t)(20u+i);
 assert(p.song.chain[0].pattern==0u&&p.song.chain[0].lane[5]==TRACK_SONG_AUTO_BASE);
 assert(track_song_insert(&p.song,0)==0);
 assert(p.song.chain[1].pattern==0u&&p.song.chain[1].repeats==1u);
 for(int c=0;c<6;c++)assert(p.song.chain[1].lane[c]==TRACK_SONG_AUTO_BASE);
 /* Old explicit lane-editing workflow remains supported after cloning. */
 p.song.chain[1].lane[5]=1u;
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
 assert(wire[4]==TRACK_STORE_VERSION&&track_store_decode(&q,wire,n)==0);
 assert(q.song.chain[0].lane[0]==2u&&q.song.chain[0].lane[5]==3u);
 assert(q.song.chain[1].lane[0]==255u&&q.song.chain[1].repeats==4u);
 assert(q.song.patterns[3].note2_fx[0].value==73u);
 /* Legacy fixture was authored by the prior V25 encoder. V26 changes
  * both the project flag layout and 48 sparse NOTE3 masks, so a V26 byte
  * stream cannot become V23 by merely deleting eight Punch-In fields. */
 FILE *legacy_file=fopen("tests/track/fixtures/valid_v23_song_chain.beetle","rb");
 assert(legacy_file);
 size_t oldn=fread(old,1,sizeof(old),legacy_file);fclose(legacy_file);
 assert(oldn>64 && old[4]==23);
 assert(track_store_decode(&q,old,(uint32_t)oldn)==0);
 assert(q.drum_note3_enabled==0);
 for(unsigned pat=0;pat<TRACK_PATTERNS;pat++)for(unsigned row=0;row<TRACK_STEPS;row++)
  assert(q.song.patterns[pat].note3[row]==TRACK_NOTE_EMPTY);
 puts("PASS V24 song chains: row clone, mixed lanes, --, NOTE2 FX, repeat, source isolation, V23 read");
 return 0;
}
