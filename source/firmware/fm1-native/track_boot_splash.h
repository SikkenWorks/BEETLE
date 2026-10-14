/* SPDX-License-Identifier: GPL-3.0-only
 * BEETLE 0.42: intentionally only the play-icon animation at power on.
 * Two 22x23 bitmap frames, scaled exactly x3 via nearest neighbor.
 * No image-run arithmetic, no persistent disk/flash reads.
 */
#pragma once
#include <stdint.h>
#include "beetle_play_icon.h"
static inline void beetle_boot_art_rows(uint8_t frame){
 const uint32_t *glyph=beetle_play_icon[frame&1u];
 const uint16_t size=3u, origin_x=87u, origin_y=85u;
 lcd_fill(0u,0u,240u,240u,0x0000u);
 for(uint16_t y=0u;y<23u;y++){
  const uint32_t bits=glyph[y];
  for(uint16_t x=0u;x<22u;){
   if(!(bits&(1u<<(21u-x)))){x++;continue;}
   uint16_t begin=x;
   while(x<22u&&(bits&(1u<<(21u-x))))x++;
   lcd_fill(origin_x+begin*size,origin_y+y*size,(x-begin)*size,size,0xF81Au);
  }
 }
}
static inline void beetle_boot_art(uint8_t frame){
 for(uint16_t top=0u;top<240u;top+=120u){
  etch_scope_origin=top;etch_scope_height=120u;
  etch_scope_active=1u;etch_panel_composite=1u;
  beetle_boot_art_rows(frame);
  etch_panel_commit();
 }
}
