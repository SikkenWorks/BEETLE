#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "native_ota_matrix.h"
static uint16_t screen[240*240];
static uint16_t canvas[240*120];
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
 for(uint32_t y=0;y<etch_scope_height;y++)for(uint32_t x=0;x<240;x++)screen[(etch_scope_origin+y)*240+x]=canvas[y*240+x];
 etch_panel_composite=0;etch_scope_active=0;
}
#include "track_boot_splash.h"
int main(void){
 uint16_t screens[2][240*240];
 for(int frame=0;frame<2;frame++){
  memset(screen,0,sizeof screen);
  beetle_boot_art(frame);
  memcpy(screens[frame],screen,sizeof screen);
  assert(invalid_draw==0);
  uint32_t pixels=0,tree=0;for(uint32_t i=0;i<240*240;i++)if(screen[i]){
   if(screen[i]==0xF81E)pixels++;else tree++;
  }
  assert(pixels>1600 && tree>4900);
  printf("boot frame %d: %u magenta / %u tree pixels\n",frame,pixels,tree);
 }
 assert(memcmp(screens[0],screens[1],sizeof screen)!=0);
 for(int frame=0;frame<2;frame++){
  uint16_t matrix[240*240];
  for(uint32_t band=0;band<6;band++){
   static uint16_t block[240*40];
   etch_matrix_band(block,band*40,40,frame,3,0);
   memcpy(matrix+band*40*240,block,sizeof block);
  }
  uint32_t iconpink=0;
  for(int y=86;y<155;y++)for(int x=87;x<153;x++)if(matrix[y*240+x]==etch_matrix_pixel(0xF81Au))iconpink++;
  assert(iconpink>200);printf("OTA frame %d play-icon pink pixels: %u\n",frame,iconpink);
 }
 printf("PASS boot OTA both animations correct\n");
 return 0;
}
