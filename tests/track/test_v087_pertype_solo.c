/* GPL-3.0-only. BEETLE V0.87 live keys and waveform type bank. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "track_project_store.h"
#include "track_master_fx.h"
#include "track_ui_controls.h"
#include "beetle_punch_timing.h"
static struct track_store_project a,b;
static uint8_t encoded[TRACK_STORE_MAX];
static void put32(uint8_t *b,uint32_t v){for(int i=0;i<4;i++)b[i]=(uint8_t)(v>>(8*i));}
static void seal(uint32_t size){put32(encoded+8,size);put32(encoded+12,track_store_crc32(encoded+24,size-24));put32(encoded+16,track_store_crc32(encoded,16));}
int main(void){
 for(uint8_t i=0;i<6;i++){
  const uint8_t black[6]={1,3,5,8,10,13};
  const uint8_t white[6]={0,2,4,7,9,12};
  assert(beetle_black_mute_lane(black[i])==i);
  assert(beetle_white_solo_lane(white[i])==i);
  assert(beetle_white_solo_lane(black[i])==-1);
 }
 assert(beetle_stutter_index(50)==3);
 assert(beetle_stutter_value(3)==50);
 struct track_engine e;track_init(&e);track_song_init(&a.song,&e);
 track_mixer_init(&a.mixer);track_master_default(&a.master);
 a.slot=0;a.bpm=120;a.punch_param[1]=20;a.punch_param[4]=50;
 for(int i=0;i<4;i++){
  a.instruments[i].type=i;a.instruments[i].volume=65;a.instruments[i].duty=50;
  a.instruments[i].cutoff=7000;track_va_patch_default(&a.instruments[i].va);
 }
 for(int i=0;i<TRACK_DRUM_COUNT;i++)track_drum_timbre_default(&a.drum[i]);
 /* One channel, two distinct TYPE-specific patches with independent ADSR. */
 a.type_valid[0]=(1u<<0)|(1u<<9);
 a.type_presets[0][0]=a.instruments[0];
 a.type_presets[0][0].attack=7;
 a.type_presets[0][9]=a.instruments[0];
 a.type_presets[0][9].type=9;
 a.type_presets[0][9].attack=92;
 a.type_presets[0][9].va.detune=44;
 uint32_t n=0;
 assert(track_store_encode(&a,encoded,sizeof encoded,&n)==0);
 assert(encoded[4]==17 && n<TRACK_STORE_V4_MAX);
 assert(track_store_decode(&b,encoded,n)==0);
 assert(b.type_presets[0][0].attack==7 && b.type_presets[0][9].attack==92);
 assert(b.type_presets[0][9].va.detune==44);
 assert(b.type_valid[0]==a.type_valid[0]);
 assert(b.punch_param[1]==20 && b.punch_param[4]==50);
 /* Old V16 projects (with no type bank) should seed their active TYPE. */
 a.type_valid[0]=0;a.type_valid[1]=0;a.type_valid[2]=0;a.type_valid[3]=0;
 assert(track_store_encode(&a,encoded,sizeof encoded,&n)==0);
 assert(n>=8);encoded[4]=16;seal(n-8);
 assert(track_store_decode(&b,encoded,n-8)==0);
 assert(b.type_valid[0]==(1u<<a.instruments[0].type));
 assert(b.type_presets[0][a.instruments[0].type].volume==a.instruments[0].volume);
 puts("PASS V0.87: mute/solo keys, Stutter X1, Ring 20, V17 per-type patch roundtrip, V16 migration");
 return 0;
}
