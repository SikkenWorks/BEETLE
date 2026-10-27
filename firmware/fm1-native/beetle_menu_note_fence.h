/* SPDX-License-Identifier: GPL-3.0-only
 * FM1 panel note-edge handover from NEW/LOAD project dialogs. */
#ifndef BEETLE_MENU_NOTE_FENCE_H
#define BEETLE_MENU_NOTE_FENCE_H
#include <stdint.h>
/* The HAL note queue is consumed separately while modal. A key held across
 * modal exit remains suppressed until its first physical release. */
static inline void beetle_menu_note_capture(uint32_t *blocked,uint32_t held){
 *blocked |= held;
}
static inline uint32_t beetle_menu_note_filter(uint32_t *blocked,uint32_t held,uint32_t press_edges){
 *blocked &= held;
 return press_edges & ~*blocked;
}
#endif
