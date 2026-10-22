/* SPDX-License-Identifier: GPL-3.0-only
 * BEETLE V1.4.13: original sample note routing primitives (host tested).
 * 0..13 = factory kit, 14..26 = U01..U13; 255 = no assigned sample.
 * This is mapping only. No storage, DMA, LED, USB or audio callbacks. */
#ifndef BEETLE_GLOBAL_SAMPLE_ROUTE_H
#define BEETLE_GLOBAL_SAMPLE_ROUTE_H
#include <stdint.h>
#define BEETLE_GLOBAL_FIRST_NOTE 14u
#define BEETLE_GLOBAL_SLOT_COUNT 13u
#define BEETLE_GLOBAL_NO_SLOT 255u
static inline uint8_t beetle_global_sample_slot(uint8_t drum_note){
 return (uint8_t)(drum_note>=BEETLE_GLOBAL_FIRST_NOTE&&drum_note<BEETLE_GLOBAL_FIRST_NOTE+BEETLE_GLOBAL_SLOT_COUNT ? drum_note-BEETLE_GLOBAL_FIRST_NOTE : BEETLE_GLOBAL_NO_SLOT);
}
static inline uint8_t beetle_global_sample_triggerable(uint8_t drum_note,uint16_t assigned_mask){
 const uint8_t slot=beetle_global_sample_slot(drum_note);
 return (uint8_t)(slot<BEETLE_GLOBAL_SLOT_COUNT&&((assigned_mask>>slot)&1u));
}
#endif
