#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "track_chip.h"
#include "track_mixer.h"
#include "track_project_store.h"
#include "track_song.h"
static int errors=0;
#define OK(x) do{if(!(x)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);errors++;}}while(0)
static struct track_store_project data;
static struct track_store_project decoded;
static unsigned char wire[TRACK_STORE_MAX];
static struct track_chip_engine synth;
static int16_t pcm[512];
static uint32_t hash_audio(void){
 uint32_t hash=2166136261u;
 for(unsigned i=0;i<512;i++){hash=(hash^(uint16_t)pcm[i])*16777619u;}
 return hash;
}
int main(void){
 track_chip_init(&synth);
 OK(track_chip_health(&synth));
 uint32_t hashes[4];
 for(unsigned t=0;t<4;t++){
  track_chip_init(&synth);synth.osc_type[0]=t;
  OK(track_chip_note(&synth,0,0,65,100));
  track_chip_render(&synth,NULL,pcm,256);
  hashes[t]=hash_audio();
  OK(hashes[t]!=0u);
  for(unsigned u=0;u<t;u++)OK(hashes[t]!=hashes[u]);
 }
 track_chip_init(&synth);
 for(unsigned k=0;k<8;k++)OK(track_chip_note(&synth,0,0,60+k,90));
 OK(synth.voice_count==8u);
 OK(track_chip_note(&synth,0,0,84,110));
 OK(synth.steal_count==1u&&synth.voice_count==8u);
 OK(synth.voices[synth.last_voice_index].note==84u);
 track_chip_note_off(&synth,0,84u);
 OK(synth.voices[synth.last_voice_index].released==1u);
 OK(track_chip_note(&synth,0,0,90,100));
 OK(synth.voice_count==8u&&synth.steal_count==2u);
 track_chip_set_mono(&synth,1u);
 OK(synth.voice_count==1u&&synth.mono==1u);
 synth.health_cookie=0u;
 track_chip_render(&synth,NULL,pcm,128);
 OK(track_chip_health(&synth)&&synth.restart_count==1u);
 // Host roundtrip: track-only format, 48 full patterns, drums & FX events.
 struct track_engine engine;track_init(&engine);
 track_song_init(&data.song,&engine);
 track_mixer_init(&data.mixer);
 memcpy(data.title,"TEST PROJ",10);
 data.slot=3;data.bpm=123;data.harmony.root=2;data.harmony.mode=1;data.harmony.keylock=2;
 data.song.patterns[47].ch[3][15].note=0;
 data.song.patterns[47].ch[3][15].chord=3;
 data.song.patterns[47].ch[0][8].note=69;
 data.song.patterns[47].ch[0][8].effect=2;
 data.song.clips[47].data[0]=(struct track_song_event){8u,2u,1u};data.song.clips[47].count=1;
 data.song.chain[0].pattern=47u;data.song.chain[0].repeats=4u;
 uint32_t size=0;
 OK(track_store_encode(&data,wire,sizeof(wire),&size)==0);
 OK(size>15000u&&size<TRACK_STORE_MAX);
 OK(track_store_decode(&decoded,wire,size)==0);
 OK(decoded.slot==3u&&decoded.bpm==123u&&decoded.song.chain[0].pattern==47u);
 OK(decoded.song.patterns[47].ch[3][15].note==0u);
 OK(decoded.song.patterns[47].ch[3][15].chord==3u);
 OK(decoded.song.patterns[47].ch[0][8].note==69u);
 OK(decoded.song.clips[47].data[0].fx==2u);
 wire[57]^=1u;
 OK(track_store_decode(&decoded,wire,size)!=0);
 unsigned char blank[4096];memset(blank,0xFF,sizeof(blank));
 OK(track_store_can_claim(blank,sizeof(blank)));
 blank[0]='F';blank[1]='E';blank[2]='L';blank[3]='U';
 OK(!track_store_can_claim(blank,sizeof(blank)));
 if(errors){fprintf(stderr,"FAIL: %d errors\n",errors);return 1;}
 printf("PASS 4 distinct oscillator renders, eight-voice advanced allocation, mono, DSP auto-recover, TRACK-only project CRC roundtrip (%u bytes), reject foreign sectors\n",size);
 return 0;
}
