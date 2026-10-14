/* Native tracker UI rules. STEP has NO focusable value: future multi-select only.
 * The NOTE/INST/CHD/FX columns are 0/1/2/3, no STEP column cursor.
 */
#ifndef TRACK_NAVIGATION_H
#define TRACK_NAVIGATION_H
#include <stdint.h>
#define TRACK_NAV_COLUMNS 4u
static inline uint8_t track_step_display_number(uint8_t step){return step&15u;}
static inline uint8_t track_column_move(uint8_t current,int32_t detents){
 int32_t col=(int32_t)current+detents;
 if(col<0)col=0;else if(col>=(int32_t)TRACK_NAV_COLUMNS)col=TRACK_NAV_COLUMNS-1;
 return (uint8_t)col;
}
static inline uint8_t track_row_move(uint8_t row,int32_t detents){
 int32_t st=(int32_t)row+detents;
 if(st<0)st=0;else if(st>15)st=15;
 return (uint8_t)st;
}
#endif
