/* Native FM1 SETTINGS button transaction. Independent of browser display.
 * SEL is physical matrix button #3. A held press MUST NOT activate the new
 * settings page a second time; arm the toggle only after full release.
 * Host-testable so a short physical press, even if only its trailing edge is
 * observed by the foreground poll, still opens SETTINGS exactly once.
 * SPDX-License-Identifier: GPL-3.0-only
 */
#ifndef BEETLE_SETTINGS_NAV_H
#define BEETLE_SETTINGS_NAV_H
#include <stdint.h>
#define BEETLE_SETTINGS_MENU_INDEX 6u
#define BEETLE_SETTINGS_MENU_PAGE 1u
#define BEETLE_SETTINGS_PAGE 38u
#define BEETLE_SETTINGS_SEL (1ul<<3u)
/* Physical FM1 page validator is called BOTH on each input poll and inside
 * screen(). SETTINGS used page 38 but an older whitelist stopped at 37,
 * so selecting SETTINGS immediately reset the on-device screen to TRACKER.
 * Keep the page whitelist in a testable native header, not the browser. */
static inline uint8_t beetle_ui_page_valid(uint8_t page){
 return (uint8_t)(page<=30u || page==32u || (page>=33u && page<=BEETLE_SETTINGS_PAGE));
}

struct beetle_settings_nav {
 uint8_t menu_held;
 uint8_t toggle_armed;
};
/* Called on every native UI poll while inside the PROJECTS menu; an observed
 * physical SEL hold or a freshly queued rising edge is enough to enter.
 * The release is a fallback for an event which fell between two UI polls.
 */
static inline uint8_t beetle_settings_enter(struct beetle_settings_nav *n,
 uint8_t page,uint8_t index,uint32_t buttons,uint32_t pressed,uint32_t released){
 if(!n)return 0u;
 if(page!=BEETLE_SETTINGS_MENU_PAGE){n->menu_held=0u;return 0u;}
 const uint8_t held=(uint8_t)((buttons&BEETLE_SETTINGS_SEL)!=0u);
 const uint8_t went_down=(uint8_t)((pressed&BEETLE_SETTINGS_SEL)!=0u);
 const uint8_t went_up=(uint8_t)((released&BEETLE_SETTINGS_SEL)!=0u);
 if(index==BEETLE_SETTINGS_MENU_INDEX &&
     (went_down || (went_up && n->menu_held))) {
  n->menu_held=0u;
  n->toggle_armed=0u;
  return 1u;
 }
 n->menu_held=held;
 return 0u;
}
/* SETTINGS is modal: its first release arms the subsequent, separate press.
 * Consumes both edges so neither can become a tracker delete or a menu click.
 */
static inline uint8_t beetle_settings_toggle(struct beetle_settings_nav *n,
 uint32_t buttons,uint32_t pressed,uint32_t released){
 if(!n)return 0u;
 const uint8_t ready=n->toggle_armed;
 if(released&BEETLE_SETTINGS_SEL)n->toggle_armed=1u;
 if(!(buttons&BEETLE_SETTINGS_SEL) && !(pressed&BEETLE_SETTINGS_SEL))n->toggle_armed=1u;
 if((pressed&BEETLE_SETTINGS_SEL)&&ready){
  n->toggle_armed=0u;return 1u;
 }
 return 0u;
}
#endif
