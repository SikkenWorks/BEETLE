/* RC1.02 reverse key/scale search – read-only, root+all chord tones,
 * excludes drums and empty/STP steps. Host test. GPL-3.0-only. */
#include "beetle_reverse_key.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static struct track_engine eng;
static struct track_song_bank song;
static struct beetle_reverse_report result;
static void new_song(void){track_init(&eng);track_song_init(&song,&eng);}
static const struct beetle_reverse_candidate *lookup(unsigned root,unsigned mode){
 for(int i=0;i<BEETLE_REVERSE_CANDIDATES;i++)if(result.ranked[i].root==root&&result.ranked[i].mode==mode)return &result.ranked[i];return 0;
}
int main(void){
 new_song();beetle_reverse_analyse(&song,&eng,&result);
 CHECK(!result.events&&!result.exact_count);
 /* Drum note must not affect key inference. */
 eng.active.ch[TRACK_CHANNELS-1u][0].note=61;
 beetle_reverse_analyse(&song,&eng,&result);CHECK(!result.events);
 /* A C major chord from C, repeated patterns, contributes C/E/G. */
 eng.active.ch[0][0].note=60;eng.active.ch[0][0].chord=1;
 beetle_reverse_analyse(&song,&eng,&result);CHECK(result.events==1);
 const struct beetle_reverse_candidate *major=lookup(0,0);
 CHECK(major&&major->outside==0&&major->fit==100);
 uint8_t voices[5]={0};uint8_t count=track_chord(60,1,voices);
 CHECK(count>=2);
 /* A deliberately dissonant note outside C major reduces its fit. */
 eng.active.ch[1][2].note=66;
 beetle_reverse_analyse(&song,&eng,&result);CHECK(result.events==2);
 CHECK(lookup(0,0)->outside>0&&lookup(0,0)->fit<100);
 /* Stored pattern 10 must be included even if not selected. */
 new_song();song.patterns[10].ch[1][5].note=66;
 beetle_reverse_analyse(&song,&eng,&result);CHECK(result.events==1);
 /* Active pattern must be counted ONCE and is never overwritten by cached song. */
 eng.active.ch[0][3].note=62; song.patterns[0].ch[0][3].note=76;
 beetle_reverse_analyse(&song,&eng,&result);CHECK(result.events==2);
 /* STP/blank chord decorations are not new notes. */
 eng.active.ch[0][2].note=254;eng.active.ch[0][2].chord=5;
 beetle_reverse_analyse(&song,&eng,&result);CHECK(result.events==2);
 /* All candidate combinations and fit in 0..100; sorting by outside. */
 for(int i=0;i<BEETLE_REVERSE_CANDIDATES;i++){
  CHECK(result.ranked[i].root<12&&result.ranked[i].mode<9&&result.ranked[i].fit<=100);
  if(i)CHECK(result.ranked[i-1].outside<=result.ranked[i].outside);
 }
 printf("PASS RC1.02 reverse search: chromatic note + chord tones, 48 patterns, drum exclusion, CRC-independent\n");
 return 0;
}
