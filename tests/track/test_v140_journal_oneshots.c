/* SPDX-License-Identifier: GPL-3.0-only
 * BEETLE V1.4.0 native V23 one-shot A/B flash journal + interrupted-write rollback test.
 * Host simulated NOR; physical FM1 flash NOT accessed. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "track_journal.h"
#include "track_master_fx.h"
#include "beetle_instrument_library.h"
static uint8_t disk[2][TRACK_JOURNAL_BANK_BYTES];
static uint8_t scratch[TRACK_STORE_MAX];
static uint8_t data0[BEETLE_ONESHOT_FIRST_BYTES],data1[BEETLE_ONESHOT_SECOND_BYTES];
static uint8_t check0[BEETLE_ONESHOT_FIRST_BYTES],check1[BEETLE_ONESHOT_SECOND_BYTES];
static struct track_store_project p,q;
static int write_calls,fail_at=-1;
static int rd(void*ctx,uint8_t s,uint8_t b,uint32_t pos,uint8_t*dst,uint32_t n){
 (void)ctx;if(s||b>=2||pos>TRACK_JOURNAL_BANK_BYTES||n>TRACK_JOURNAL_BANK_BYTES-pos)return -1;
 memcpy(dst,disk[b]+pos,n);return 0;
}
static int er(void*ctx,uint8_t s,uint8_t b){(void)ctx;if(s||b>=2)return -1;memset(disk[b],255,TRACK_JOURNAL_BANK_BYTES);return 0;}
static int wr(void*ctx,uint8_t s,uint8_t b,uint32_t pos,const uint8_t*src,uint32_t n){
 (void)ctx;if(s||b>=2||pos>TRACK_JOURNAL_BANK_BYTES||n>TRACK_JOURNAL_BANK_BYTES-pos)return -1;
 if(fail_at>=0 && write_calls++>=fail_at)return -1;
 for(uint32_t i=0;i<n;i++){if((disk[b][pos+i]&src[i])!=src[i])return -1;disk[b][pos+i]&=src[i];}
 return 0;
}
static struct track_journal_io io={0,rd,er,wr};
int main(void){
 memset(disk,255,sizeof disk);
 struct track_engine e;track_init(&e);track_song_init(&p.song,&e);track_mixer_init(&p.mixer);track_master_default(&p.master);beetle_library_init(&p.library);
 p.slot=0;p.bpm=120;for(int i=0;i<8;i++)track_va_patch_default(&p.instruments[i].va);
 for(int i=0;i<TRACK_DRUM_COUNT;i++)track_drum_component_default(&p.drum[i],i);
 for(int i=0;i<5;i++)p.punch_param[i]=50;
 p.oneshots.first=data0;p.oneshots.second=data1;q.oneshots.first=check0;q.oneshots.second=check1;
 p.oneshots.length[0]=1700;p.oneshots.length[5]=1000;memcpy(p.oneshots.name[0],"SNAP",4);
 p.song.patterns[0].ch[TRACK_DRUM_CHANNEL][0].note=14;
 for(int i=0;i<2700;i++)beetle_oneshot_set(&p.oneshots,i,(uint8_t)(i*17u+43u));
 assert(track_journal_save(&io,0,&p,scratch,sizeof scratch,0)==TRACK_JOURNAL_OK);
 assert(track_journal_load(&io,0,&q,scratch,sizeof scratch,0)==TRACK_JOURNAL_OK);
 assert(q.oneshots.length[0]==1700&&q.oneshots.length[5]==1000);
 for(int i=0;i<2700;i++)assert(beetle_oneshot_get(&q.oneshots,i)==(uint8_t)(i*17u+43u));
 assert(q.song.patterns[0].ch[TRACK_DRUM_CHANNEL][0].note==14);
 // Power loss during the alternate-bank write must preserve the existing project.
 p.oneshots.length[0]=3500;p.oneshots.length[5]=1000;p.bpm=161;
 for(int i=0;i<4500;i++)beetle_oneshot_set(&p.oneshots,i,(uint8_t)(255u-i));
 fail_at=2;write_calls=0;
 assert(track_journal_save(&io,0,&p,scratch,sizeof scratch,0)!=TRACK_JOURNAL_OK);
 fail_at=-1;
 assert(track_journal_load(&io,0,&q,scratch,sizeof scratch,0)==TRACK_JOURNAL_OK);
 assert(q.bpm==120&&q.oneshots.length[0]==1700&&q.oneshots.length[5]==1000);
 assert(track_journal_save(&io,0,&p,scratch,sizeof scratch,0)==TRACK_JOURNAL_OK);
 assert(track_journal_load(&io,0,&q,scratch,sizeof scratch,0)==TRACK_JOURNAL_OK);
 assert(q.bpm==161&&q.oneshots.length[0]==3500&&q.oneshots.length[5]==1000);
 puts("PASS V1.4.0 sample bank A/B save, reload, interrupted write recovery");return 0;
}
