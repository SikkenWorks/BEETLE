#include "../../firmware/fm1-native/track_chip.h"
#include "../../firmware/fm1-native/track_mixer.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static uint64_t difference(const int16_t *a,const int16_t *b,unsigned n){uint64_t d=0;for(unsigned i=0;i<n;i++){int32_t q=(int32_t)a[i]-(int32_t)b[i];d+=(uint32_t)(q<0?-q:q);}return d;}
static void render_case(uint8_t mask,uint8_t which,uint8_t value,int16_t *data){
 struct track_chip_engine s;track_chip_init(&s);s.punch_param[which]=value;
 for(unsigned k=0;k<1024;k++){unsigned a=k*2;data[a]=(int16_t)(((k*1831u)&16383u)-8192);data[a+1]=(int16_t)(((k*953u)&16383u)-8192);}
 track_chip_process_punch(&s,mask,data,1024);
 assert(track_chip_health(&s));
}
int main(void){
 struct track_chip_engine s;track_chip_init(&s);
 assert(s.health_cookie && s.punch_param[0]==50 && s.punch_param[7]==50);
 int16_t a[2048],b[2048];
 const uint8_t ids[]={0,1,2,3,4};const uint8_t masks[]={1,2,4,8,16};
 for(unsigned i=0;i<5;i++){
  render_case(masks[i],ids[i],8,a);render_case(masks[i],ids[i],92,b);
  uint64_t diff=difference(a,b,2048);
  printf("effect %u delta=%llu\n",ids[i],(unsigned long long)diff);
  assert(diff>0);
 }
 puts("PASS TRACK V0.15 native temporary FX parameters and bounded DSP");
}
