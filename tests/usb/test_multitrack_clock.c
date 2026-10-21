/* GPL-3.0-only: six-stem USB shared read clock producer/consumer model.
 * No actual USB peripheral; deterministic codec 44117.6Hz, USB 44100Hz.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "beetle_usb_multitrack.h"
int main(void){
 const uint64_t DURATION_MS=600000u; /* 10 minute recording */
 uint64_t w=104u,r=0u,srcframes=0u,usbframes=0u;
 /* Fixed-point microsecond clock to avoid simulation float drift. */
 uint64_t next_dma_ns=0,now_ns;
 const uint64_t block_ns=2901339ull; /* 128 / 44117.6 seconds */
 uint32_t step=BEETLE_UAC_MT_ONE,frac=0u,acc=0u;
 int32_t fill_q8=104*256;
 uint32_t und=0,over=0,minfill=512u,maxfill=0u,max_step=step,min_step=step;
 for(uint32_t ms=0;ms<DURATION_MS;ms++){
  now_ns=(uint64_t)ms*1000000ull;
  while(next_dma_ns<=now_ns){
   uint32_t fill=(uint32_t)(w-r);
   fill_q8=beetle_uac_mt_filter_fill(fill_q8,fill>512u?512u:fill);
   step=beetle_uac_mt_next_step(step,fill_q8,104);
   if(fill+128u>512u)over++;
   else {w+=128u;srcframes+=128u;}
   next_dma_ns+=block_ns;
  }
  uint32_t n=44u;acc+=100u;if(acc>=1000u){acc-=1000u;n++;}
  uint32_t fill=(uint32_t)(w-r);
  if(fill<minfill)minfill=fill;
  if(fill>maxfill)maxfill=fill;
  for(uint32_t i=0;i<n;i++){
   if((frac>>16u)+1u>=fill){und++;break;}
   frac+=step;
  }
  uint32_t whole=frac>>16u;
  if(whole>fill-(fill?1u:0u))whole=fill-(fill?1u:0u);
  frac&=65535u;r+=whole;
  usbframes+=n;
  if(step>max_step)max_step=step;
  if(step<min_step)min_step=step;
 }
 printf("10min MT UAC model: source=%llu host=%llu underruns=%u overruns=%u ring=%u..%u step=%u..%u final=%u\n",
 (unsigned long long)srcframes,(unsigned long long)usbframes,und,over,minfill,maxfill,min_step,max_step,step);
 assert(und==0u && over==0u && minfill>20u && maxfill<320u);
 assert(usbframes==DURATION_MS/10u*441u);
 assert(step>65540u && step<65610u);
 return 0;
}
