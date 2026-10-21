/* SPDX-License-Identifier: GPL-3.0-only
 * BEETLE 1.4.4: sample-clock playhead dirty regions for 240x240 ST7789.
 * UI thread only; never allocate, block or draw from an audio IRQ.
 */
#ifndef BEETLE_PLAYHEAD_UPDATE_H
#define BEETLE_PLAYHEAD_UPDATE_H
#include <stdint.h>
struct beetle_lcd_dirty_band {uint16_t y, height;};
/* Coverage matches original TRACKER: highlighted step from y-2 through y+10.
 * Redrawing from the existing renderer preserves all cell text and overlays. */
static inline struct beetle_lcd_dirty_band beetle_grid_play_band(uint8_t step){
 struct beetle_lcd_dirty_band b={(uint16_t)(41u+10u*(step&15u)),13u};
 return b;
}
/* Song highlight uses y-2 through y+12; 8 visible rows per page. */
static inline struct beetle_lcd_dirty_band beetle_song_play_band(uint8_t pos){
 struct beetle_lcd_dirty_band b={(uint16_t)(69u+15u*(pos&7u)),15u};
 return b;
}
static inline uint8_t beetle_playhead_step_dirty(uint8_t playing,uint8_t page,
    uint8_t painted_page,uint8_t last_step,uint8_t step){
 return (uint8_t)(playing && page==0u && painted_page==0u && last_step!=step);
}
static inline uint8_t beetle_song_fast_row_dirty(uint8_t playing,uint8_t page,
    uint8_t painted_page,uint8_t enabled,uint8_t prev_pos,uint8_t pos){
 return (uint8_t)(playing&&page==9u&&painted_page==9u&&enabled&&
     prev_pos!=pos&&((prev_pos>>3u)==(pos>>3u)));
}
#endif
