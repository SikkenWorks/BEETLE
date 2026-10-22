/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "beetle_mono_audition.h"
#include "track_song.h"
#include "track_chip.h"
#include "track_mixer.h"
static uint8_t test_priority[27],test_release_age[27];
static uint8_t kind(struct beetle_mono_audition *m,uint32_t keys,uint32_t ms,uint8_t expected){
 struct beetle_mono_action a=beetle_mono_scan(m,keys,ms,test_priority,test_release_age);
 assert(a.kind==expected);return a.key;
}
static void mono_inputs(void){
 struct beetle_mono_audition m;beetle_mono_reset(&m);
 assert(kind(&m,1u,100u,1u)==0u);
 /* Fast second onset within previous 45 ms MUST sound: regression. */
 assert(kind(&m,3u,103u,1u)==1u);
 assert(kind(&m,7u,106u,1u)==2u);
 /* Brief contact dip must not kill SID note or alter priority. */
 assert(kind(&m,3u,107u,0u)==2u);
 assert(kind(&m,7u,111u,0u)==2u);
 assert(kind(&m,3u,113u,0u)==2u);
 assert(kind(&m,3u,122u,1u)==1u); /* previous key still held */
 assert(kind(&m,1u,123u,0u)==1u);
 assert(kind(&m,1u,132u,1u)==0u);
 assert(kind(&m,0u,133u,0u)==0u);
 assert(kind(&m,0u,142u,2u)==BEETLE_AUDITION_NONE);
 /* Rapid repeated notes and deliberate key-up gaps >8ms. */
 for(unsigned i=0;i<100;i++){
  uint32_t at=200u+i*32u;
  assert(kind(&m,1u,at,1u)==0u);
  assert(kind(&m,0u,at+2u,0u)==0u);
  assert(kind(&m,0u,at+12u,2u)==BEETLE_AUDITION_NONE);
 }
 /* Wraparound-safe debounce. */
 assert(kind(&m,2u,0xFFFFFFF0u,1u)==1u);
 assert(kind(&m,0u,0xFFFFFFF6u,0u)==1u);
 assert(kind(&m,0u,4u,2u)==BEETLE_AUDITION_NONE);
}
static void sid_engine(void){
 struct track_chip_engine c;track_chip_init(&c);
 for(uint8_t type=TRACK_CHIP_SID_6581;type<=TRACK_CHIP_SID_NOISE;type++){
  track_chip_silence(&c);c.osc_type[0]=type;
  for(uint8_t n=0u;n<85u;n++){
   track_chip_stop_channel(&c,0u);
   assert(track_chip_note(&c,0u,0u,(uint8_t)(48u+(n%12u)),100u));
   uint32_t serial=c.voices[c.last_voice_index].birth;
   int16_t samples[64*2];struct track_mixer mix;track_mixer_init(&mix);
   memset(samples,0,sizeof(samples));track_chip_render(&c,&mix,samples,64u);
   assert(track_chip_health(&c));
   /* A stale release with an old serial is not permitted to gate this note. */
   assert(c.voices[c.last_voice_index].birth==serial);
  }
 }
}
static void song_data(void){
 struct track_engine e;track_init(&e);
 struct track_song_bank b;track_song_init(&b,&e);
 /* Auto row P01: visually empty, reference retained for auto activation. */
 for(uint8_t ch=0;ch<6u;ch++){
  assert(b.chain[0].lane[ch]==TRACK_SONG_AUTO_BASE);
  assert(!track_song_lane_visible(&b,b.chain[0].lane[ch],ch));
 }
 b.patterns[0].ch[0][1].note=60u;
 assert(track_song_lane_visible(&b,b.chain[0].lane[0],0u));
 assert(!track_song_lane_visible(&b,b.chain[0].lane[1],1u));
 assert(track_song_insert(&b,0u)==0);
 assert(b.chain[1].lane[0]==TRACK_SONG_AUTO_BASE);
 assert(track_song_lane_visible(&b,b.chain[1].lane[0],0u));
 b.patterns[1].ch[3][2].note=TRACK_NOTE_STOP;
 assert(track_song_lane_visible(&b,1u,3u));
 b.patterns[1].ch[5][2].chord=4u;
 assert(track_song_lane_visible(&b,1u,5u));
 /* Manual silence must stay muted even after later edits to its pattern. */
 b.chain[1].lane[3]=TRACK_SONG_SILENT;
 assert(!track_song_lane_visible(&b,b.chain[1].lane[3],3u));
 assert(track_song_change(&b,1u,3u,1)==0);
 assert(b.chain[1].lane[3]==0u);
 /* FX-only channels count as populated, including sparse global FX2. */
 b.extras.count=1u;b.extras.data[0]=(struct track_extra_fx){1u,(uint8_t)(2u*16u+5u),2u,50u};
 assert(track_song_lane_visible(&b,1u,2u));
}
int main(void){mono_inputs();song_data();sid_engine();puts("PASS V1.4.10 mono note arbitration, 5 SID types, auto Song Chains lanes");return 0;}
