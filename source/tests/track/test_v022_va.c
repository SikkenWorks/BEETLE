#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "track_chip.h"
#include "track_project_store.h"
#include "track_song.h"
#include "track_mixer.h"
static struct track_chip_engine engine;
static struct track_store_project project,loaded;
static uint8_t wire[TRACK_STORE_MAX];
static int16_t pcm[2048];
#define CHECK(x) do { if(!(x)){fprintf(stderr,"FAILED line %u: %s\n",(unsigned)__LINE__,#x);return 1;} }while(0)
static uint32_t renderhash(uint8_t type,uint8_t wave,uint8_t secondary,uint8_t mix,uint8_t morph,int8_t coarse){
 track_chip_init(&engine);engine.osc_type[0]=type;
 engine.patches[0].wave_a=wave;engine.patches[0].wave_b=secondary;
 engine.patches[0].mix=mix;engine.patches[0].morph=morph;
 engine.patches[0].coarse=coarse;
 track_chip_note(&engine,0,0,64,115);
 memset(pcm,0,sizeof pcm);track_chip_render(&engine,NULL,pcm,1024);
 uint32_t hash=2166136261u;
 for(unsigned i=0;i<2048;i++)hash=(hash^(uint16_t)pcm[i])*16777619u;
 return hash;
}
int main(void){
 uint32_t hashes[5];
 for(uint8_t w=0;w<5;w++){
  hashes[w]=renderhash(TRACK_CHIP_VA_OSC,w,1,50,0,0);
  CHECK(hashes[w]);for(uint8_t j=0;j<w;j++)CHECK(hashes[w]!=hashes[j]);
 }
 CHECK(renderhash(2,0,1,50,0,0)!=renderhash(2,0,1,50,100,0));
 CHECK(renderhash(3,0,3,10,0,0)!=renderhash(3,0,3,90,0,0));
 CHECK(renderhash(3,0,3,50,0,0)!=renderhash(3,0,3,50,0,12));
 track_chip_init(&engine);engine.patches[0].coarse=12;engine.patches[0].fine=50;engine.patches[0].detune=40;
 CHECK(track_chip_note(&engine,0,0,60,100));
 CHECK(engine.voices[0].step>0);CHECK(engine.voices[0].step2>engine.voices[0].step);
 for(int k=0;k<7;k++)CHECK(track_chip_note(&engine,0,0,61+k,100));
 CHECK(track_chip_note(&engine,0,0,80,100));CHECK(engine.voice_count==8&&engine.steal_count>0);
 engine.health_cookie=0;track_chip_render(&engine,NULL,pcm,32);
 CHECK(track_chip_health(&engine));CHECK(engine.patches[0].coarse==12);
 track_chip_init(&engine);engine.osc_type[0]=TRACK_CHIP_VA_OSC;
 engine.env_attack[0]=1u;engine.env_decay[0]=10u;engine.env_sustain[0]=60u;engine.env_release[0]=1u;
 CHECK(track_chip_note(&engine,0,0,60,100));
 track_chip_render(&engine,NULL,pcm,1024);
 CHECK(engine.voices[0].envelope>0u && engine.voices[0].envelope<=1048576u);
 track_chip_note_off(&engine,0,60);
 track_chip_render(&engine,NULL,pcm,1024);
 CHECK(engine.voices[0].active==0u);
 struct track_engine e;track_init(&e);track_song_init(&project.song,&e);track_mixer_init(&project.mixer);
 project.slot=2;project.bpm=128;
 for(unsigned c=0;c<4;c++){
  project.instruments[c].type=c;
  project.instruments[c].cutoff=7000;project.instruments[c].duty=50;
  project.instruments[c].volume=65;
  track_va_patch_default(&project.instruments[c].va);
 }
 struct track_va_patch *p=&project.instruments[2].va;
 *p=(struct track_va_patch){4,3,-12,-45,37,85,24,63};
 project.song.patterns[47].ch[0][15].note=61;
 uint32_t n=0;CHECK(track_store_encode(&project,wire,sizeof wire,&n)==0);
 CHECK(wire[4]==2);
 CHECK(track_store_decode(&loaded,wire,n)==0);
 CHECK(memcmp(&loaded.instruments[2].va,p,sizeof *p)==0);
 CHECK(loaded.song.patterns[47].ch[0][15].note==61);
 /* Migrate a genuine legacy v1 layout: strip v2 suffix, recompute CRC. */
 uint32_t legacy=n-4u*17u;
 wire[4]=1;wire[5]=0;wire[8]=(uint8_t)legacy;wire[9]=(uint8_t)(legacy>>8);
 wire[10]=(uint8_t)(legacy>>16);wire[11]=(uint8_t)(legacy>>24);
 uint32_t crc=track_store_crc32(wire+24,legacy-24);
 for(int j=0;j<4;j++)wire[12+j]=(uint8_t)(crc>>(j*8));
 crc=track_store_crc32(wire,16);
 for(int j=0;j<4;j++)wire[16+j]=(uint8_t)(crc>>(j*8));
 CHECK(track_store_decode(&loaded,wire,legacy)==0);
 CHECK(loaded.song.patterns[47].ch[0][15].note==61);
 CHECK(track_va_patch_valid(&loaded.instruments[2].va));
 CHECK(loaded.instruments[2].va.pulse_width==50);
 printf("PASS V0.22 five VA waveforms, morph, mix, tuning, 8-voice steal, DSP recovery, project v2 roundtrip and legacy v1 import (%u bytes)\n",n);
 return 0;
}
