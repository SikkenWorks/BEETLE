/* SPDX-License-Identifier: GPL-3.0-only
 * BEETLE V1.4.30 regression: 16-bit SID DSP and project-menu key fence. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../../firmware/fm1-native/beetle_sid_lite.h"
#include "../../firmware/fm1-native/beetle_menu_note_fence.h"
static uint32_t run_sid(uint8_t chip,uint8_t cutoff,uint8_t combo,uint8_t sync,uint8_t ring){
 struct beetle_sid_prepared p=beetle_sid_prepare(chip,ring?1u:0u,1,50,50,50,cutoff,70,24,combo,sync,ring==1u);
 uint32_t ph=0,ph2=0,lfsr=0x7ffff8u;
 int32_t lp=0,bp=0;uint32_t checksum=2166136261u;
 for(unsigned i=0;i<10000;i++){
  ph+=21377731u;ph2+=26977211u;
  if(sync && ph<21377731u)ph2=0u;
  int32_t y=beetle_sid_sample_prepared(&p,ph,21377731u,ph2,&lfsr,&lp,&bp);
  assert(y>=-16000 && y<=16000);
  checksum=(checksum^(uint32_t)y)*16777619u;
 }
 return checksum;
}
int main(void){
 struct beetle_sid_prepared a=beetle_sid_prepare(4u,0,1,50,50,50,50,70,0,0,0,0);
 struct beetle_sid_prepared b=beetle_sid_prepare(5u,0,1,50,50,50,50,70,0,0,0,0);
 assert(a.filter_coefficient<b.filter_coefficient);
 for(unsigned i=0;i<=100;i++){
  struct beetle_sid_prepared x=beetle_sid_prepare(4u,0,1,50,50,50,i,100,0,0,0,0);
  struct beetle_sid_prepared y=beetle_sid_prepare(5u,0,1,50,50,50,i,100,0,0,0,0);
  assert(x.filter_coefficient>0 && x.filter_coefficient<=124);
  assert(y.filter_coefficient>0 && y.filter_coefficient<=124);
 }
 uint32_t c0=run_sid(4,70,0,0,0),c1=run_sid(5,70,0,0,0),c2=run_sid(4,70,85,0,0);
 uint32_t c3=run_sid(4,70,0,1,0),c4=run_sid(4,70,0,0,1);
 uint32_t c5=run_sid(4,70,0,0,2); /* same TRI A but ring OFF */
 /* Ring modulation must change TRIANGLE A, but not other waveforms. */
 assert(c0!=c1 && c0!=c2 && c0!=c3 && c4!=c5);
 uint32_t blocked=0u;
 /* A key down while NEW/LOAD is open cannot be replayed in the tracker. */
 beetle_menu_note_capture(&blocked,1u<<4);
 assert(beetle_menu_note_filter(&blocked,1u<<4,1u<<4)==0);
 /* New independent key in tracker still works with previous key held. */
 assert(beetle_menu_note_filter(&blocked,(1u<<4)|(1u<<9),1u<<9)==(1u<<9));
 /* Releasing then pressing the old menu key makes it available again. */
 assert(beetle_menu_note_filter(&blocked,0u,0u)==0u);
 assert(beetle_menu_note_filter(&blocked,1u<<4,1u<<4)==(1u<<4));
 /* A key tapped then released entirely inside the modal has no edge. */
 beetle_menu_note_capture(&blocked,1u<<7);
 assert(beetle_menu_note_filter(&blocked,0u,0u)==0u);
 puts("PASS V1.4.30 SID curves, combo/sync/ring and NEW/LOAD key isolation");
 return 0;
}
