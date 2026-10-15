/* SPDX-License-Identifier: GPL-3.0-only
 * Exact rendered RGB565 boot-splash regression against user's GIF frames.
 * Emits 240x240 uint16 frames for Python per-pixel comparison.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
static uint16_t screen[240*240],canvas[240*120];
static uint16_t etch_scope_origin,etch_scope_height;
static uint8_t etch_scope_active,etch_panel_composite;
static uint32_t invalid_draw;
static void lcd_fill(uint32_t x,uint32_t y,uint32_t w,uint32_t h,uint16_t color){
 if(x>=240||y>=240||w>240-x||h>240-y){invalid_draw++;return;}
 for(uint32_t yy=y;yy<y+h;yy++)for(uint32_t xx=x;xx<x+w;xx++){
  if(etch_panel_composite && (yy<etch_scope_origin||yy>=etch_scope_origin+etch_scope_height))continue;
  if(etch_panel_composite)canvas[(yy-etch_scope_origin)*240+xx]=color;
  else screen[yy*240+xx]=color;
 }
}
static void etch_panel_commit(void){
 for(uint32_t y=0;y<etch_scope_height;y++)for(uint32_t x=0;x<240;x++)
  screen[(etch_scope_origin+y)*240+x]=canvas[y*240+x];
 etch_panel_composite=0;etch_scope_active=0;
}
#include "track_boot_splash.h"
int main(int argc,char **argv){
 assert(argc==2);
 FILE *out=fopen(argv[1],"wb");assert(out);
 uint16_t first[240*240];
 for(int frame=0;frame<2;frame++){
  memset(screen,0xAA,sizeof screen);
  beetle_boot_art((uint8_t)frame);
  if(frame==0)memcpy(first,screen,sizeof screen);
  else assert(memcmp(first,screen,sizeof screen)!=0);
  assert(fwrite(screen,sizeof(uint16_t),240*240,out)==240*240);
  uint32_t pink=0,tree=0;
  for(uint32_t i=0;i<240*240;i++){
   pink+=(screen[i]==0xF81Eu);tree+=(screen[i] && screen[i]!=0xF81Eu);
  }
  printf("frame %d: pink=%u tree=%u\n",frame,pink,tree);
  assert(pink>1600 && tree>4900);
 }
 fclose(out);assert(invalid_draw==0);
 puts("PASS: native 240x240 two-frame splash with safe stripe coordinates");
 return 0;
}
