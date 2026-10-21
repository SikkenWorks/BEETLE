/* RC1.01 KEYLOCK regression: editor + generated harmony + compatibility.
 * SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "track_core.h"
static const unsigned char mode_degrees[9][7]={
 {0,2,4,5,7,9,11},{0,2,3,5,7,9,10},{0,1,3,5,7,8,10},
 {0,2,4,6,7,9,11},{0,2,4,5,7,9,10},{0,2,3,5,7,8,10},
 {0,1,3,5,6,8,10},{0,2,3,5,7,8,11},{0,2,3,5,7,9,11}};
static int allowed(unsigned v,const struct track_harmony *h){
 for(int k=0;k<7;k++)if(v%12==(unsigned)((h->root+mode_degrees[h->mode][k])%12))return 1;
 return 0;
}
int main(void){
 unsigned cases=0;
 for(unsigned root=0;root<12;root++)for(unsigned mode=0;mode<9;mode++){
  struct track_harmony h={(unsigned char)root,(unsigned char)mode,2};
  unsigned seq[130],len=0;
  for(unsigned n=0;n<128;n++)if(allowed(n,&h))seq[len++]=n;
  seq[len++]=TRACK_NOTE_EMPTY;seq[len++]=TRACK_NOTE_STOP;
  for(unsigned at=0;at<len;at++)for(int d=-3;d<=3;d++){
   unsigned res=track_note_scroll_keylocked((unsigned char)seq[at],d,0,&h);
   int index=((int)at+d)%((int)len);if(index<0)index+=(int)len;
   assert(res==seq[index]);cases++;
  }
  for(unsigned n=0;n<128;n++){
   for(int dir=-4;dir<=4;dir++)if(dir){
    unsigned result=track_note_scroll_keylocked((unsigned char)n,dir,0,&h);
    assert(result>=128||allowed(result,&h));cases++;
   }
   for(unsigned type=0;type<TRACK_CHORD_COUNT;type++){
    unsigned char tones[5]={0};unsigned c=track_chord_keylocked((unsigned char)n,(unsigned char)type,&h,tones);
    assert(c>0&&c<=5);
    for(unsigned v=0;v<c;v++){
     assert(allowed(tones[v],&h));
     if(v>0)assert(tones[v]>=tones[v-1]);
    }
    cases++;
   }
  }
 }
 struct track_harmony c={0,0,2};unsigned char out[5];
 assert(track_chord_keylocked(62,1,&c,out)==3);
 assert(out[0]==62&&out[1]==65&&out[2]==69);/* D MAJOR -> D-F-A in C */
 assert(track_chord_keylocked(71,3,&c,out)==3);
 assert(out[0]==71&&out[1]==74&&out[2]==77);/* B DIM -> B-D-F */
 assert(track_note_scroll_keylocked(60,1,0,&c)==62);
 assert(track_note_scroll_keylocked(62,-1,0,&c)==60);
 assert(track_note_scroll_keylocked(61,1,0,&c)==62);
 assert(track_note_scroll_keylocked(61,-1,0,&c)==60);
 assert(track_note_scroll_keylocked(255,1,0,&c)==254);
 assert(track_note_scroll_keylocked(254,1,0,&c)==0);
 assert(track_note_scroll_keylocked(3,1,1,&c)==4);/* Drums stay unchanged */
 struct track_harmony guide={0,0,1},off={0,0,0};
 assert(track_note_scroll_keylocked(60,1,0,&guide)==61);
 assert(track_note_scroll_keylocked(60,1,0,&off)==61);
 assert(track_chord_keylocked(62,1,&guide,out)==3&&out[1]==66);
 assert(track_chord_keylocked(62,1,&off,out)==3&&out[1]==66);
 puts("KEYLOCK tracker scrolling and linked chord voices: PASS");
 printf("Validated %u all-scale cases and legacy modes.\n",cases);
 return 0;
}
