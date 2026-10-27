/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include "../../firmware/fm1-native/beetle_project_delete_ui.h"
static void boot_sequences(void){
 struct beetle_select_transaction g={0};
 uint32_t sel=BEETLE_CONFIRM_SEL,edit=BEETLE_DELETE_EDIT,glo=BEETLE_DELETE_GLO;
 /* Standalone SEL opens the chosen menu ONLY when physically released. */
 assert(!beetle_boot_select_confirm(&g,0u,sel,sel,0));
 assert(!beetle_boot_select_confirm(&g,0u,sel,0,0));
 assert(!beetle_boot_select_confirm(&g,20u,0,0,sel));
 assert(beetle_boot_select_confirm(&g,200u,0,0,0));
 assert(!beetle_boot_select_confirm(&g,0u,0,0,0));
 /* Extremely short standalone tap fully debounced between UI polls. */
 beetle_boot_select_reset(&g);
 assert(!beetle_boot_select_confirm(&g,20u,0u,sel,sel));
 assert(beetle_boot_select_confirm(&g,200u,0u,0,0));
 /* Chord entirely between two polls must not trigger NEW. */
 beetle_boot_select_reset(&g);
 assert(!beetle_boot_select_confirm(&g,0u,0u,sel|edit,sel|edit));
 /* EDIT first; a SEL press must never become NEW. */
 beetle_boot_select_reset(&g);
 assert(!beetle_boot_select_confirm(&g,0u,edit,edit,0));
 assert(!beetle_boot_select_confirm(&g,0u,edit|sel,sel,0));
 assert(!beetle_boot_select_confirm(&g,0u,edit,0,sel));
 assert(!beetle_boot_select_confirm(&g,0u,0,0,edit));
 /* SEL first; adding EDIT before release must revoke NEW. */
 beetle_boot_select_reset(&g);
 assert(!beetle_boot_select_confirm(&g,0u,sel,sel,0));
 assert(!beetle_boot_select_confirm(&g,0u,sel|edit,edit,0));
 assert(!beetle_boot_select_confirm(&g,0u,edit,0,sel));
 assert(!beetle_boot_select_confirm(&g,0u,0,0,edit));
 /* Even if a noisy SEL edge appears during EDIT+GLO: refuse NEW. */
 beetle_boot_select_reset(&g);
 assert(!beetle_boot_select_confirm(&g,0u,edit|glo|sel,edit|glo|sel,0));
 assert(!beetle_boot_select_confirm(&g,0u,edit|glo,0,sel));
 assert(!beetle_boot_select_confirm(&g,0u,0,0,edit|glo));
}
static void load_sequences(void){
 uint32_t sel=BEETLE_CONFIRM_SEL,edit=BEETLE_DELETE_EDIT,glo=BEETLE_DELETE_GLO;
 assert(BEETLE_DELETE_CHORD==(edit|sel));
 assert(beetle_delete_chord_released(0));
 assert(!beetle_delete_chord_released(sel));
 assert(!beetle_delete_chord_released(edit));
 assert(!beetle_delete_hotkey(14,1,sel,sel,1));
 assert(!beetle_delete_hotkey(14,1,edit,edit,1));
 assert(!beetle_delete_hotkey(14,1,edit|glo,edit|glo,1));
 assert(!beetle_delete_hotkey(29,1,sel|edit,sel|edit,1));
 assert(!beetle_delete_hotkey(14,2,sel|edit,sel|edit,1));
 assert(!beetle_delete_hotkey(14,1,sel|edit,sel|edit,0));
 assert(beetle_delete_hotkey(14,1,sel|edit,edit,1));
 assert(beetle_delete_hotkey(14,1,sel|edit,sel,1));
 assert(beetle_delete_hotkey(14,1,sel|edit,0,1));
 /* modal waits for BOTH keys released before any new SEL confirmation */
 uint8_t gate=1;
 if(beetle_delete_chord_released(sel))gate=0;
 assert(gate);
 if(beetle_delete_chord_released(edit))gate=0;
 assert(gate);
 if(beetle_delete_chord_released(0))gate=0;
 assert(!gate);
}
int main(void){boot_sequences();load_sequences();return 0;}
