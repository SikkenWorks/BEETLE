/* BEETLE V0.73 telemetry contract: runs on host only. */
#include "beetle_rc_diagnostics.h"
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static void test_audio(void){
 struct beetle_rc_monitor m={0};
 struct etch_audio_timing a=etch_audio_timing_end(0xfffffff0u,0x20u,20u,BEETLE_RC_BLOCK_TICKS,0u);
 assert(a.wall_ticks==48u && a.nested_ticks==20u && a.render_ticks==28u && !a.late);
 beetle_rc_audio(&m,a);
 assert(m.blocks==1u && !m.wall_85 && !m.late && m.max_render==28u);
 a=etch_audio_timing_end(0u,BEETLE_RC_BLOCK_TICKS*90u/100u,1000000u,BEETLE_RC_BLOCK_TICKS,0u);
 assert(a.nested_ticks==a.wall_ticks && !a.render_ticks);
 beetle_rc_audio(&m,a);
 assert(m.wall_85==1u && m.wall_95==0u && m.max_nested==a.wall_ticks);
 a=etch_audio_timing_end(0u,BEETLE_RC_BLOCK_TICKS-1u,0u,BEETLE_RC_BLOCK_TICKS,1u);
 assert(a.late);beetle_rc_audio(&m,a);
 assert(m.wall_95==1u && m.late==1u && m.blocks==3u);
 beetle_rc_guard(&m);assert(m.guard==1u);
 assert(beetle_rc_tenths(0u)==0u);
 assert(beetle_rc_tenths(BEETLE_RC_BLOCK_TICKS)==1000u);
 assert(beetle_rc_tenths(UINT32_MAX)==9999u);
 m.guard=UINT32_MAX;beetle_rc_guard(&m);assert(m.guard==UINT32_MAX);
 m.blocks=UINT32_MAX;beetle_rc_audio(&m,a);assert(m.blocks==UINT32_MAX);
}
static void test_midi(void){
 struct beetle_rc_monitor m={0};
 beetle_rc_midi(&m,0x90u,1234u);assert(m.midi_rx==1 && !m.midi_clock);
 beetle_rc_midi(&m,0xF8u,0xfffffffcu);
 assert(m.midi_clock==1u && !m.clock_sampled);
 beetle_rc_midi(&m,0xF8u,0u);assert(m.clock_sampled && m.clock_min_ms==4u);
 beetle_rc_midi(&m,0xF8u,0u);assert(m.clock_min_ms==0u && m.clock_max_ms==4u);
 beetle_rc_midi(&m,0xF8u,20u);assert(m.clock_min_ms==0u && m.clock_max_ms==20u);
 beetle_rc_midi(&m,0xF8u,1021u);assert(!m.clock_sampled && m.clock_last_ms==0u);
 beetle_rc_midi(&m,0xF8u,1040u);assert(m.clock_min_ms==19u && m.clock_max_ms==19u);
 beetle_rc_inc(&m.midi_overflows);assert(m.midi_overflows==1u);
 assert(m.midi_rx==7u && m.midi_clock==6u);
}
int main(void){test_audio();test_midi();puts("PASS V0.73 read-only ISR audio & MIDI telemetry");return 0;}
