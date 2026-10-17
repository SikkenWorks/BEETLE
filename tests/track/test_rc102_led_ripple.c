/* Pure LED animation checks: every physical FM1 key/button lights during
 * the boot ripple, beginning with two central musical keys. */
#include "beetle_led_ripple.h"
#include <stdio.h>
int main(void){
 if(beetle_ripple_level(0,27)!=2 || beetle_ripple_level(0,28)!=2){puts("FAIL centre");return 1;}
 for(unsigned id=0;id<41;id++){
  int seen=0;
  for(unsigned t=0;t<1120;t+=8)if(beetle_ripple_level(t,(uint8_t)id)==2){seen=1;break;}
  if(!seen){printf("FAIL LED %u never lit\n",id);return 1;}
 }
 for(unsigned id=0;id<41;id++)if(beetle_ripple_level(1160,(uint8_t)id)){puts("FAIL fade");return 1;}
 puts("PASS 41 physical LEDs centre-out ripple and end fade");return 0;
}
