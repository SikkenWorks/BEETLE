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
 "HELP: K1 SCROLLS ONLY",
 "K4 BPM  SEQ+K4 PAT LEN 1-16",
 "PLAY+ALG: QUEUE NEXT PATTERN",
 "SELECT: CHANNEL 1-6",
 "ALGORITHM: PATTERN 1-48",
 "PRESET: NOTE / EMPTY / STP",
 "ENV: INSERT STOP [STP]",
 "DRUM NOTE2+ENV: NOTE2 STP",
 "SEL: CLEAR SELECTED STEP",
 "KEYS WRITE NOTE FROM ANY CELL",
 "SEQ+OCT-/+ STEP NOTE +/-12",
 "OCT LED: LIVE KEY OCT SHIFT",
 "SEQ+OP1: COPY ONE LANE",
 "SEQ+OP2: PASTE ONE LANE",
 "SEQ+OP3: COPY WHOLE PATTERN",
 "SEQ+OP4: PASTE PATTERN + PUNCH FX",
 "SEQ+OP5: UNDO ALL EDITORS",
 "SEQ+OP6: REDO ALL EDITORS",
 "SEQ+SEL: BLOCK EDIT MENU",
 "SEQ+FX: STEP VELOCITY/GATE",
 "SEQ+ENV: STEP GROOVE EDIT",
 "GROOVE K1 STEP K2 PARAM K3 VAL",
 "MICRO 0-7/64 STEP (LATE)",
 "RATCHET 1-4 PROB 100/75/50/25",
 "IN GROOVE: SEQ+OP1 COPY STEP",
 "IN GROOVE: SEQ+OP2 PASTE STEP",
 "IN GROOVE: SEQ+OP3 FILL LANE",
 "IN GROOVE: SEQ+OP4 CLEAR LANE",
 "PRESET NOTE2: DRUM/---/STP",
 "CH6 NOTE2 KEY: SECOND HIT",
 "DYNAMICS K1 STEP K2 VEL/GATE",
 "DYNAMICS K3 VALUE SEL RESET",
 "GATE 0 LEGATO, 1-15 SHORT",
 "BLOCK: K4 START, K1 END",
 "BLOCK: K2 ACTION K3 AMOUNT",
 "BLOCK: SEL APPLY",
 "# INSTRUMENT / KEYBOARD",
 "EDIT: INSTRUMENT SETTINGS",
 "K1 ROW  K2 PAGE  K3 VALUE",
 "OCT - / +: LIVE KEYBOARD",
 "EDIT: BOTH OCT = MONO/POLY",
 "EDIT: SEQ+EDIT LIBRARY",
 "LIBRARY: 8 PROJECT PATCHES",
 "LIB: K1 SLOT K2 ACTION",
 "LIB: K3 ACTIVE INST 1-4",
 "LIB: K4 COPY DEST SLOT",
 "LIB: SEL STORE/LOAD/COPY/NAME",
 "NAME: K1 POS K2 CHAR SEL SAVE",
 "LIB: HOME EDIT SEQ TRACKER",
 "UNDO: PATCH / LIBRARY CHANGES",
 "UNDO: MASTER / MIXER CHANGES",
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
 "HOLD FX KEY + K1: CHANGE FX",
 "REPEAT/RING/FILTER/TAPE: K1 +/-1",
 "RING MOD DEFAULT 20",
 "STUTTER DEFAULT BPM X1",
 "REC + PLAY: RECORD PUNCH FX",
 "# SONG / MIXER / MASTER",
 "ARP: LIVE WHEN SONG ON",
 "LIVE MIXER: BLACK 1-6 MUTE",
 "WHITE KEYS BELOW: SOLO 1-6",
 "SOLO KEY LED = SOLO ACTIVE",
 "LIVE SOLO: [X] S / [Q S]",
 "SONG OFF: LIVE MUTES/SOLOS BYPASS",
 "OCT-: Q MUTE ON/OFF",
 "OCT+: Q SOLO ON/OFF",
 "Q ON: NEXT PAT; OFF: DIRECT",
 "NORMAL MIXER Q DEFAULT OFF",
 "MIXER VOLUME: 0-120%",
 "UNDO: SONG ARRANGEMENT EDITS",
 "GLO: MIXER",
 "MIXER SEL: QUANT MUTES ON/OFF",
 "MIXER SEQ+SEL: PERFORMANCE",
 "SNAPS: K1 SLOT  K2 SAVE",
 "SNAPS: K3/SEL RECALL",
 "SNAPS: FX MUTE GROUPS",
 "GROUPS: K1 GROUP K2 MEMBER",
 "GROUPS: SEL ADD/REMOVE LANE",
 "GROUPS: K3 MUTE/UNMUTE ALL",
 "GROUPS: HOME TO SNAPSHOTS",
 "GROUPS FOLLOW QUANT MUTES",
 "SNAPS: HOME MIXER SEQ TRACKER",
 "SLOT COLORS: PINK MUTED WHITE ON",
 "Q ON: PLAY MUTES AT NEXT BAR",
 "Q OFF: MUTES APPLY DIRECTLY",
 "Q1 PENDING MUTE / Q0 UNMUTE",
 "SWITCH OFF: APPLY QUEUED NOW",
 "STOP CANCELS PENDING MUTES",
 "LFO: MASTER EQ / FX",
 "MASTER SELECT: FX SCROLL",
 "MASTER SEL: FX ON/OFF",
 "MASTER K1-K4: PARAMETERS",
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
/* Jump directly between real chapter headings, not arbitrary pages. */
static inline uint8_t beetle_help_heading(uint8_t current,int32_t detents){
 int8_t dir=(detents>0)?1:-1;
 while(detents){
  uint8_t found=current;uint8_t got=0u;
  if(dir>0){
   for(uint8_t i=(uint8_t)(current+1u);i<BEETLE_HELP_COUNT;i++)
    if(beetle_help_lines[i][0]=='#'){found=i;got=1u;break;}
  }else{
   for(int32_t i=(int32_t)current-1;i>=0;i--)
    if(beetle_help_lines[i][0]=='#'){found=(uint8_t)i;got=1u;break;}
  }
  if(got){current=found>beetle_help_last_offset()?beetle_help_last_offset():found;}
  else if(dir>0)current=beetle_help_last_offset();else current=0u;
  if(detents>0)detents--;else detents++;
 }
 return current;
}
static inline uint8_t beetle_help_scroll(uint8_t current,int32_t delta){
 int32_t next=(int32_t)current+delta;
 if(next<0)next=0;
 if(next>beetle_help_last_offset())next=beetle_help_last_offset();
 return (uint8_t)next;
}
#endif
