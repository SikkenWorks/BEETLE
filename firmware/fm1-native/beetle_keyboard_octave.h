/* BEETLE V0.51: independent global performance-keyboard octave. GPL-3.0-only. */
#pragma once
#include <stdint.h>
/* FM1 key 0 is MIDI F3 = 53. Valid octave shift -4 to +3. */
static inline int8_t beetle_keyboard_shift(int8_t current, int direction){
 int n=(int)current+direction;
 return (int8_t)(n< -4?-4:n>3?3:n);
}
static inline uint8_t beetle_keyboard_pitch(uint8_t key, int8_t octave){
 int32_t n=53+(int32_t)key+12*(int32_t)octave;
 return (uint8_t)(n<0?0:n>127?127:n);
}
