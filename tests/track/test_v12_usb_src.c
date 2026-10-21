/* GPL-3.0-only, deterministic two-minute dual-clock UAC1 PCM model. */
#include "beetle_usb_capture_src.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#define RING_N 512u
#define PI 3.14159265358979323846
static uint32_t buf[RING_N],write_index,read_index;
static uint32_t step=BEETLE_UAC_SRC_ONE,pos,lowpoint=104u;
static uint64_t produced,consumed,output_count;
static uint32_t underruns,overruns,highfill,lowfill=512u;
static int previous,last_output,has_previous;
static uint32_t ticks;
static uint32_t pack(int16_t l,int16_t r){return (uint16_t)l|((uint32_t)(uint16_t)r<<16);}
static void put(uint32_t pcm){if(write_index-read_index>=RING_N){overruns++;return;}buf[(write_index++)&(RING_N-1u)]=pcm;}
static void render(void){
 lowpoint=write_index-read_index;
 for(int i=0;i<128;i++){
  double t=(double)produced/44117.6;
  int16_t l=(int16_t)(sin(2*PI*440.0*t)*11000.0);
  put(pack(l,(int16_t)-l));
  produced++;
 }
}
int main(void){
 for(int i=0;i<104;i++)put(0); /* real UAC startup priming */
 double next_render=0;
 uint32_t acc=0;
 for(ticks=0;ticks<120000;ticks++){
  double time=(double)ticks/1000.;
  while(next_render<=time){render();next_render+=128.0/44117.6;}
  acc+=100;uint32_t n=44;if(acc>=1000){n++;acc-=1000;}
  step=beetle_usb_src_next_step(step,lowpoint,104u);
  uint32_t fill=write_index-read_index;
  if(fill>highfill)highfill=fill;
  if(fill<lowfill)lowfill=fill;
  uint32_t advance=pos,hold=0;
  for(uint32_t j=0;j<n;j++){
   uint32_t src=advance>>16;
   if(src+1>=fill){underruns++;break;}
   uint32_t pcm=beetle_usb_src_lerp(buf[(read_index+src)&(RING_N-1u)],
                        buf[(read_index+src+1)&(RING_N-1u)],(uint16_t)advance);
   int16_t l=(int16_t)pcm,r=(int16_t)(pcm>>16);
   assert((int)l+(int)r>=-2 && (int)l+(int)r<=2);
   if(has_previous && ticks>2000){int delta=(int)l-previous;
      /* 440 Hz, 11k amplitude: a healthy pair of neighbours differs <700. */
      assert(delta>-1000 && delta<1000);
   }
   previous=l;has_previous=1;last_output=l;hold++;
   advance+=step;output_count++;
  }
  uint32_t whole=advance>>16;
  if(whole>=fill)whole=fill?fill-1:0;
  read_index+=whole;consumed+=whole;
  pos=(uint16_t)advance;
 }
 assert(overruns==0 && underruns==0);
 assert(output_count==120000u/10u*441u);
 assert(step>65545u&&step<65620u); /* 44,117.6 -> 44,100: ~+26 Q16 */
 assert(highfill<350u&&lowfill>8u);
 printf("PASS V1.2 UAC SRC: 120s, %llu host frames, no underrun/overrun, step %u, ring %u..%u\n",
       (unsigned long long)output_count,step,lowfill,highfill);
 return 0;
}
