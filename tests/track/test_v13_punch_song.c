/* SPDX-License-Identifier: GPL-3.0-only. Native project Punch recording regression. */
#include "track_song.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 struct track_engine e;struct track_song_bank b;
 track_init(&e);track_song_init(&b,&e);
 assert(track_song_record_value(&b,2u,15u,0u,1u,47u)==0);
 /* Ordinary looping playback can carry. Fresh PLAY must never infer prior loop state. */
 assert(track_song_effect_mask(&b,2u,0u)&1u);
 assert(!(track_song_effect_mask_boundary(&b,2u,0u,0u)&1u));
 assert(track_song_record_value(&b,2u,5u,1u,1u,37u)==0);
 track_song_clear_punch_fx(&b,2u,0u);
 assert(b.clips[2].count==1u && b.clips[2].data[0].fx==1u);
 assert(!(track_song_effect_mask(&b,2u,0u)&1u));
 assert(track_song_record_value(&b,2u,1u,0u,1u,26u)==0);
 assert(track_song_record_value(&b,2u,1u,0u,0u,255u)==0);
 assert(track_song_effect_mask_boundary(&b,2u,1u,0u)&1u);
 assert(!(track_song_effect_mask(&b,2u,2u)&1u));
 /* Song uses CH1 pattern, NOT cursor pattern and NOT Song row index. */
 b.count=2u;b.enabled=1u;
 b.chain[0].lane[0]=7u;b.chain[0].repeats=1u;
 b.chain[1].lane[0]=9u;b.chain[1].repeats=1u;
 e.pattern_slot=0u;
 track_song_start(&b,&e);
 assert(track_song_punch_pattern(&b,&e)==7u);
 assert(track_song_record_value(&b,track_song_punch_pattern(&b,&e),0u,4u,1u,72u)==0);
 assert(b.clips[7].count==1u && b.clips[0].count==0u);
 assert(track_song_on_wrap(&b,&e)==1);
 assert(track_song_punch_pattern(&b,&e)==9u);
 assert(!(track_song_effect_mask_boundary(&b,9u,0u,0u)&(1u<<4)));
 assert(track_song_record_value(&b,9u,4u,2u,1u,60u)==0);
 assert(track_song_record_value(&b,9u,5u,2u,0u,255u)==0);
 assert(track_song_effect_mask(&b,9u,4u)==4u);
 assert(track_song_effect_mask(&b,9u,5u)==0u);
 printf("PASS V1.3: fresh transport boundary, Song CH1 source, per-FX re-take, short hits\n");
}
