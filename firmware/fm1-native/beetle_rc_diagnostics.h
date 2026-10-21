/* SPDX-License-Identifier: GPL-3.0-only
 * BEETLE V0.73 read-only, bounded ISR telemetry.
 * Tick counts are TIMER4 ticks, not calibrated CPU percent or proven DMA safety.
 * No heap, storage, USB writes or slow division in an interrupt.
 */
#pragma once
#include <stdint.h>
#include "native_audio_timing.h"
#define BEETLE_RC_BLOCK_TICKS (128u*544u)
/* LCD is a main-loop SPI DMA client, competing for memory bandwidth with
 * ALNK. Retain full step-timed accuracy of the playhead model; render only a
 * fresh snapshot when the measured audio pressure allows it. The governor
 * reacts to actual DMA/IRQ pressure, not just how many notes are playing. */
static inline uint16_t beetle_lcd_min_interval(uint8_t pressure,uint8_t playing){
 if(!playing)return 0u;
 if(pressure>=20u)return 380u;
 if(pressure>=9u)return 210u;
 if(pressure>=3u)return 125u;
 return 0u;
}
struct beetle_rc_monitor {
 uint32_t blocks, guard, late, wall_85, wall_95;
 uint8_t last_late_song_pos; /* 1..64, 0 outside Song Mode */
 uint32_t max_wall, max_render, max_nested;
 uint32_t midi_rx, midi_clock, midi_overflows;
 uint32_t clock_previous_ms;
 uint16_t clock_min_ms, clock_max_ms, clock_last_ms;
 uint8_t clock_seen, clock_sampled;
};
static inline void beetle_rc_inc(uint32_t *v){if(*v<UINT32_MAX)++*v;}
/* Updated only by audio IRQ after actual preflight/missed deadline, never
 * inferred from clipping or a desktop render. */
static inline void beetle_rc_song_deadline(struct beetle_rc_monitor *m,uint8_t pos){
 if(m)m->last_late_song_pos=pos;
}
static inline void beetle_rc_guard(struct beetle_rc_monitor *m){
 if(m)beetle_rc_inc(&m->guard);
}
static inline void beetle_rc_audio(struct beetle_rc_monitor *m,struct etch_audio_timing t){
 if(!m)return;
 beetle_rc_inc(&m->blocks);
 if(t.late)beetle_rc_inc(&m->late);
 if(t.wall_ticks>=BEETLE_RC_BLOCK_TICKS*85u/100u)beetle_rc_inc(&m->wall_85);
 if(t.wall_ticks>=BEETLE_RC_BLOCK_TICKS*95u/100u)beetle_rc_inc(&m->wall_95);
 if(t.wall_ticks>m->max_wall)m->max_wall=t.wall_ticks;
 if(t.render_ticks>m->max_render)m->max_render=t.render_ticks;
 if(t.nested_ticks>m->max_nested)m->max_nested=t.nested_ticks;
}
/* Diagnostic of *received* USB/TRS MIDI packet timestamps (millisecond
 * resolution). This is not measured MIDI-to-audio latency, jitter or clock sync.
 * A gap >500ms invalidates the previous clock interval. The first tick
 * establishes a baseline; no invented zero-length interval is recorded. */
static inline void beetle_rc_midi(struct beetle_rc_monitor *m,uint8_t status,uint32_t ms){
 if(!m)return;
 beetle_rc_inc(&m->midi_rx);
 if(status!=0xf8u)return;
 beetle_rc_inc(&m->midi_clock);
 if(m->clock_seen){
  uint32_t gap=ms-m->clock_previous_ms;
  if(gap<=500u){
   uint16_t d=(uint16_t)gap;
   m->clock_last_ms=d;
   if(!m->clock_sampled || d<m->clock_min_ms)m->clock_min_ms=d;
   if(!m->clock_sampled || d>m->clock_max_ms)m->clock_max_ms=d;
   m->clock_sampled=1u;
  }else{
   m->clock_min_ms=0;m->clock_max_ms=0;m->clock_last_ms=0;m->clock_sampled=0u;
  }
 }
 m->clock_previous_ms=ms;m->clock_seen=1u;
}
/* Round to one tenth of a percent, deliberately cap at 999.9%. */
static inline uint32_t beetle_rc_tenths(uint32_t ticks){
 uint64_t v=((uint64_t)ticks*1000u+BEETLE_RC_BLOCK_TICKS/2u)/BEETLE_RC_BLOCK_TICKS;
 return (uint32_t)(v>9999u?9999u:v);
}
