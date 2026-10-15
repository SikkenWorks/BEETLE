/* BEETLE V0.86: Live Mixer, project compatibility and 120% gain. GPL-3.0-only. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "track_mixer.h"
#include "track_project_store.h"
#include "track_master_fx.h"
#include "beetle_quantized_mute.h"
static struct track_store_project project,loaded;
static uint8_t packed[TRACK_STORE_MAX];
int main(void){
 struct track_mixer m;track_mixer_init(&m);
 for(int c=0;c<TRACK_SYNTH_CHANNELS;c++)assert(m.ch[c].volume==50);
 assert(m.ch[TRACK_DRUM_CHANNEL].volume==80);
 assert(m.quantize_mutes==0);
 for(int c=0;c<TRACK_CHANNELS;c++){
   m.col=0;m.row=(uint8_t)c;track_mixer_change(&m,10000);
   assert(m.ch[c].volume==120);
   assert(track_mixer_velocity(&m,(uint8_t)c,100u)==120u);
   track_mixer_change(&m,-10000);assert(m.ch[c].volume==0);
 }
 struct beetle_mute_queue q={0};
 track_mixer_init(&m);
 /* Mimic Live Mixer key 1 (CH1) and key 6 (CH6) while PLAY: always
  * queue even though the global preference is OFF. */
 assert(m.quantize_mutes==0);
 assert(beetle_mute_toggle(&q,&m,0u));
 assert(beetle_mute_toggle(&q,&m,5u));
 assert(q.pending==((1u<<0)|(1u<<5)));
 assert(!m.ch[0].mute && !m.ch[5].mute);
 assert(beetle_mute_commit(&q,&m));
 assert(m.ch[0].mute && m.ch[5].mute && !q.pending);
 assert(beetle_mute_toggle(&q,&m,0u));
 assert(beetle_mute_toggle(&q,&m,0u)); /* cancel pending when toggled twice */
 assert(!q.pending && m.ch[0].mute);
 struct track_engine engine;track_init(&engine);
 track_song_init(&project.song,&engine);
 track_mixer_init(&project.mixer);track_master_default(&project.master);
 for(int c=0;c<4;c++){project.instruments[c].volume=65;track_va_patch_default(&project.instruments[c].va);}
 for(int c=0;c<TRACK_DRUM_COUNT;c++)track_drum_timbre_default(&project.drum[c]);
 project.bpm=120;project.slot=1;
 project.mixer.ch[0].volume=115;project.mixer.ch[5].volume=120;
 uint32_t used=0;
 assert(track_store_encode(&project,packed,sizeof(packed),&used)==0);
 assert(track_store_decode(&loaded,packed,used)==0);
 assert(loaded.mixer.ch[0].volume==115 && loaded.mixer.ch[5].volume==120);
 assert(loaded.mixer.quantize_mutes==0);
 project.mixer.ch[0].volume=121;
 assert(track_store_encode(&project,packed,sizeof(packed),&used)!=0);
 puts("PASS V0.86: 5 synth defaults, drum volume, 120%, saved volumes, Q-off, Live Mixer six-lane queue");
 return 0;
}
