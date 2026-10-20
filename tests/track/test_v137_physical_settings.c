/* Physical FM1 UI transition / screen-validator regression.
 * Host test of native C page validation, not the WebSimulator state machine. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "beetle_settings_nav.h"
int main(void){
 struct beetle_settings_nav nav={0};
 const uint32_t sel=BEETLE_SETTINGS_SEL;
 uint8_t page=1u, menu=6u;
 assert(beetle_ui_page_valid(0u));
 assert(beetle_ui_page_valid(1u));
 assert(beetle_ui_page_valid(32u));
 assert(beetle_ui_page_valid(37u));
 assert(beetle_ui_page_valid(BEETLE_SETTINGS_PAGE));
 assert(!beetle_ui_page_valid(31u));
 assert(!beetle_ui_page_valid(39u));
 /* Choose menu item 6; a real FM1 matrix button-3 edge must open page 38. */
 assert(beetle_settings_enter(&nav,page,menu,sel,sel,0u));
 page=BEETLE_SETTINGS_PAGE;
 /* The LCD performs this validation again directly after SEL. */
 if(!beetle_ui_page_valid(page))page=0u;
 assert(page==BEETLE_SETTINGS_PAGE);
 assert(!beetle_settings_toggle(&nav,sel,sel,0u));
 assert(!beetle_settings_toggle(&nav,0u,0u,sel));
 assert(!beetle_settings_toggle(&nav,0u,0u,0u));
 assert(beetle_settings_toggle(&nav,sel,sel,0u));
 assert(page==BEETLE_SETTINGS_PAGE);
 assert(!beetle_settings_toggle(&nav,sel,0u,0u));
 assert(!beetle_settings_toggle(&nav,0u,0u,sel));
 /* A new HOME button leads to PROJECTS instead of TRACKER. */
 page=1u;assert(beetle_ui_page_valid(page));
 assert(!beetle_settings_enter(&nav,page,menu,0u,0u,0u));
 /* Other project menu items must not open SETTINGS. */
 assert(!beetle_settings_enter(&nav,1u,4u,sel,sel,0u));
 puts("PASS V1.3.7 native FM1 SETTINGS 38, SEL transaction, HOME");
 return 0;
}
