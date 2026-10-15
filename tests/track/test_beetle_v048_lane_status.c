/* SPDX-License-Identifier: GPL-3.0-only */
#include "track_core.h"
#include <stdio.h>
#include <assert.h>
int main(void){
 struct track_engine e;track_init(&e);
 for(unsigned lane=0;lane<6;lane++)assert(!track_lane_has_data(&e.active,(uint8_t)lane));
 assert(!track_lane_has_data(&e.active,6u));
 assert(!track_lane_has_data(0,0u));
 assert(!track_set_note(&e,0u,3u,60u));
 assert(track_lane_has_data(&e.active,0u));
 assert(!track_lane_has_data(&e.active,1u));
 assert(!track_set_note(&e,5u,0u,4u));
 assert(track_lane_has_data(&e.active,5u));
 assert(!track_clear_step(&e,0u,3u));
 assert(!track_lane_has_data(&e.active,0u));
 e.active.ch[1][9].note=TRACK_NOTE_STOP;
 assert(track_lane_has_data(&e.active,1u));
 assert(!track_clear_step(&e,1u,9u));
 e.active.ch[2][4].effect=4;
 assert(track_lane_has_data(&e.active,2u));
 assert(!track_clear_step(&e,2u,4u));
 e.active.ch[3][2].chord=1;
 assert(track_lane_has_data(&e.active,3u));
 assert(!track_clear_step(&e,3u,2u));
 e.active.ch[4][1].instrument=3;
 e.active.ch[4][1].value=20;
 assert(!track_lane_has_data(&e.active,4u));
 assert(track_lane_has_data(&e.active,5u));
 assert(!track_clear_step(&e,5u,0u));
 for(unsigned lane=0;lane<6;lane++)assert(!track_lane_has_data(&e.active,(uint8_t)lane));
 puts("PASS V0.48: six lane occupancy, notes, STP, drum, FX, chord, clear, empty defaults, out of range");return 0;
}
