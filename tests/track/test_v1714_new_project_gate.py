#!/usr/bin/env python3
"""V1.7.14 protected warm NEW: immutable source/route regressions."""
from pathlib import Path
r=Path(__file__).resolve().parents[2]
t=(r/'firmware/fm1-native/track_runtime.h').read_text()
a=(r/'firmware/fm1-native/audio_target.c').read_text()
new=t.split('static void beetle_project_new_session(void){',1)[1].split('static void beetle_project_open_load_browser(void){',1)[0]
save=t.split('static void track_complete_save(void){',1)[1].split('static uint8_t beetle_first_unused_slot_with_owner(int owner){',1)[0]
assert 'BEETLE V1.7.14' in t
assert new.index('beetle_storage_input_lock();')<new.index('native_new_quiesce()')<new.index('track_init(&track_state);')
assert 'if(!track_state.transport.playing &&' in new or '!track_state.transport.playing &&' in new
assert 'if(!warm_new)native_pause();' in new
assert new.index('if(warm_new)native_new_resume();')<new.index('else if(resume_audio)native_start();')
assert 'beetle_new_perf_scan_ms=0u' in new and 'beetle_new_perf_loading_ms=0u' in new
for forbidden in ('beetle_first_unused_slot_with_owner(', 'beetle_hal_probe_ownership_ui(', 'beetle_new_project_loading_indicator(', 'beetle_v2_hal_read','screen();'):
 assert forbidden not in new,forbidden
assert 'track_new_slot_pending=1u' in new
assert 'beetle_first_unused_slot_with_owner(beetle_storage_last_audit)' in save
assert save.index('native_pause();')<save.index('beetle_first_unused_slot_with_owner(beetle_storage_last_audit)')
assert 'TRACK_JOURNAL_CAPACITY' in save
assert 'capture_active=(uint8_t)(uac.feed!=0u)' in a and '!capture_active &&' in a # no warm NEW with active UAC
assert 'native_output_enabled &&' in a and 'native_output_analog_ready &&' in a
assert 'native_screen_gate.ready && !native_screen_gate.timed_out' in a
assert 'native_suspended=1u' in a and 'native_suspended=0u' in a
assert a.index('if(native_suspended){')<a.index('silent_dst[i]=0')<a.index('fm1_audio_ack_half();native_in_audio=0u;return;')
assert 'native_output_fade=0u;' in a
assert 'fm1_audio_init(' in a # cold path retained
assert 'native_budget_late()' in a and 'native_stop()' in a
assert 'beetle_master_safe_gain(' in a
print('PASS V1.7.14: guarded warm DMA NEW, silence-only ISR, cold fallback, deferred fail-closed SAVE')
