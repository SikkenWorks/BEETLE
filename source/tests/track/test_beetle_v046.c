/* BEETLE V0.46: drum source gain, recorded ALGORITHM curves and stable tail headroom. */
#include "track_song.h"
#include "track_mixer.h"
#include "track_chip.h"
#include "track_drum.h"
#include "track_project_store.h"
#include "track_master_fx.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static struct track_store_project project,decoded;
static struct track_chip_engine chip;
static uint8_t data[TRACK_STORE_MAX];
static int16_t pcm[4096u*2u];
static void test_punch(void){
 struct track_engine engine;track_init(&engine);
 track_song_init(&project.song,&engine);
 assert(track_song_record_value(&project.song,0u,1u,0u,1u,33u)==0);
 assert(track_song_record_value(&project.song,0u,3u,0u,2u,60u)==0);
 assert(track_song_record_value(&project.song,0u,3u,0u,2u,67u)==0);
 assert(project.song.clips[0].count==2u); /* duplicate motion in same row coalesces */
 assert(track_song_record_value(&project.song,0u,4u,0u,0u,255u)==0);
 assert(track_song_record_value(&project.song,0u,5u,4u,1u,23u)==0);
 assert((track_song_effect_mask(&project.song,0u,2u)&1u)==1u);
 assert((track_song_effect_mask(&project.song,0u,3u)&1u)==1u);
 assert((track_song_effect_mask(&project.song,0u,4u)&1u)==0u);
 assert(track_song_effect_parameter(&project.song,0u,2u,0u,50u)==33u);
 assert(track_song_effect_parameter(&project.song,0u,3u,0u,50u)==67u);
 assert(track_song_effect_parameter(&project.song,0u,5u,4u,50u)==23u);
 track_mixer_init(&project.mixer);track_master_default(&project.master);
 project.bpm=120u;project.slot=0u;
 for(unsigned c=0;c<4;c++){track_va_patch_default(&project.instruments[c].va);project.instruments[c].volume=65u;}
 for(unsigned d=0;d<TRACK_DRUM_COUNT;d++)track_drum_component_default(&project.drum[d],d);
 uint32_t used=0;
 assert(track_store_encode(&project,data,sizeof data,&used)==0);
 assert(data[4]==9u);
 assert(track_store_decode(&decoded,data,used)==0);
 assert(decoded.song.clips[0].count==4u);
 assert(decoded.song.clips[0].data[1].down==2u);
 assert(decoded.song.clips[0].data[1].param==67u);
 assert((track_song_effect_mask(&decoded.song,0u,4u)&1u)==0u);
 assert(track_song_effect_parameter(&decoded.song,0u,3u,0u,50u)==67u);
 /* V0.46 saves the actual user's setting, including manually chosen 100. */
 project.mixer.ch[TRACK_DRUM_CHANNEL].volume=100u;
 assert(track_store_encode(&project,data,sizeof data,&used)==0);
 assert(track_store_decode(&decoded,data,used)==0);
 assert(decoded.mixer.ch[TRACK_DRUM_CHANNEL].volume==100u);
 /* Construct a valid V8 empty-pattern fixture, using CRC recomputation. */
 struct track_engine old;track_init(&old);track_song_init(&project.song,&old);
 assert(track_store_encode(&project,data,sizeof data,&used)==0);
 data[4]=8u;
 uint32_t crc=track_store_crc32(data,16u);
 for(unsigned k=0;k<4u;k++)data[16u+k]=(uint8_t)(crc>>(k*8u));
 assert(track_store_decode(&decoded,data,used)==0);
 assert(decoded.mixer.ch[TRACK_DRUM_CHANNEL].volume==80u);
}
static void test_headroom(void){
 track_chip_init(&chip);
 struct track_mixer m;track_mixer_init(&m);
 chip.env_attack[0]=1u;chip.env_sustain[0]=100u;
 assert(track_chip_note(&chip,0u,0u,60u,100u));
 assert(track_chip_note(&chip,1u,0u,60u,100u));
 memset(pcm,0,sizeof pcm);track_chip_render(&chip,&m,pcm,4096u);
 assert(chip.phrase_max_voices==2u&&chip.phrase_div_q8==512u);
 /* Note 1 is released but remains audible during its ADSR tail. */
 track_chip_note_off(&chip,1u,60u);
 memset(pcm,0,sizeof pcm);track_chip_render(&chip,&m,pcm,4096u);
 assert(chip.phrase_div_q8==512u); /* never halve divisor during release */
 memset(pcm,0,sizeof pcm);track_chip_render(&chip,&m,pcm,4096u);
 assert(chip.phrase_div_q8==512u); /* surviving channel retains headroom */
 track_chip_stop_channel(&chip,0u);
 for(unsigned k=0;k<5;k++){memset(pcm,0,sizeof pcm);track_chip_render(&chip,&m,pcm,4096u);}
 assert(chip.phrase_max_voices==0u&&chip.phrase_div_q8==256u);
}
static void test_drum(void){
 struct track_mixer m;track_mixer_init(&m);
 assert(m.ch[TRACK_DRUM_CHANNEL].volume==80u);
 struct track_drum_engine d;track_drum_init(&d);
 assert(track_drum_trigger(&d,0u));
 memset(pcm,0,sizeof pcm);track_drum_render(&d,&m,pcm,4096u);
 int peak=0;
 for(unsigned f=0;f<4096u*2u;f++){int x=pcm[f];if(x<0)x=-x;if(x>peak)peak=x;}
 assert(peak>9000&&peak<32767);
}
int main(void){test_punch();test_headroom();test_drum();puts("BEETLE V0.46 PASS: 80% drum fader with boosted PCM, step ALGORITHM record/save/replay, no release gain jump");return 0;}
