/* SPDX-License-Identifier: GPL-3.0-only
 * Post-load input barrier. Synchronous flash operations cannot poll buttons;
 * a held or queued SEL must never be interpreted as an edit on the tracker.
 * Pure helper, with wrap-safe unsigned timestamps, independently host-testable.
 */
#ifndef BEETLE_INPUT_SAFETY_H
#define BEETLE_INPUT_SAFETY_H
#include <stdint.h>
#define BEETLE_SELECT_BUTTON (1u << 3)
#define BEETLE_LOAD_QUIET_MS 320u
struct beetle_load_input_gate {
 uint32_t quiet_start;
 uint8_t active;
 uint8_t needs_release;
};
static inline void beetle_load_input_arm(struct beetle_load_input_gate *g,uint32_t now){
 if(!g)return;
 g->quiet_start=now;
 g->active=1u;
 g->needs_release=1u;
}
static inline void beetle_load_input_filter(struct beetle_load_input_gate *g,
 uint32_t now,uint32_t buttons,uint32_t *pressed,uint32_t *released){
 if(!g||!g->active)return;
 /* Consume SEL edges even if the press was released before polling resumed. */
 if(pressed)*pressed&=~BEETLE_SELECT_BUTTON;
 if(released)*released&=~BEETLE_SELECT_BUTTON;
 if(buttons&BEETLE_SELECT_BUTTON){g->needs_release=1u;g->quiet_start=now;}
 else if(g->needs_release){g->needs_release=0u;g->quiet_start=now;}
 else if((uint32_t)(now-g->quiet_start)>=BEETLE_LOAD_QUIET_MS){g->active=0u;}
}
#endif
