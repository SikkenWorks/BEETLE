/* BEETLE V1.3.1: historical V19 import and V20 eight-slot encode/decode. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "track_project_store.h"
#include "track_master_fx.h"
#ifndef TRACK_CHIP_SLOTS
#define TRACK_CHIP_SLOTS 4u
#endif

static struct track_store_project before,after;
static uint8_t raw[TRACK_STORE_V4_MAX];

static void init_project(void){
 memset(&before,0,sizeof(before));
 static struct track_engine engine;
 track_init(&engine);track_song_init(&before.song,&engine);
 track_mixer_init(&before.mixer);track_master_default(&before.master);
 for(int i=0;i<TRACK_CHIP_SLOTS;i++){
   struct track_store_instrument *p=&before.instruments[i];
   p->volume=65;p->duty=50;p->cutoff=7000;p->type=(uint8_t)(i%TRACK_CHIP_TYPES);
   track_va_patch_default(&p->va);
 }
 for(int i=0;i<TRACK_DRUM_COUNT;i++)track_drum_timbre_default(&before.drum[i]);
 before.bpm=120;before.slot=0;memcpy(before.title,"8 PATCH TEST",12);
 for(int i=0;i<5;i++)before.punch_param[i]=50;
 before.song.count=64;
 for(int i=0;i<64;i++){
   before.song.chain[i].pattern=i%48;before.song.chain[i].repeats=1;
   for(int ch=0;ch<6;ch++)before.song.chain[i].lane[ch]=i%48;
 }
 before.song.patterns[0].ch[0][0].note=60u;
 before.song.patterns[0].ch[0][0].instrument=3u;
 before.song.patterns[0].ch[4][1].note=64u;
#ifdef BEETLE_V131_GENERATE
 before.song.patterns[0].ch[4][1].instrument=0u;
#else
 before.song.patterns[0].ch[4][1].instrument=7u;
 before.song.patterns[0].ch[1][2].note=67u;
 before.song.patterns[0].ch[1][2].instrument=6u;
 before.song.patterns[0].ch[5][0].note=1u;
 before.song.patterns[0].ch[5][0].instrument=3u;
 for(int i=0;i<TRACK_CHIP_SLOTS;i++){
    before.instruments[i].type=(uint8_t)(i==4?TRACK_CHIP_VA_OSC:i==5?TRACK_CHIP_VA_OSC2:i==6?TRACK_CHIP_SID_6581:i==7?TRACK_CHIP_NES_PULSE:i);
    before.instruments[i].cutoff=(uint16_t)(6000+i*400);
 }
#endif
}
int main(int argc,char**argv){
 if(argc<2||argc>3){fprintf(stderr,"expected output fixture or historical fixture path\n");return 2;}
#ifdef BEETLE_V131_GENERATE
 init_project();uint32_t n=0;assert(track_store_encode(&before,raw,sizeof(raw),&n)==0);
 assert(raw[4]==19u);FILE *f=fopen(argv[1],"wb");assert(f);
 assert(fwrite(raw,1,n,f)==n);fclose(f);
 printf("PASS: produced untouched historical V19 project %u bytes\n",(unsigned)n);
#else
 init_project();
 uint32_t n=0;assert(track_store_encode(&before,raw,sizeof(raw),&n)==0);
 assert(raw[4]==TRACK_STORE_VERSION);assert(track_store_decode(&after,raw,n)==0);
 if(argc==3){FILE *out=fopen(argv[2],"wb");assert(out);assert(fwrite(raw,1,n,out)==n);fclose(out);}
 assert(after.song.count==64u);
 for(int i=0;i<8;i++){
   assert(after.instruments[i].type==before.instruments[i].type);
   assert(after.instruments[i].cutoff==before.instruments[i].cutoff);
 }
 assert(after.song.patterns[0].ch[4][1].instrument==7u);
 assert(after.song.patterns[0].ch[1][2].instrument==6u);
 assert(after.song.patterns[0].ch[5][0].note==1u);
 /* Standalone pattern transfer uses full-byte patch IDs, unlike old 2-bit TRK1. */
 uint8_t patternwire[TRACK_PATTERN_BYTES];struct track_pattern recovered;
 int patternbytes=track_pattern_encode(&after.song.patterns[0],patternwire,sizeof(patternwire));
 assert(patternbytes==(int)sizeof(patternwire));
 assert(track_pattern_decode(&recovered,patternwire,sizeof(patternwire))==0);
 assert(recovered.ch[4][1].instrument==7u && recovered.ch[1][2].instrument==6u);
 assert(recovered.ch[5][0].instrument==3u);
 /* A blank project with five synth channels must not explode packed mask usage. */
 before.song.patterns[0].ch[4][1].note=TRACK_NOTE_EMPTY;
 before.song.patterns[0].ch[4][1].instrument=4u;
 assert(track_store_encode(&before,raw,sizeof(raw),&n)==0);
 FILE *f=fopen(argv[1],"rb");assert(f);assert(fseek(f,0,SEEK_END)==0);
 long sz=ftell(f);assert(sz>=42&&sz<=TRACK_STORE_V4_MAX);
 assert(fseek(f,0,SEEK_SET)==0);assert(fread(raw,1,(size_t)sz,f)==(size_t)sz);fclose(f);
 assert(raw[4]==19u);
 assert(track_store_decode(&after,raw,(uint32_t)sz)==0);
 assert(after.song.count==64u);
 assert(after.song.patterns[0].ch[0][0].instrument==3u);
 assert(after.song.patterns[0].ch[4][1].instrument==0u);
 assert(after.instruments[0].type==0u);
 assert(after.instruments[3].type==3u);
 assert(after.instruments[4].type==TRACK_CHIP_VA_OSC);
 assert(after.instruments[5].type==TRACK_CHIP_VA_OSC2);
 assert(after.instruments[6].type==TRACK_CHIP_SID_6581);
 assert(after.instruments[7].type==TRACK_CHIP_NES_PULSE);
 assert(track_store_encode(&after,raw,sizeof(raw),&n)==0 && raw[4]==TRACK_STORE_VERSION);
 assert(track_store_decode(&before,raw,n)==0);
 assert(before.instruments[7].type==TRACK_CHIP_NES_PULSE);
 puts("PASS V1.3.1: 8 patch slots; dual VA; 64-song; CH6 drum isolation; V19 migration to V20");
#endif
 return 0;
}
