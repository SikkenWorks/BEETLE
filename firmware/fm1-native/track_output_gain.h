/* BEETLE master: all chip, drum and punch signals pass here, immediately before DMA.
 * Pure Q16 helper is also host-testable. No malloc/float or dynamic allocation.
 */
#ifndef BEETLE_TRACK_OUTPUT_GAIN_H
#define BEETLE_TRACK_OUTPUT_GAIN_H
#include <stdint.h>
static inline int16_t track_master_apply(int16_t sample,uint16_t knob,int32_t *smoothed_q16){
 if(!smoothed_q16)return 0;
 if(knob>32767u)knob=32767u;
 const int32_t limit=(int32_t)((uint32_t)knob<<16u);
 int32_t value=*smoothed_q16;
 if(value<0)value=0;
 if(value>limit)value=limit; /* never exceed physical MASTER, even in ramp */
 else if(value<limit){
  int32_t step=(limit-value)/96;
  if(step<1)step=1;
  value+=step;
  if(value>limit)value=limit;
 }
 *smoothed_q16=value;
 return (int16_t)(((int64_t)sample*(value>>16u))/32767);
}
#endif
