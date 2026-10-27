/* SPDX-License-Identifier: GPL-3.0-only
 * BEETLE physical LOAD-project delete: EDIT (matrix button 6) + SEL (3).
 * All actions are based on the entire debounced scan history, NOT just the
 * instantaneous held mask. A quick modifier tap can begin and end between
 * 18ms UI polls, leaving pressed/released edges but no held bit. In previous
 * firmware that edge was ignored, so a pending SEL could still open NEW.
 */
#pragma once
#include <stdint.h>
#define BEETLE_CONFIRM_SEL (1u<<3)
#define BEETLE_DELETE_GLO (1u<<7)
#define BEETLE_DELETE_EDIT (1u<<6)
#define BEETLE_DELETE_CHORD (BEETLE_CONFIRM_SEL|BEETLE_DELETE_EDIT)
#define BEETLE_BOOT_SELECT_SETTLE_MS 180u
#define BEETLE_LOAD_SELECT_SETTLE_MS 110u

static inline uint8_t beetle_delete_chord_released(uint32_t held){
 return (uint8_t)((held&BEETLE_DELETE_CHORD)==0u);
}
static inline uint8_t beetle_delete_hotkey(uint8_t page,uint8_t slot_status,
                                            uint32_t held,uint32_t newly_pressed,
                                            uint8_t armed){
 (void)newly_pressed;
 return (uint8_t)(page==14u && armed && slot_status==1u &&
   (held&BEETLE_DELETE_CHORD)==BEETLE_DELETE_CHORD);
}

/* A plain SEL confirmation becomes valid ONLY after it has been released
 * cleanly and no other button appeared (held OR either edge) throughout the
 * entire press/release transaction and the subsequent quiet window. A chord
 * never commits NEW or LOAD, even if the modifier was a sub-poll tap.
 * The quiet window also catches a late debounced EDIT edge after SEL-up.
 */
struct beetle_select_transaction {
 uint32_t released_at;
 uint8_t pending;
 uint8_t settling;
};
typedef struct beetle_select_transaction beetle_boot_select_gate;
static inline void beetle_select_reset(struct beetle_select_transaction *g){
 g->pending=0u;g->settling=0u;g->released_at=0u;
}
static inline uint8_t beetle_select_confirm(struct beetle_select_transaction *g,
        uint32_t now,uint32_t held,uint32_t pressed,uint32_t released,
        uint32_t quiet_ms){
 /* Held modifiers and either edge count. Importantly: EDIT press+release
  * between polls must revoke an already armed SEL transaction. */
 if((held|pressed|released)&~BEETLE_CONFIRM_SEL){
  beetle_select_reset(g);return 0u;
 }
 if(g->settling){
  /* Only a completely idle button scan is an eligible settling scan. */
  if((held|pressed|released)!=0u){beetle_select_reset(g);return 0u;}
  if((uint32_t)(now-g->released_at)>=quiet_ms){
   beetle_select_reset(g);return 1u;
  }
  return 0u;
 }
 if(pressed&BEETLE_CONFIRM_SEL)g->pending=1u;
 if(g->pending && (released&BEETLE_CONFIRM_SEL) &&
    !(held&BEETLE_CONFIRM_SEL)){
  g->pending=0u;g->settling=1u;g->released_at=now;
 }
 return 0u;
}
static inline void beetle_boot_select_reset(struct beetle_select_transaction *g){
 beetle_select_reset(g);
}
static inline uint8_t beetle_boot_select_confirm(struct beetle_select_transaction *g,
        uint32_t now,uint32_t held,uint32_t pressed,uint32_t released){
 return beetle_select_confirm(g,now,held,pressed,released,BEETLE_BOOT_SELECT_SETTLE_MS);
}
static inline uint8_t beetle_load_select_confirm(struct beetle_select_transaction *g,
        uint32_t now,uint32_t held,uint32_t pressed,uint32_t released){
 return beetle_select_confirm(g,now,held,pressed,released,BEETLE_LOAD_SELECT_SETTLE_MS);
}
