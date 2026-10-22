/* SPDX-License-Identifier: GPL-3.0-only
 * BEETLE V1.4.13: function key 13 = REC. Main loop only. */
#ifndef BEETLE_REC_LED_H
#define BEETLE_REC_LED_H
#include <stdint.h>
/* Punch-In pages use their independent record arm, every other page uses
 * the user's Tracker note-entry toggle. No LED state is persisted as project
 * data; freshly started tracker sessions default to REC ON. */
static inline uint8_t beetle_rec_led_on(uint8_t page,uint8_t note_entry_on,uint8_t punch_record_on){
 return (uint8_t)(((page==3u||page==4u)?punch_record_on:note_entry_on)!=0u);
}
#endif
