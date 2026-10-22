/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "beetle_rec_led.h"
int main(void){
 for(unsigned p=0;p<32;p++)for(unsigned note=0;note<=1;note++)for(unsigned punch=0;punch<=1;punch++){
  unsigned expected=(p==3||p==4)?punch:note;
  assert(beetle_rec_led_on((uint8_t)p,(uint8_t)note,(uint8_t)punch)==expected);
 }
 assert(beetle_rec_led_on(0,1,0)); /* boot default REC ON */
 assert(!beetle_rec_led_on(0,0,1)); /* tracker off despite FX previously armed */
 assert(!beetle_rec_led_on(3,1,0)); /* FX off despite note entry armed */
 assert(beetle_rec_led_on(4,0,1)); /* Punch menu on */
 puts("PASS V1.4.13: physical REC LED follows note-entry / Punch-In arm independently on all screens");
 return 0;
}
