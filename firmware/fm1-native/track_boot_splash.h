/* SPDX-License-Identifier: GPL-3.0-only
 * BEETLE V0.58: exact user-provided moon/tree 240x240 backdrop.
 * Original two-frame beetle is composited above the fixed moon/tree.
 * 100 ms/frame, repeating until the tracker screen is ready.
 * All drawing happens in the normal two-stripe LCD compositor. No heap.
 */
#pragma once
#include <stdint.h>
#include "beetle_boot_frames.h"
#include "beetle_play_icon.h" /* still used by PLAY/STOP indicator, not splash */

static inline void beetle_boot_art_rows(uint8_t frame,uint16_t start,uint16_t end){
 lcd_fill(0u,start,240u,(uint16_t)(end-start),0x0000u);
 for(uint32_t i=0;i<BEETLE_BOOT_TREE_RUNS;i++){
  const struct beetle_boot_base_run *r=&beetle_boot_tree[i];
  if(r->y<start||r->y>=end)continue;
  lcd_fill(r->x,r->y,r->w,1u,beetle_boot_colors[r->color]);
 }
 const struct beetle_boot_pink_run *bug=(frame&1u)?beetle_boot_bug1:beetle_boot_bug0;
 uint32_t count=(frame&1u)?BEETLE_BOOT_BUG1_RUNS:BEETLE_BOOT_BUG0_RUNS;
 for(uint32_t i=0u;i<count;i++){
  const struct beetle_boot_pink_run *r=&bug[i];
  if(r->y<start||r->y>=end)continue;
  lcd_fill(r->x,r->y,r->w,1u,0xF81Eu); /* original #FF00F0 */
 }
}
static inline void beetle_boot_art(uint8_t frame){
 for(uint16_t top=0u;top<240u;top+=120u){
  etch_scope_origin=top;etch_scope_height=120u;
  etch_scope_active=1u;etch_panel_composite=1u;
  beetle_boot_art_rows(frame,top,(uint16_t)(top+120u));
  etch_panel_commit();
 }
}
