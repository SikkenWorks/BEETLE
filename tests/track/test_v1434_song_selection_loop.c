/* SPDX-License-Identifier: GPL-3.0-only
 * V1.4.34: transient SONG range playback must not rewrite song chain.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "track_song.h"
static struct track_song_bank b;
static struct track_engine e;
static struct track_song_entry original[TRACK_SONG_ENTRIES];
int main(void){
 track_init(&e);track_song_init(&b,&e);
 b.count=6u;b.enabled=1u;
 for(unsigned r=0;r<b.count;r++){
  b.chain[r].pattern=(uint8_t)r;
  b.chain[r].repeats=(uint8_t)(r==3u?3u:1u);
  for(unsigned c=0;c<TRACK_CHANNELS;c++)b.chain[r].lane[c]=(uint8_t)r;
  b.patterns[r].ch[0][0].note=(uint8_t)(60u+r);
 }
 memcpy(original,b.chain,sizeof(original));
 b.loop=0u;
 track_song_start_range(&b,&e,4u,2u); /* reversed 5th -> 3rd */
 assert(b.play_range_active && b.play_range_first==2u && b.play_range_last==4u);
 assert(b.position==2u && b.repeat_index==0u && e.active.ch[0][0].note==62u);
 assert(track_song_on_wrap(&b,&e)==1&&b.position==3u&&b.repeat_index==0u);
 assert(track_song_on_wrap(&b,&e)==0&&b.position==3u&&b.repeat_index==1u);
 assert(track_song_on_wrap(&b,&e)==0&&b.position==3u&&b.repeat_index==2u);
 assert(track_song_on_wrap(&b,&e)==1&&b.position==4u&&b.repeat_index==0u);
 assert(track_song_on_wrap(&b,&e)==1&&b.position==2u&&b.repeat_index==0u);
 for(unsigned i=0;i<12;i++)assert(track_song_on_wrap(&b,&e)>=0);
 assert(memcmp(original,b.chain,sizeof(original))==0);
 track_song_restore(&b,&e);
 assert(!b.play_range_active&&!b.composite_active);
 track_song_start_range(&b,&e,5u,5u); /* selected single row repeats indefinitely */
 for(unsigned i=0;i<24;i++)assert(track_song_on_wrap(&b,&e)==1&&b.position==5u);
 track_song_restore(&b,&e);
 track_song_start_range(&b,&e,55u,60u); /* corrupted input bounded */
 assert(b.position==5u&&b.play_range_first==5u&&b.play_range_last==5u);
 track_song_restore(&b,&e);
 track_song_start(&b,&e); /* regular song honors global LOOP OFF */
 for(unsigned i=0;i<5;i++){
  while(i==3u&&b.repeat_index<2u)assert(track_song_on_wrap(&b,&e)==0);
  assert(track_song_on_wrap(&b,&e)==1);
 }
 assert(track_song_on_wrap(&b,&e)==-1&&!e.transport.playing);
 b.loop=1u;track_song_start(&b,&e);
 for(unsigned i=0;i<5;i++){
  while(i==3u&&b.repeat_index<2u)assert(track_song_on_wrap(&b,&e)==0);
  assert(track_song_on_wrap(&b,&e)==1);
 }
 assert(track_song_on_wrap(&b,&e)==1&&b.position==0u);
 assert(!b.play_range_active);
 assert(memcmp(original,b.chain,sizeof(original))==0);
 puts("PASS V1.4.34: selected bounds, reversed, 64 loops, repeats, single row, stops, stored chain unchanged");
 return 0;
}
