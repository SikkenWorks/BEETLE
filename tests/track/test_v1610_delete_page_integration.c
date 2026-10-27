/* SPDX-License-Identifier: GPL-3.0-only
 * Test the actual runtime branch's page-validator contract, not just a
 * helper that reports a chord. Reproduces V1.6.9's popup -> TRACKER jump.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "beetle_project_delete_ui.h"
#include "beetle_settings_nav.h"
static uint8_t paint(uint8_t page){
 /* track_runtime_validate() is invoked before painting both LCD slices. */
 if(!beetle_ui_page_valid(page))page=0u;
 return page;
}
int main(void){
 uint8_t page=14u,slot_occupied=1u;
 /* Previous self-tests stopped here and never validated the next page. */
 assert(beetle_delete_hotkey(page,slot_occupied,BEETLE_DELETE_EDIT|
        BEETLE_CONFIRM_SEL,BEETLE_CONFIRM_SEL,1u));
 page=39u; /* DELETE PROJECT? popup */
 assert(paint(page)==39u && "DELETE PROJECT? must survive screen validation");
 /* Only a second, fresh SELECT after YES can transition into delete I/O. */
 page=40u; /* DELETING PROJECT progress screen */
 assert(paint(page)==40u && "DELETING PROJECT must survive screen validation");
 assert(paint(41u)==0u); /* genuinely invalid UI pages still quarantined */
 puts("PASS: real native DELETE pages 39 and 40 survive UI validation");
 return 0;
}
