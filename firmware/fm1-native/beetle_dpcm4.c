/* SPDX-License-Identifier: GPL-3.0-only
 * BEETLE BD4S bounded, allocation-free, decoder for prefetched sample blocks.
 * This LAB module is intentionally not linked into FM1 builds yet.
 */
#include "beetle_dpcm4.h"
static int32_t bd4_clip(int32_t x, int32_t lo, int32_t hi){
 return x < lo ? lo : (x > hi ? hi : x);
}
static int32_t bd4_step_next(int32_t step, int32_t mag){
 if(mag<=1) step-=step/16;
 else if(mag>=5)step+=(step*(mag-4))/4;
 return bd4_clip(step,8,8192);
}
int beetle_bd4_decode_block(const uint8_t *src,uint32_t size,int16_t *dst,uint32_t cap){
 if(!src||!dst||size<6u)return -1;
 uint32_t frames=(uint32_t)src[4]|((uint32_t)src[5]<<8);
 uint32_t step=(uint32_t)src[2]|((uint32_t)src[3]<<8);
 if(!frames||frames>BEETLE_BD4_BLOCK_FRAMES||frames>cap||step<8u||step>8192u)return -2;
 if(size != 6u+(frames/2u))return -3; /* ceil((frames-1)/2) = frames/2 */
 int32_t predictor=(int16_t)((uint16_t)src[0]|((uint16_t)src[1]<<8));
 dst[0]=(int16_t)predictor;
 for(uint32_t i=1;i<frames;i++){
  uint8_t byte=src[6u+(i-1u)/2u];
  uint8_t nib=(uint8_t)(((i-1u)&1u)?(byte>>4u):(byte&15u));
  int32_t signed_delta=nib<8u?(int32_t)nib:(int32_t)nib-16;
  predictor=bd4_clip(predictor+signed_delta*(int32_t)step,-32768,32767);
  dst[i]=(int16_t)predictor;
  step=(uint32_t)bd4_step_next((int32_t)step,signed_delta<0?-signed_delta:signed_delta);
 }
 return (int)frames;
}
