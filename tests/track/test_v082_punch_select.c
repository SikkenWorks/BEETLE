#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include "beetle_punch_timing.h"
#include "beetle_input_safety.h"
#define SEL BEETLE_SELECT_BUTTON
int main(void){
 static const uint8_t values[7]={0,17,33,50,67,83,100};
 for(uint16_t bpm=30;bpm<=300;bpm+=15){
  uint32_t last=0xFFFFFFFFu;
  for(uint8_t i=0;i<7;i++){
   uint32_t half=beetle_stutter_half_frames(bpm,values[i]);
   assert(beetle_stutter_index(values[i])==i);
   assert(half<last);last=half;
  }
  assert(beetle_stutter_half_frames(bpm,50u)==(2646000u/bpm)/2u);
 }
 assert(beetle_reverse_rate_q8(50)==128u);
 assert(beetle_reverse_rate_q8(0)==64u);
 assert(beetle_reverse_rate_q8(100)==256u);
 assert(beetle_reverse_rate_q8(25)<128u&&beetle_reverse_rate_q8(75)>128u);
 struct beetle_load_input_gate g={0};uint8_t ready=0u;
 beetle_sel_rearm(&ready,0u,0u,0u,&g);assert(ready);
 assert(ready&&beetle_step_delete_eligible(0,0,&g,SEL));
 beetle_load_input_arm(&g,100);ready=0u;
 for(uint8_t page=1u;page<=18u;page+=17u){
  uint32_t p=SEL,r=0u;
  beetle_load_input_filter(&g,180,SEL,&p,&r);
  beetle_sel_rearm(&ready,page,SEL,p,&g);
  assert(!ready&&!beetle_step_delete_eligible(page,0,&g,SEL));
 }
 uint32_t p=SEL,r=SEL;
 beetle_load_input_filter(&g,300,0u,&p,&r);
 beetle_sel_rearm(&ready,0u,0u,p,&g);assert(!ready);
 p=SEL;r=0;
 beetle_load_input_filter(&g,625,0u,&p,&r);
 beetle_sel_rearm(&ready,0u,0u,p,&g);
 assert(!g.active&&ready);
 p=SEL;beetle_sel_rearm(&ready,0u,SEL,p,&g);
 assert(ready&&beetle_step_delete_eligible(0u,0u,&g,p));
 beetle_sel_rearm(&ready,8u,SEL,SEL,&g);assert(!ready);
 beetle_sel_rearm(&ready,0u,SEL,SEL,&g);assert(!ready);
 beetle_sel_rearm(&ready,0u,0u,0u,&g);assert(ready);
 puts("PASS V0.82: 7 BPM ratios, reverse-speed neutral, SEL rearm across I/O/errors");
 return 0;
}
