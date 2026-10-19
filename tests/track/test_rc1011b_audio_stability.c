/* RC1.0.11b: 64-row chains and bounded BEETLE-only DMA load mitigation.
 * No user project included. Uses maximum-length synthetic chain. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "track_song.h"
#include "track_chip.h"
#include "track_mixer.h"
static struct track_song_bank bank;
static struct track_chip_engine chip;
static struct track_mixer mix;
static struct track_engine engine;
static int16_t pcm[256];
int main(void){
 track_init(&engine);track_song_init(&bank,&engine);track_chip_init(&chip);track_mixer_init(&mix);
 assert(chip.realtime_voice_budget==8u);
 for(unsigned p=0;p<48;p++){
  bank.patterns[p].length=(uint8_t)(1u+p%16u);
  for(unsigned c=0;c<6;c++)for(unsigned s=0;s<16;s++){
   bank.patterns[p].ch[c][s].note=(uint8_t)(c<5?24u+(p+s+c)%50u:255u);
  }
  for(unsigned s=0;s<16;s++){bank.patterns[p].note2_fx[s].effect=(uint8_t)(p%9u);bank.patterns[p].note2_fx[s].value=(uint8_t)(p%100u);}
 }
 bank.count=64u;bank.enabled=1u;bank.loop=0u;
 for(unsigned row=0;row<64;row++){
  bank.chain[row].pattern=(uint8_t)(row%48u);bank.chain[row].repeats=1u;
  for(unsigned c=0;c<6;c++)bank.chain[row].lane[c]=(uint8_t)((row+c)%48u);
  if(row%9u==0u)bank.chain[row].lane[3]=TRACK_SONG_SILENT;
 }
 track_song_start(&bank,&engine);
 for(unsigned row=0;row<64;row++){
  assert(bank.position==row);
  const struct track_song_entry *en=&bank.chain[row];
  for(unsigned c=0;c<6;c++)for(unsigned st=0;st<16;st++){
   const struct track_step *actual=&engine.active.ch[c][st];
   if(en->lane[c]==255u)assert(actual->note==255u);
   else{
    const struct track_pattern *src=&bank.patterns[en->lane[c]];
    assert(!memcmp(actual,&src->ch[c][st%src->length],sizeof(*actual)));
    if(c==5)assert(!memcmp(&engine.active.note2_fx[st],&src->note2_fx[st%src->length],sizeof(engine.active.note2_fx[st])));
   }
  }
  if(row+1<64)assert(track_song_on_wrap(&bank,&engine)==1);
 }
 for(unsigned c=0;c<4;c++){
  chip.osc_type[c]=TRACK_CHIP_RETRO_PULSE;
  for(unsigned i=0;i<2;i++)assert(track_chip_note(&chip,c,c,(uint8_t)(48+c*2+i),110));
 }
 assert(chip.voice_count==8u);
 /* Four late callbacks must actually reduce TRACK DSP voice count. */
 for(int b=7;b>=4;b--){track_chip_set_realtime_budget(&chip,(uint8_t)b);assert(chip.voice_count<=b);assert(track_chip_health(&chip));}
 for(int i=0;i<32;i++){
  assert(track_chip_note(&chip,(uint8_t)(i%4),0,(uint8_t)(i%40+48),100));
  assert(chip.voice_count<=4u);
  track_chip_render(&chip,&mix,pcm,128);
  assert(track_chip_health(&chip));
 }
 track_chip_set_realtime_budget(&chip,8u);
 for(unsigned i=0;i<4;i++)assert(track_chip_note(&chip,(uint8_t)(i%4),0,(uint8_t)(72+i),110));
 assert(chip.voice_count<=8u);
 puts("PASS dense 64-row song transitions, source isolation, 4..8 voice budget, 32 audio chunks");
 return 0;
}
