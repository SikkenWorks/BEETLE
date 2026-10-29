#include <stdint.h>
#include <assert.h>
#include "../../firmware/fm1-native/beetle_battery_policy.h"
int main(void){
 assert(!beetle_battery_raw_valid(0));
 assert(!beetle_battery_raw_valid(1023));
 assert(beetle_battery_raw_valid(531));
 assert(beetle_battery_bars(530)==0u);
 assert(beetle_battery_bars(531)==1u);
 assert(beetle_battery_bars(560)==1u);
 assert(beetle_battery_bars(561)==3u);
 assert(beetle_battery_bars(590)==3u);
 assert(beetle_battery_bars(591)==4u);
 assert(beetle_battery_filter(530,538,1)==531);
 assert(beetle_battery_filter(530,538,0)==538);
 assert(beetle_battery_low20(543,1));
 assert(!beetle_battery_low20(544,1));
 return 0;
}
