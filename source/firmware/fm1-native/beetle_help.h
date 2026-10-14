/* SPDX-License-Identifier: GPL-3.0-only
 * BEETLE on-device help: fixed-width, <=36 glyphs at 5x7+1 pixel.
 * Headlines use the leading '#' marker; all items are one clipped row.
 */
#ifndef BEETLE_HELP_H
#define BEETLE_HELP_H
#include <stdint.h>
#define BEETLE_HELP_VISIBLE 10u
static const char *const beetle_help_lines[] = {
 "# TRACKER",
 "K1 STEP   K2 CELL  K3 VALUE",
 "K4 BPM IN TRACKER",
 "SELECT: CHANNEL 1-6",
 "ALGORITHM: PATTERN 1-48",
 "PRESET: NOTE / EMPTY / STP",
 "ENV: INSERT STOP [STP]",
 "SEL: CLEAR SELECTED STEP",
 "SEQ+OP1: COPY CURRENT LANE",
 "SEQ+OP2: PASTE CURRENT LANE",
 "COPY IS ONE LANE AT A TIME",
 "# INSTRUMENT / KEYBOARD",
 "EDIT: INSTRUMENT SETTINGS",
 "K1 ROW  K2 PAGE  K3 VALUE",
 "OCT - / +: LIVE KEYBOARD",
 "EDIT: BOTH OCT = MONO/POLY",
 "OCT DOES NOT MOVE SAVED NOTES",
 "# PROJECTS",
 "HOME: PROJECTS MENU",
 "SAVE: SAVE CURRENT PROJECT",
 "SAVE AS: CHOOSE NAME / SLOT",
 "LOAD: SELECT A SAVED SLOT",
 "DISCARD OPENS PROJECT LIST",
 "NEW: START EMPTY PROJECT",
 "SEQ: RETURN TO TRACKER",
 "# LIVE FX / RECORD",
 "PLAY: START OR STOP",
 "REC: ARM PUNCH-IN FX",
 "FX: EDIT STEP EFFECTS",
 "HOLD FX KEY + ALG: CHANGE FX",
 "REC + PLAY: RECORD PUNCH FX",
 "# SONG / MIXER / MASTER",
 "ARP: SONG MODE",
 "GLO: MIXER",
 "LFO: MASTER EQ / FX",
 "MASTER: SELECT FX ROW",
 "ENV: TOGGLE SELECTED FX",
 "K1-K4: MASTER PARAMETERS",
 "SEQ+K4: REORDER MASTER FX",
 "# GUIDE / NAVIGATION",
 "HOLD HOME: SCALE GUIDE",
 "HOME: OPEN PROJECTS MENU",
 "SEQ: BACK TO TRACKER",
 };
#define BEETLE_HELP_COUNT ((uint8_t)(sizeof(beetle_help_lines)/sizeof(beetle_help_lines[0])))
static inline uint8_t beetle_help_last_offset(void){
 return BEETLE_HELP_COUNT>BEETLE_HELP_VISIBLE?(uint8_t)(BEETLE_HELP_COUNT-BEETLE_HELP_VISIBLE):0u;
}
static inline uint8_t beetle_help_scroll(uint8_t current,int32_t delta){
 int32_t next=(int32_t)current+delta;
 if(next<0)next=0;
 if(next>beetle_help_last_offset())next=beetle_help_last_offset();
 return (uint8_t)next;
}
#endif
