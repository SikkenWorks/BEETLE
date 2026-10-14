/* SPDX-License-Identifier: GPL-3.0-only
 * BEETLE V0.61: consume SEQ+K4 before the ordinary BPM handler.
 * Pure helpers allow input regression testing without target hardware.
 */
#ifndef BEETLE_PATTERN_LENGTH_SHORTCUT_H
#define BEETLE_PATTERN_LENGTH_SHORTCUT_H
#include <stdint.h>
static inline uint8_t beetle_pattern_length_chord(uint8_t page,uint32_t held,
 uint32_t released,int32_t delta){
 return page==0u && delta!=0 && ((held|released)&(1u<<11))!=0u;
}
static inline uint8_t beetle_pattern_length_target(uint8_t length,int32_t delta){
 int32_t value=(int32_t)length+delta;
 if(value<1)value=1;
 if(value>16)value=16;
 return (uint8_t)value;
}
#endif
