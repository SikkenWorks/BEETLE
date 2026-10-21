/* Stereo USB SRC regression model: 44,117.6 Hz codec -> 44,100 USB.
   Corrected controller updates once per 128-frame native DMA block.
   This is a host simulation, NOT a physical USB/device compliance test. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "beetle_usb_capture_src.h"
#include "beetle_usb_multitrack.h"
int main(void){
 const uint32_t DURATION_MS=600000u;
 const uint64_t block_ns=2901339ull;
 uint64_t w=104u,r=0u,next=0u;
 uint32_t step=BEETLE_UAC_SRC_ONE,frac=0u,acc=0u;
 int32_t filtered=104*256;
 uint32_t under=0,over=0,minfill=512u,maxfill=0u,minstep=100000u,maxstep=0u;
 for(uint32_t ms=0;ms<DURATION_MS;ms++){
  uint64_t ns=(uint64_t)ms*1000000ull;
  while(next<=ns){
   uint32_t fill=(uint32_t)(w-r);
   filtered=beetle_uac_mt_filter_fill(filtered,fill>512u?512u:fill);
   step=beetle_usb_src_next_step(step,(uint32_t)(filtered+128)>>8u,104u);
   if(fill+128u>512u)over++;else w+=128u;
   next+=block_ns;
  }
  uint32_t n=44u;acc+=100u;if(acc>=1000u){acc-=1000u;n++;}
  uint32_t fill=(uint32_t)(w-r);
  if(fill<minfill)minfill=fill;
  if(fill>maxfill)maxfill=fill;
  for(uint32_t i=0;i<n;i++){
   if((frac>>16u)+1u>=fill){under++;break;}
   frac+=step;
  }
  uint32_t k=frac>>16u;
  if(k>fill-(fill?1u:0u))k=fill-(fill?1u:0u);
  frac&=65535u;r+=k;
  if(step<minstep)minstep=step;
  if(step>maxstep)maxstep=step;
 }
 printf("Stereo UAC 10min: under=%u over=%u fill=%u..%u step=%u..%u final=%u\n",under,over,minfill,maxfill,minstep,maxstep,step);
 assert(under==0u&&over==0u);
 assert(minfill>20u&&maxfill<320u);
 assert(step>=65540u&&step<65610u);
 return 0;
}
