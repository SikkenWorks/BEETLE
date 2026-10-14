/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "track_mixer.h"
#include "track_punch_events.h"
int main(void){
 struct track_mixer m;track_mixer_init(&m);
 assert(m.row==0&&m.col==0&&m.ch[0].volume==80);
 track_mixer_move(&m,1,1);assert(m.row==1&&m.col==1);
 track_mixer_change(&m,-40);assert(m.ch[1].pan==-40);
 track_mixer_move(&m,-8,-8);assert(m.row==0&&m.col==0);
 track_mixer_change(&m,10000);assert(m.ch[0].volume==100);
 track_mixer_change(&m,-10000);assert(m.ch[0].volume==0);
 track_mixer_change(&m,80);assert(track_mixer_velocity(&m,0,100)==80);
 track_mixer_move(&m,0,2);track_mixer_change(&m,1);
 assert(track_mixer_velocity(&m,0,100)==0);
 track_mixer_change(&m,-1);assert(track_mixer_velocity(&m,0,100)==80);
 track_mixer_move(&m,0,1);track_mixer_change(&m,1);
 assert(track_mixer_velocity(&m,1,100)==0);
 assert(track_mixer_velocity(&m,0,100)==80);
 struct track_punch_events events;track_punch_clear(&events);
 assert(track_punch_add(&events,0,0,1)==0);
 assert(track_punch_add(&events,0,0,0)==0);
 assert(track_punch_add(&events,16,0,1)==-1);
 assert(track_punch_add(&events,0,8,1)==-1);
 assert(events.events[1].down==0);
 for(unsigned i=events.count;i<TRACK_MAX_PUNCH_EVENTS;i++)assert(track_punch_add(&events,i%16,i%8,i%2)==0);
 assert(track_punch_add(&events,0,0,0)==-2 && events.overflow);
 puts("TRACK V0.3: mixer, punch-event limits, mute/solo and values PASS");return 0;
}
