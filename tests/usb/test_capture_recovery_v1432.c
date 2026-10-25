/* SPDX-License-Identifier: GPL-3.0-only
 * Portable validation for the same bounds, clock controller and fade helpers
 * used by native usb.c. Models delayed PCM renders and stopped host reads.
 * This is not a physical JieLi USB peripheral/interrupt timing test.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "beetle_usb_capture_recovery.h"
#include "beetle_usb_multitrack.h"

#define N 512u
#define PRIME BEETLE_UAC_PRIME_44100
static uint32_t pseudorandom=0x47664c1du;
static uint32_t rnd(void){pseudorandom^=pseudorandom<<13;pseudorandom^=pseudorandom>>17;pseudorandom^=pseudorandom<<5;return pseudorandom;}
static void test_helpers(void){
 assert(BEETLE_UAC_PRIME_44100+128u+46u<N);
 assert(BEETLE_UAC_PRIME_48000+140u+49u<N);
 assert(beetle_uac_can_write(400,100,128,N));
 assert(!beetle_uac_can_write(510,100,128,N));
 assert(!beetle_uac_can_write(100,101,128,N)); /* stale r > w must not wrap to free space */
 assert(!beetle_uac_can_write(1000,1,1,N));
 assert(beetle_uac_can_write(0,0,512,N));
 assert(!beetle_uac_can_write(0,0,513,N));
 assert(beetle_uac_fade_tail(30000,0,40)==0);
 assert(beetle_uac_fade_tail(30000,40,40)==30000);
 assert(beetle_uac_fade_tail(-32768,0,40)==0);
 assert(beetle_uac_fade_in(23000,0)==0);
 assert(beetle_uac_fade_in(23000,BEETLE_UAC_FADE_FRAMES)==23000);
 assert(beetle_uac_fade_stereo(0x80007fffu,0u,40u)==0);
 for(uint32_t i=0;i<100000;i++){
  uint32_t packed=rnd(),left=rnd()%48,total=48;
  uint32_t f=beetle_uac_fade_stereo(packed,left,total);
  assert((int32_t)(int16_t)f<=32767 &&(int32_t)(int16_t)(f>>16)<=32767);
 }
}

static void simulate(int inject){
 /* Producer 128 frames every 2.901339 ms; USB consumer 44.1 frames/ms.
  * The codec is intentionally 0.04% fast, clock compensation must settle. */
 const uint32_t duration=inject?360000u:600000u;
 uint64_t next_dma=0;
 uint64_t w=PRIME, r=0;
 uint32_t frac=0,acc=0,step=65536u;
 int32_t filter=(int32_t)PRIME*256;
 uint32_t prime_restarts=0,over=0,under=0,skipped=0,pkts=0;
 uint32_t fill_max=0,fill_min=N;
 int go=1;
 for(uint32_t ms=0;ms<duration;ms++){
  uint64_t now=(uint64_t)ms*1000000ull;
  while(next_dma<=now){
   if(!go){
    w=r+PRIME;frac=0;step=65536u;filter=(int32_t)PRIME*256;
    go=1;prime_restarts++;
   }else{
    uint32_t fill=(uint32_t)(w-r);
    assert(fill<=N);
    filter=beetle_uac_mt_filter_fill(filter,fill);
    step=beetle_uac_mt_next_step(step,filter,PRIME);
   }
   if(!beetle_uac_can_write((uint32_t)w,(uint32_t)r,128,N)){
    over++;go=0;
   }else w+=128;
   /* A realistic delayed DMA IRQ under load can arrive one or two ms
    * late, even though the nominal average buffer duty looks fine. */
   next_dma+=2901339ull;
   if(inject && ((ms%2503u)==1090u))next_dma+=4000000ull;
   if(inject && ((ms%10019u)==8400u))next_dma+=9500000ull;
  }
  /* Simulate host briefly not reading EP4 (falling asleep / DMA stalls). */
  if(inject && (ms%78017u)>=25000u &&(ms%78017u)<25022u){skipped++;continue;}
  uint32_t n=44u;acc+=100u;if(acc>=1000u){acc-=1000u;n++;}
  pkts++;
  if(!go)continue;
  uint32_t fill=(uint32_t)(w-r);
  assert(fill<=N);
  if(fill>fill_max)fill_max=fill;
  if(fill<fill_min)fill_min=fill;
  uint32_t pos=frac;
  for(uint32_t i=0;i<n;i++){
   if((pos>>16u)+1u>=fill){under++;go=0;break;}
   pos+=step;
  }
  uint32_t k=pos>>16u;
  if(k>fill-(fill?1u:0u))k=fill-(fill?1u:0u);
  r+=k;frac=go?(uint16_t)pos:0;
  assert(w>=r && w-r<=N);
 }
 printf("USB %s model: underruns=%u overruns=%u reprimes=%u USB-pkts=%u host-stall-polls=%u fill=%u..%u last-step=%u\n",
  inject?"injected six-minute stressed":"ten-minute nominal",under,over,prime_restarts,pkts,skipped,fill_min,fill_max,step);
 if(!inject){assert(under==0 &&over==0 &&prime_restarts==0);assert(fill_min>100&&fill_max<490);}
 else{assert(prime_restarts>0);assert(over>0||under>0);assert(under<800&&over<500);}
}
int main(void){test_helpers();simulate(0);simulate(1);puts("PASS V1.4.32 USB-C capture recovery host tests");return 0;}
