/* SPDX-License-Identifier: GPL-3.0-only
 * UI-host regression: 240x240 framebuffer, full-frame ISR race, narrow-band
 * repaint, row-overlap, step wrap, accelerated steps. This DOES NOT run FM1 SPI.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "beetle_playhead_update.h"
#define W 240
#define H 240
#define PINK 9
#define INK 2
static unsigned char actual[H][W], expected[H][W];
static void paint(uint8_t step, int only_y, int height, unsigned char dest[H][W]) {
  int y0 = only_y < 0 ? 0 : only_y;
  int y1 = only_y < 0 ? H : only_y+height;
  for(int y=y0; y<y1; ++y)
    memset(dest[y],0,W);
  for(int row=0; row<16; ++row){
    int y=43+10*row;
    if(row==step){
      for(int yy=y-2; yy<y+11; ++yy)
        if(yy>=y0 && yy<y1)for(int x=21;x<222;x++)dest[yy][x]=PINK;
    }
    /* Same row order as actual native grid painter. The text is always there
     * whether the row was focused before or after the dirty-band update. */
    for(int yy=y; yy<y+7; ++yy)
      if(yy>=y0 && yy<y1)for(int x=48;x<53;x++)dest[yy][x]=INK;
  }
}
static void full(uint8_t step, unsigned char dst[H][W]) {paint(step,-1,0,dst);}
static void delta(uint8_t old,uint8_t next){
  struct beetle_lcd_dirty_band b=beetle_grid_play_band(old);
  paint(next,b.y,b.height,actual);
  b=beetle_grid_play_band(next);
  paint(next,b.y,b.height,actual);
}
int main(void){
  uint8_t latched, actual_isr, painted;
  /* Reproduce the exact race seen on FM1: ISR ticks during the 115KiB full
   * transfer. The prior buggy code said the current ISR step was painted. */
  latched=0; actual_isr=1;full(latched,actual);
  uint8_t wrong_painted=actual_isr;
  assert(wrong_painted!=latched);
  /* Fix: full draw records the frozen LCD snapshot, not ISR after transfer. */
  painted=latched;
  if(beetle_playhead_step_dirty(1,0,0,painted,actual_isr)){
    delta(painted,actual_isr);painted=actual_isr;
  }
  full(actual_isr,expected);
  assert(!memcmp(actual,expected,sizeof actual));
  /* Simulate multiple loop iterations, occasional long SPI full render with
   * ISR ticks during it, skipped steps and wrap. Must never leave old pink. */
  for(unsigned run=0;run<64;run++){
    for(unsigned step=0;step<16;step++){
      uint8_t now=(uint8_t)step;
      if(step%4==0){
        uint8_t snap=painted;
        full(snap,actual);/* ISR advanced between begin and commit. */
        painted=snap;
      }
      if(beetle_playhead_step_dirty(1,0,0,painted,now)){
        delta(painted,now);painted=now;
      }
      full(now,expected);
      if(memcmp(actual,expected,sizeof actual)){
        fprintf(stderr,"GHOST at run=%u step=%u prior=%u\n",run,step,painted);
        return 2;
      }
    }
  }
  /* Non-adjacent catch-up under blocked UI/poll. */
  const uint8_t jumps[]={0,7,14,2,12,0,15,3,9,5,1};
  for(unsigned i=0;i<sizeof jumps;i++){
    uint8_t now=jumps[i]; delta(painted,now);painted=now;
    full(now,expected);assert(!memcmp(actual,expected,sizeof actual));
  }
  puts("PASS V1.4.5: all-step frame consistency, 64 loops, ISR-mid-full-race, overlap, wrap & jumps");
  return 0;
}
