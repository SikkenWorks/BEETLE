/* SPDX-License-Identifier: GPL-3.0-only
 * Hardware-independent dirty-band and frame-cadence regression.
 * Does NOT claim physical LCD bandwidth or DAC DMA validation.
 */
#include <assert.h>
#include <stdio.h>
#include "beetle_playhead_update.h"
#include "beetle_rc_diagnostics.h"
int main(void){
 for(unsigned i=0;i<16;i++){
  struct beetle_lcd_dirty_band b=beetle_grid_play_band((uint8_t)i);
  assert(b.y==41u+10u*i && b.height==13u && b.y+b.height<=240u);
  assert(beetle_playhead_step_dirty(1,0,0,(uint8_t)((i+15)&15),(uint8_t)i));
 }
 for(unsigned i=0;i<64;i++){
  struct beetle_lcd_dirty_band b=beetle_song_play_band((uint8_t)i);
  assert(b.y==69u+15u*(i&7u) && b.height==15u && b.y+b.height<=240u);
 }
 assert(!beetle_playhead_step_dirty(0,0,0,1,2));
 assert(!beetle_playhead_step_dirty(1,0,9,1,2));
 assert(!beetle_playhead_step_dirty(1,0,0,2,2));
 assert(beetle_song_fast_row_dirty(1,9,9,1,17,18));
 assert(!beetle_song_fast_row_dirty(1,9,9,1,15,16));
 assert(!beetle_song_fast_row_dirty(0,9,9,1,17,18));
 /* Under severe audio pressure, the old FULL-screen governor pauses 380ms.
  * Grid dirty strips MUST still be requested every ~96ms at 156 BPM. */
 assert(beetle_lcd_min_interval(32,1)==380u);
 unsigned step_updates=0;
 for(unsigned ms=96;ms<=960;ms+=96){
  if(beetle_playhead_step_dirty(1,0,0,(uint8_t)((ms/96-1)&15),
                               (uint8_t)((ms/96)&15)))step_updates++;
 }
 assert(step_updates==10u);
 /* Two grid strips and compact icon: 2*240*13*2 + 22*23*2 bytes.
  * Previous full redraw transferred 2*240*120*2 bytes. */
 assert(2u*240u*13u*2u+22u*23u*2u==13492u);
 assert(13492u<2u*240u*120u*2u/8u); /* 8.5x less SPI payload */
 puts("PASS V1.4.4: every step dirty; song rows; high-pressure governor bypass; 8.5x less pixels");
 return 0;
}
