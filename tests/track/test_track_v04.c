/* SPDX-License-Identifier: GPL-3.0-only; native music model regression tests */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "track_core.h"
#include "track_song.h"
static struct track_engine engine;
static struct track_song_bank bank;
int main(void){
 track_init(&engine);track_song_init(&bank,&engine);
 assert(bank.count==1u&&bank.chain[0].pattern==0u);
 assert(bank.patterns[0].ch[0][0].note==255u);
 assert(track_song_switch(&bank,&engine,48u)==-1);
 assert(track_song_switch(&bank,&engine,47u)==0);
 assert(engine.pattern_slot==47u);
 assert(track_set_note(&engine,2u,15u,75u)==0);
 assert(track_song_switch(&bank,&engine,0u)==0);
 assert(track_song_switch(&bank,&engine,47u)==0);
 assert(engine.active.ch[2][15].note==75u); // 48th pattern survives switch
 assert(track_song_switch(&bank,&engine,0u)==0);
 assert(track_song_change(&bank,0,0,1000)==0&&bank.chain[0].pattern==47u);
 assert(track_song_change(&bank,0,1,1000)==0&&bank.chain[0].repeats==16u);
 assert(track_song_change(&bank,0,1,-1000)==0&&bank.chain[0].repeats==1u);
 assert(track_song_change(&bank,0,0,-1000)==0&&bank.chain[0].pattern==0u);
 assert(track_song_insert(&bank,0u)==0&&bank.count==2u);
 bank.chain[0]=(struct track_song_entry){0u,2u};
 bank.chain[1]=(struct track_song_entry){47u,1u};
 bank.enabled=1u;bank.loop=0u;
 track_song_start(&bank,&engine);
 assert(engine.pattern_slot==0u&&bank.position==0u);
 track_play(&engine);
 assert(track_song_on_wrap(&bank,&engine)==0); // second repeat of first pattern
 assert(bank.position==0u&&bank.repeat_index==1u);
 assert(track_song_on_wrap(&bank,&engine)==1); // second pattern
 assert(bank.position==1u&&engine.pattern_slot==47u);
 assert(engine.active.ch[2][15].note==75u);
 assert(track_song_on_wrap(&bank,&engine)==-1); // no loop => stop
 assert(!engine.transport.playing);
 bank.loop=1u;track_song_start(&bank,&engine);track_play(&engine);
 assert(track_song_on_wrap(&bank,&engine)==0);
 assert(track_song_on_wrap(&bank,&engine)==1);
 assert(track_song_on_wrap(&bank,&engine)==1&&bank.position==0u);
 assert(track_song_delete(&bank,1u)==0&&bank.count==1u);
 assert(track_song_delete(&bank,0u)==-1);
 assert(track_song_record(&bank,0u,0u,2u,1u)==0);
 assert(track_song_record(&bank,0u,3u,7u,1u)==0);
 assert(track_song_record(&bank,0u,6u,2u,0u)==0);
 assert(track_song_effect_mask(&bank,0u,0u)==(1u<<2));
 assert(track_song_effect_mask(&bank,0u,4u)==((1u<<2)|(1u<<7)));
 assert(track_song_effect_mask(&bank,0u,7u)==(1u<<7));
 assert(track_song_effect_mask(&bank,1u,7u)==0u); // isolated patterns
 assert(track_song_record(&bank,48u,0u,0u,1u)==-1);
 assert(track_song_record(&bank,0u,16u,0u,1u)==-1);
 assert(track_song_record(&bank,0u,0u,8u,1u)==-1);
 struct track_song_clip *c=&bank.clips[0];
 while(c->count<TRACK_SONG_FX_EVENTS)assert(track_song_record(&bank,0u,0u,0u,1u)==0);
 assert(track_song_record(&bank,0u,0u,0u,0u)==-2&&c->overflow==1u);
 while(bank.count<TRACK_SONG_ENTRIES)assert(track_song_insert(&bank,bank.count-1u)==0);
 assert(track_song_insert(&bank,bank.count-1u)==-1&&bank.count==64u);
 printf("TRACK V0.4: 48 RAM patterns, song repeats/loop, Punch-In replay/bounds PASS (%zu bytes bank)\n",sizeof(bank));
 return 0;
}
