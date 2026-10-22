/* SPDX-License-Identifier: GPL-3.0-only. Native host regression, no flash writes. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "track_song.h"
#include "track_drum.h"
#include "track_mixer.h"
#include "track_master_fx.h"
#include "beetle_instrument_library.h"
#include "track_project_store.h"
static struct track_store_project a,b;
static unsigned char wire[TRACK_STORE_MAX];
static void test_song(void){
 struct track_engine e;track_init(&e);track_song_init(&a.song,&e);
 struct track_song_bank *s=&a.song;
 assert(s->count==1u);
 for(int ch=0;ch<6;ch++){
  assert(s->chain[0].lane[ch]==128u);
  assert(!track_song_lane_visible(s,s->chain[0].lane[ch],(uint8_t)ch));
 }
 s->patterns[0].ch[0][0].note=60u;
 assert(track_song_lane_visible(s,s->chain[0].lane[0],0u));
 s->chain[0].repeats=3u;
 s->chain[0].lane[1]=255u;
 s->chain[0].lane[2]=3u; /* explicitly assigned empty P04 must sustain */
 assert(track_song_lane_visible(s,3u,2u));
 assert(track_song_insert(s,0u)==0);
 assert(s->count==2u);
 for(int ch=0;ch<6;ch++)assert(s->chain[1].lane[ch]==s->chain[0].lane[ch]);
 assert(s->chain[1].repeats==3u);
 s->enabled=1u;track_song_start(s,&e);
 assert((track_song_stop_mask(s)&(1u<<1))!=0);
 assert((track_song_stop_mask(s)&(1u<<2))==0); /* explicit empty */
 assert((track_song_stop_mask(s)&(1u<<3))!=0); /* auto empty */
 s->patterns[0].ch[3][1].note=64u;
 assert((track_song_stop_mask(s)&(1u<<3))==0); /* populated auto */
 s->chain[1].lane[2]=255u;
 assert(s->chain[0].lane[2]==3u); /* row copied by value, not alias */
 assert(track_song_change(s,1u,3u,1)==0); /* auto -> explicitly entered P02 */
 assert(s->chain[1].lane[3]==1u);
}
static void test_serialization(void){
 struct track_engine e;track_init(&e);
 /* Reinitialize project with valid factory values. */
 memset(&a,0,sizeof a);track_song_init(&a.song,&e);
 track_mixer_init(&a.mixer);track_master_default(&a.master);
 beetle_library_init(&a.library);a.bpm=120u;a.slot=0u;
 for(int i=0;i<TRACK_CHIP_SLOTS;i++)track_va_patch_default(&a.instruments[i].va);
 for(int i=0;i<TRACK_DRUM_COUNT;i++)track_drum_component_default(&a.drum[i],(uint8_t)i);
 for(int i=0;i<5;i++)a.punch_param[i]=50;
 a.song.chain[0].lane[0]=128u;
 a.song.chain[0].lane[1]=255u;
 a.song.chain[0].lane[2]=6u;
 uint32_t n=0;
 assert(track_store_encode(&a,wire,sizeof wire,&n)==0 && wire[4]==24u);
 assert(track_store_decode(&b,wire,n)==0);
 assert(b.song.chain[0].lane[0]==128u);
 assert(b.song.chain[0].lane[1]==255u);
 assert(b.song.chain[0].lane[2]==6u);
 wire[40]^=0x01u;assert(track_store_decode(&b,wire,n)==-3);wire[40]^=0x01u;
 /* No sample bank suffix in V24 is valid; decoding stays guarded by CRC. */
}
static unsigned long long render_drum(int dual){
 struct track_drum_engine d;track_drum_init(&d);
 struct track_mixer mix;track_mixer_init(&mix);
 unsigned long long checksum=1469598103934665603ULL;
 int16_t out[256];
 /* 128 loops of a dense 16-step kick/crash pattern; tails cross boundaries. */
 for(int loop=0;loop<128;loop++)for(int step=0;step<16;step++){
  track_drum_trigger_pair(&d,(uint8_t)((step%4)==0?0u:2u),dual?7u:255u);
  for(int burst=0;burst<12;burst++){
   memset(out,0,sizeof out);track_drum_render(&d,&mix,out,128u);
   for(unsigned i=0;i<256;i++){checksum^=(uint16_t)out[i];checksum*=1099511628211ULL;}
  }
  assert(d.trigger_serial>0);
 }
 return checksum;
}
int main(void){
 test_song();test_serialization();
 printf("DRUM SINGLE CRC %016llx\n",render_drum(0));
 printf("DRUM DOUBLE CRC %016llx\n",render_drum(1));
 puts("PASS Song -- STOP, explicit empty sustain, row copy, V24 CRC and 128 dense drum loops");
 return 0;
}
