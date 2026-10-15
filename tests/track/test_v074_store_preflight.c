/* BEETLE V0.74: CRC-correct-but-unloadable saves must be rejected before NOR I/O.
 * Simulated bank backend only, no physical flash access. SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "track_journal.h"
#include "track_master_fx.h"
#include "beetle_instrument_library.h"
static uint8_t flash[2][TRACK_JOURNAL_BANK_BYTES],scratch[TRACK_STORE_MAX];
static struct track_store_project p,loaded;
static uint32_t reads,erases,programs;
static int rd(void *ctx,uint8_t s,uint8_t b,uint32_t at,uint8_t *v,uint32_t n){
 (void)ctx;reads++;if(s||b>1||at>TRACK_JOURNAL_BANK_BYTES||n>TRACK_JOURNAL_BANK_BYTES-at)return -1;
 memcpy(v,flash[b]+at,n);return 0;
}
static int er(void *ctx,uint8_t s,uint8_t b){
 (void)ctx;if(s||b>1)return -1;erases++;memset(flash[b],0xff,sizeof(flash[b]));return 0;
}
static int wr(void *ctx,uint8_t s,uint8_t b,uint32_t at,const uint8_t *v,uint32_t n){
 (void)ctx;if(s||b>1||at>TRACK_JOURNAL_BANK_BYTES||n>TRACK_JOURNAL_BANK_BYTES-at)return -1;
 programs++;for(uint32_t k=0;k<n;k++){if((flash[b][at+k]&v[k])!=v[k])return -1;flash[b][at+k]&=v[k];}
 return 0;
}
static const struct track_journal_io io={0,rd,er,wr};
static void setup(void){
 struct track_engine e;track_init(&e);
 memset(&p,0,sizeof(p));track_song_init(&p.song,&e);
 track_mixer_init(&p.mixer);track_master_default(&p.master);
 for(uint8_t k=0;k<4;k++){
  p.instruments[k].type=k;p.instruments[k].duty=50;p.instruments[k].volume=65;
  p.instruments[k].cutoff=7000;track_va_patch_default(&p.instruments[k].va);
 }
 for(uint8_t k=0;k<TRACK_DRUM_COUNT;k++)track_drum_timbre_default(&p.drum[k]);
 beetle_library_init(&p.library);
 p.bpm=120;p.slot=0;p.mixer.quantize_mutes=1;memcpy(p.title,"RC TEST 074 ",12);
}
static void should_reject(const char *label){
 const uint32_t r=reads,e=erases,w=programs;
 uint8_t out[TRACK_STORE_MAX];uint32_t len=0;
 assert(track_store_encode(&p,out,sizeof(out),&len)!=0);
 assert(track_journal_save(&io,0,&p,scratch,sizeof(scratch),NULL)==TRACK_JOURNAL_CAPACITY);
 assert(reads==r && erases==e && programs==w);
 assert(track_journal_load(&io,0,&loaded,scratch,sizeof(scratch),NULL)==TRACK_JOURNAL_OK);
 assert(loaded.bpm==120u);
 printf("PASS invalid preflight: %s, no flash I/O on save\n",label);
}
int main(void){
 memset(flash,0xff,sizeof(flash));setup();
 assert(track_journal_save(&io,0,&p,scratch,sizeof(scratch),NULL)==TRACK_JOURNAL_OK);
 assert(track_journal_load(&io,0,&loaded,scratch,sizeof(scratch),NULL)==TRACK_JOURNAL_OK);
 assert(loaded.bpm==120u);
 p.bpm=0;should_reject("BPM underflow");p.bpm=301;should_reject("BPM overflow");p.bpm=120;
 p.harmony.root=12;should_reject("key root");p.harmony.root=0;
 p.harmony.mode=9;should_reject("mode");p.harmony.mode=0;
 p.harmony.keylock=3;should_reject("keylock");p.harmony.keylock=0;
 p.song.enabled=2;should_reject("song enabled");p.song.enabled=0;
 p.song.loop=2;should_reject("song loop");p.song.loop=1;
 p.song.chain[0].pattern=48;should_reject("song pattern");p.song.chain[0].pattern=0;
 p.song.chain[0].repeats=0;should_reject("song repeats low");p.song.chain[0].repeats=1;
 p.song.chain[0].repeats=17;should_reject("song repeats high");p.song.chain[0].repeats=1;
 p.song.patterns[47].length=0;should_reject("pattern length 0");p.song.patterns[47].length=16;
 p.mixer.ch[0].volume=121;should_reject("mixer volume");p.mixer.ch[0].volume=80;
 p.mixer.ch[0].pan=-101;should_reject("mixer pan");p.mixer.ch[0].pan=0;
 p.mixer.ch[0].solo=2;should_reject("mixer solo");p.mixer.ch[0].solo=0;
 p.mixer.ch[0].swing=101;should_reject("mixer swing");p.mixer.ch[0].swing=0;
 p.drum[0].gain=101;should_reject("drum gain");p.drum[0].gain=100;
 p.drum[0].pitch=40;should_reject("drum pitch");p.drum[0].pitch=0;
 p.master.eq_width[2]=255;should_reject("master EQ width");p.master.eq_width[2]=50;
 p.master.comp_ratio=101;should_reject("master compressor");p.master.comp_ratio=40;
 p.master.enabled[0]=2;should_reject("master enabled");p.master.enabled[0]=1;
 p.master.order[0]=1;should_reject("master duplicate FX");p.master.order[0]=0;
 p.master.order[0]=3;should_reject("master FX range");p.master.order[0]=0;
 p.mixer.quantize_mutes=2;should_reject("mute quantize");p.mixer.quantize_mutes=1;
 /* A valid edited project remains backward-compatible with V16. */
 p.bpm=135;
 assert(track_journal_save(&io,0,&p,scratch,sizeof(scratch),NULL)==TRACK_JOURNAL_OK);
 assert(track_journal_load(&io,0,&loaded,scratch,sizeof(scratch),NULL)==TRACK_JOURNAL_OK);
 assert(loaded.bpm==135u);
 /* Flip one bit in the committed successor; loading falls back to prior. */
 flash[1][TRACK_JOURNAL_HEADER_BYTES+40u]^=0x01u;
 assert(track_journal_load(&io,0,&loaded,scratch,sizeof(scratch),NULL)==TRACK_JOURNAL_OK);
 assert(loaded.bpm==120u);
 puts("PASS BEETLE V0.74: fail-closed save, unchanged old bank, V16 compatibility, CRC rollback");
 return 0;
}
