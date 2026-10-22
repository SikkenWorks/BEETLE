#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../../firmware/fm1-native/beetle_song_rows.h"
#include "../../firmware/fm1-native/beetle_battery_policy.h"
int main(void){
 struct track_song_bank song={0};
 struct beetle_song_rows_clip clip={0};
 song.count=6u;
 for(unsigned i=0;i<6;i++){
  song.chain[i].repeats=(uint8_t)(i+1u);
  for(unsigned c=0;c<TRACK_CHANNELS;c++)song.chain[i].lane[c]=(uint8_t)(c==1?255u:(128+i));
 }
 assert(beetle_song_rows_in_range(2,4,2));
 assert(!beetle_song_rows_in_range(5,4,2));
 assert(beetle_song_rows_copy(&clip,&song,4u,2u)==3);
 assert(clip.count==3u && clip.rows[0].repeats==3u && clip.rows[2].repeats==5u);
 assert(beetle_song_rows_paste(&clip,&song,5u)==3);
 assert(song.count==8u);
 assert(song.chain[5].repeats==3u&&song.chain[6].repeats==4u&&song.chain[7].repeats==5u);
 assert(song.chain[7].lane[1]==TRACK_SONG_SILENT);
 /* Preflight is atomic, boundary rejection cannot modify any row. */
 struct track_song_entry before=song.chain[7];
 assert(beetle_song_rows_paste(&clip,&song,62u)<0);
 assert(!memcmp(&before,&song.chain[7],sizeof(before)));
 for(unsigned i=0;i<64;i++){
  song.chain[i].repeats=(uint8_t)((i%16u)+1u);
  song.chain[i].lane[0]=(uint8_t)(i%TRACK_PATTERNS);
 }
 song.count=64u;
 assert(beetle_song_rows_copy(&clip,&song,0,63)==64);
 assert(beetle_song_rows_paste(&clip,&song,0)==64);
 assert(song.count==64u && song.chain[63].repeats==16u);
 /* LCD must switch red only under threshold and recover with hysteresis. */
 assert(beetle_battery_filter(600,500,0)==500);
 assert(beetle_battery_filter(600,500,1)==588);
 assert(!beetle_battery_low20(800u,0u));
 assert(beetle_battery_low20(BEETLE_BATTERY_20_ADC_RAW,0u));
 assert(beetle_battery_low20((uint16_t)(BEETLE_BATTERY_20_ADC_RAW+10u),1u));
 assert(!beetle_battery_low20((uint16_t)(BEETLE_BATTERY_20_ADC_RAW+25u),1u));
 puts("PASS V1.4.12: 64 song rows, independent clipboard, atomic paste, battery policy");
 return 0;
}
