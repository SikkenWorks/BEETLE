#include <stdio.h>
#include <stdint.h>
#include "../../firmware/fm1-native/beetle_boot_led_policy.h"
int main(void){
 uint32_t splash=1320u;
 for(uint8_t page=0;page<41;page++)for(uint8_t id=0;id<41;id++){
  uint8_t expected=(page==29u && id==3u);
  if(beetle_start_led_lit(page,1320u,splash,id)!=expected)return 1;
  if(beetle_start_led_lit(page,1400u,splash,id)!=expected)return 2;
  if(beetle_start_led_lit(page,1319u,splash,id))return 3;
 }
 if(!beetle_start_led_only_sel(29u,1320u,splash))return 4;
 if(beetle_start_led_only_sel(14u,1320u,splash))return 5;
 puts("PASS: boot project chooser lights only SEL; splash and other pages unaffected");
 return 0;
}
