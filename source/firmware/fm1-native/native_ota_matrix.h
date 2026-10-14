/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH V0.72 OTA matrix visualizer. Pure RAM renderer: no USB, flash, audio,
 * GPIO or timer calls. A 240 x 40 stripe is published by the main-loop adapter.
 * All output pixels use the FM1 LCD DMA's native byte-swapped RGB565 layout.
 */
#pragma once
#include <stdint.h>
#include "beetle_play_icon.h"
#define ETCH_MATRIX_WIDTH 240u
#define ETCH_MATRIX_BAND_HEIGHT 40u
#define ETCH_MATRIX_SYMBOL_COUNT 45u
static inline uint16_t etch_matrix_pixel(uint16_t c){return (uint16_t)((c<<8)|(c>>8));}
static inline uint32_t etch_matrix_rand(uint32_t c,uint32_t r,uint32_t frame){
 uint32_t n=c*0x9e3779b9u ^ r*0x85ebca6bu ^ frame*0xc2b2ae35u ^ 0x4d415452u;
 n^=n>>16;n*=0x7feb352du;n^=n>>15;n*=0x846ca68bu;return n^(n>>16);
}
/* 5x7 bitmap glyphs, uppercase, digits and electronics-like symbols. */
static const uint8_t etch_matrix_glyphs[ETCH_MATRIX_SYMBOL_COUNT][7]={
 {14,17,17,31,17,17,17}, /* A */ {30,17,17,30,17,17,30}, /* B */
 {15,16,16,16,16,16,15}, /* C */ {30,17,17,17,17,17,30}, /* D */
 {31,16,16,30,16,16,31}, /* E */ {31,16,16,30,16,16,16}, /* F */
 {15,16,16,23,17,17,15}, /* G */ {17,17,17,31,17,17,17}, /* H */
 {14,4,4,4,4,4,14}, /* I */ {7,2,2,2,18,18,12}, /* J */
 {17,18,20,24,20,18,17}, /* K */ {16,16,16,16,16,16,31}, /* L */
 {17,27,21,21,17,17,17}, /* M */ {17,25,21,19,17,17,17}, /* N */
 {14,17,17,17,17,17,14}, /* O */ {30,17,17,30,16,16,16}, /* P */
 {14,17,17,17,21,18,13}, /* Q */ {30,17,17,30,20,18,17}, /* R */
 {15,16,16,14,1,1,30}, /* S */ {31,4,4,4,4,4,4}, /* T */
 {17,17,17,17,17,17,14}, /* U */ {17,17,17,17,17,10,4}, /* V */
 {17,17,17,21,21,21,10}, /* W */ {17,17,10,4,10,17,17}, /* X */
 {17,17,10,4,4,4,4}, /* Y */ {31,1,2,4,8,16,31}, /* Z */
 {14,17,19,21,25,17,14}, /* 0 */ {4,12,4,4,4,4,14}, /* 1 */
 {14,17,1,2,4,8,31}, /* 2 */ {30,1,1,14,1,1,30}, /* 3 */
 {2,6,10,18,31,2,2}, /* 4 */ {31,16,16,30,1,1,30}, /* 5 */
 {14,16,16,30,17,17,14}, /* 6 */ {31,1,2,4,8,8,8}, /* 7 */
 {14,17,17,14,17,17,14}, /* 8 */ {14,17,17,15,1,1,14}, /* 9 */
 {10,31,10,31,10,0,0}, /* # */ {14,17,23,21,23,16,14}, /* @ */
 {4,21,14,31,14,21,4}, /* * */ {0,4,4,31,4,4,0}, /* + */
 {14,17,1,2,4,0,4}, /* ? */ {2,4,8,16,8,4,2}, /* < */
 {8,4,2,1,2,4,8}, /* > */ {17,2,4,8,17,0,0}, /* % */
 {0,0,31,0,0,0,0} /* - */
};
static inline void etch_matrix_rect(uint16_t *dst,uint16_t y0,uint16_t height,
                                    int32_t x,int32_t y,uint32_t w,uint32_t h,uint16_t color){
 if(x<0||y<(int32_t)y0||x+(int32_t)w>240||y+(int32_t)h>(int32_t)(y0+height))return;
 uint16_t pix=etch_matrix_pixel(color);
 for(uint32_t iy=0;iy<h;iy++)for(uint32_t ix=0;ix<w;ix++)
  dst[(uint32_t)(y-(int32_t)y0+iy)*240u+(uint32_t)x+ix]=pix;
}
/* A glyph can span two stripes: clip each pixel against stripe boundaries. */
static inline void etch_matrix_char(uint16_t *dst,uint16_t y0,uint16_t height,
                       char ch,int32_t x,int32_t y,uint8_t scale,uint16_t color){
 uint32_t index=44u;
 if(ch>='A'&&ch<='Z')index=(uint32_t)(ch-'A');
 else if(ch>='0'&&ch<='9')index=(uint32_t)(ch-'0')+26u;
 else {static const char marks[]="#@*+?<>%-";
  for(uint32_t i=0;i<9;i++)if(ch==marks[i]){index=i+36u;break;}}
 if(index>=ETCH_MATRIX_SYMBOL_COUNT)return;
 uint16_t pix=etch_matrix_pixel(color);
 for(uint32_t row=0;row<7u;row++)for(uint32_t col=0;col<5u;col++){
  if(!(etch_matrix_glyphs[index][row]&(16u>>col)))continue;
  int32_t px=x+(int32_t)col*scale,py=y+(int32_t)row*scale;
  for(uint32_t iy=0;iy<scale;iy++)for(uint32_t ix=0;ix<scale;ix++){
   int32_t sx=px+(int32_t)ix,sy=py+(int32_t)iy;
   if(sx>=0&&sx<240&&sy>=(int32_t)y0&&sy<(int32_t)(y0+height))
    dst[(uint32_t)(sy-(int32_t)y0)*240u+(uint32_t)sx]=pix;
  }
 }
}
static inline void etch_matrix_band(uint16_t *dst,uint16_t y0,uint16_t height,
                                    uint32_t frame,uint8_t step,int32_t error){
 if(!dst||!height||y0>=240u||height>ETCH_MATRIX_BAND_HEIGHT||y0+height>240u)return;
 for(uint32_t i=0;i<(uint32_t)height*240u;i++)dst[i]=etch_matrix_pixel(0x0801u);
 const char alphabet[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789#@*+?<>%-";
 const uint32_t row_start=y0/10u,row_end=(y0+height+9u)/10u;
 for(uint32_t row=row_start;row<row_end&&row<24u;row++)for(uint32_t col=0;col<24u;col++){
  uint32_t rnd=etch_matrix_rand(col,row,frame);
  uint32_t phase=(row+frame+(col*7u))%19u;
  uint16_t shade=phase==0u?0xF81Au:phase<3u?0xA014u:phase<8u?0x500Au:0x2805u;
  char character=alphabet[rnd%(sizeof(alphabet)-1u)];
  etch_matrix_char(dst,y0,height,character,(int32_t)(col*10u+2u),(int32_t)(row*10u+1u),1u,shade);
 }
 /* Centered 2-frame BEETLE play icon. Clip every enlarged pixel to each
  * separately refreshed LCD stripe; no stale ETCH branding remains here. */
 for(uint32_t y=82u;y<160u;y++)if(y>=y0&&y<y0+height)
  for(uint32_t x=77u;x<163u;x++)dst[(y-y0)*240u+x]=etch_matrix_pixel(0x0000u);
 const uint32_t *glyph=beetle_play_icon[frame&1u];
 for(uint32_t row=0u;row<23u;row++)for(uint32_t col=0u;col<22u;col++){
  if(!(glyph[row]&(1u<<(21u-col))))continue;
  for(uint32_t sy=0u;sy<3u;sy++){
   uint32_t py=86u+row*3u+sy;
   if(py<y0||py>=y0+height)continue;
   for(uint32_t sx=0u;sx<3u;sx++)dst[(py-y0)*240u+87u+col*3u+sx]=etch_matrix_pixel(0xF81Au);
  }
 }
 /* This application-owned progress display is independent of protected ROM. */
 const char *status=error<0?"OTA FAIL":"BEETLE OTA";
 uint32_t len=0u;while(status[len])len++;
 for(uint32_t i=0u;i<len;i++)etch_matrix_char(dst,y0,height,status[i],
  (int32_t)(120u-(len*7u)/2u+i*7u),215,1u,error<0?0xf800u:0xF81Au);
 etch_matrix_char(dst,y0,height,(char)('0'+(step%10u)),193,215,1u,0xffffu);
}
