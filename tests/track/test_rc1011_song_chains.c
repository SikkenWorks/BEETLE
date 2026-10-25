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
 /* V23 compatibility: V25 persists 13 Punch-In settings, whereas V23
  * persisted exactly five. A V25 wire file with only a rewritten version
  * byte is not a valid legacy fixture! Remove the eight V25-only settings
  * first. This test intentionally has no recorded Punch-In events, since
  * V25 introduced a separate down/status byte for those events. */
 uint32_t punch_at=0u;
 for(uint32_t i=24u;i+13u<=n;i++){
  unsigned match=1u;for(unsigned j=0u;j<13u;j++)if(wire[i+j]!=20u+j){match=0u;break;}
  if(match){assert(punch_at==0u);punch_at=i;}
 }
 assert(punch_at>24u);
 uint32_t legacy_n=n-8u;
 memcpy(old,wire,punch_at+5u);
 memcpy(old+punch_at+5u,wire+punch_at+13u,n-(punch_at+13u));
 /* Before V24, auto (128+) lane references did not exist. */
 uint32_t head=24+12+1+3+2;
 assert(old[head]==2u);
 uint32_t songat=head+3u;
 for(unsigned row=0;row<p.song.count;row++)for(unsigned ch=0;ch<6;ch++){
  uint32_t pos=songat+8u*row+2u+ch;
  if(old[pos]>=TRACK_SONG_AUTO_BASE&&old[pos]<TRACK_SONG_AUTO_BASE+TRACK_PATTERNS)
   old[pos]=(uint8_t)(old[pos]-TRACK_SONG_AUTO_BASE);
 }
 old[4]=23u;
 /* V23 includes an indexed legacy one-shot table and B1OS trailer. */
 uint32_t oldn=legacy_n+BEETLE_ONESHOT_NOTES*10u+6u;
 assert(oldn<=sizeof old);
 memset(old+legacy_n,0,BEETLE_ONESHOT_NOTES*10u);
 old[oldn-6u]=(uint8_t)legacy_n;
 old[oldn-5u]=(uint8_t)(legacy_n>>8u);
 memcpy(old+oldn-4u,"B1OS",4u);
 for(unsigned j=0;j<4;j++)old[8+j]=(uint8_t)(oldn>>(j*8));
 uint32_t c=track_store_crc32(old+24,oldn-24);
 for(unsigned j=0;j<4;j++)old[12+j]=(uint8_t)(c>>(j*8));
 c=track_store_crc32(old,16);
 for(unsigned j=0;j<4;j++)old[16+j]=(uint8_t)(c>>(j*8));
 assert(track_store_decode(&q,old,oldn)==0);
 for(unsigned row=0;row<p.song.count;row++)for(unsigned ch=0;ch<6;ch++){
  uint8_t lane=p.song.chain[row].lane[ch];
  uint8_t legacy=(lane>=TRACK_SONG_AUTO_BASE&&lane<TRACK_SONG_AUTO_BASE+TRACK_PATTERNS)?
                (uint8_t)(lane-TRACK_SONG_AUTO_BASE):lane;
  assert(q.song.chain[row].lane[ch]==legacy);
 }
 puts("PASS V24 song chains: row clone, mixed lanes, --, NOTE2 FX, repeat, source isolation, V23 read");
 return 0;
}
