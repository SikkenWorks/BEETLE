#!/usr/bin/env python3
"""V1.7.13: NEW is RAM-only; SAVE retains the fail-closed slot scan."""
from pathlib import Path
if 'BEETLE V1.7.14' in (Path(__file__).resolve().parents[2]/'firmware/fm1-native/track_runtime.h').read_text():
 import runpy
 runpy.run_path(str(Path(__file__).with_name('test_v1714_new_project_gate.py')))
 raise SystemExit(0)
r=Path(__file__).resolve().parents[2]
t=(r/'firmware/fm1-native/track_runtime.h').read_text()
new=t.split('static void beetle_project_new_session(void){',1)[1].split('static void beetle_project_open_load_browser(void){',1)[0]
save=t.split('static void track_complete_save(void){',1)[1].split('static uint8_t beetle_first_unused_slot_with_owner(int owner){',1)[0]
scan=t.split('static uint8_t beetle_first_unused_slot_with_owner(int owner){',1)[1].split('static void beetle_project_new_session(void){',1)[0]
assert t.count('static uint8_t track_new_slot_pending;') == 1
assert new.index('beetle_storage_input_lock();') < new.index('native_pause();') < new.index('track_init(&track_state);') < new.index('if(resume_audio)native_start();')
for forbidden in ('beetle_hal_probe_ownership_ui(', 'beetle_hal_v2_marker_prefix(', 'beetle_v2_hal_read', 'beetle_first_unused_slot_with_owner(', 'beetle_new_project_loading_indicator(', 'beetle_project_preview_slot(', 'etch_panel_commit(', 'screen();'):
    assert forbidden not in new, forbidden
assert 'track_new_slot_pending=1u;' in new and 'track_new_project_full=0u;' in new
assert 'track_state.project_slot=selected' in new and 'uint8_t selected=0u;' in new
assert 'beetle_new_perf_scan_ms=0u;' in new and 'beetle_new_perf_loading_ms=0u;' in new
assert 'track_state.dirty=0u;' in new
assert 'track_new_slot_pending=0u' in t.split('static void track_fm1_boot(void){',1)[1].split('static void track_fm1_poll(',1)[0]
assert 'beetle_first_unused_slot_with_owner(beetle_storage_last_audit)' in save
assert save.index('native_pause();') < save.index('beetle_first_unused_slot_with_owner(beetle_storage_last_audit)') < save.index('track_name_finish(&track_editor,track_state.project_slot,track_title)') < save.index('beetle_project_capture(track_state.project_slot)')
assert 'if(next>=TRACK_PROJECTS)' in save and 'TRACK_JOURNAL_CAPACITY' in save
assert 'track_state.project_slot=next;' in save and 'track_new_slot_pending=0u;' in save
assert 'if(!track_overwrite_wait && track_state.project_slot!=track_session_saved_slot' in save
assert 'beetle_project_preview_slot(track_state.project_slot,peek_title)' in save
assert 'beetle_project_save_staged();' in save
assert 'beetle_hal_probe_ownership_ui()' not in scan
assert 'if(owner==BEETLE_CLAIM_BLANK)return 0u;' in scan
assert 'if(owner!=BEETLE_CLAIM_EXISTING)return TRACK_PROJECTS;' in scan
assert 'beetle_hal_v2_marker_prefix()' in scan
assert 'beetle_v2_audit_and_first_unused_project_fast' in scan
assert 'beetle_bd4_live_bind(' in scan
assert 'if(result!=TRACK_JOURNAL_OK)return TRACK_PROJECTS;' in scan
assert 'track_new_slot_pending=0u; /* K4 is an explicit SAVE AS destination */' in t
assert 'track_new_slot_pending=0u;track_new_project_full=0u;' in t
assert 'track_new_project_full=0u;track_new_slot_pending=0u;' in t
assert 'SLOT AUTO ON SAVE' in t
assert 'if(track_storage_result==TRACK_JOURNAL_OK)' in save
assert 'BEETLE V1.7.13' in t
print('PASS V1.7.13: no NEW flash/indicator; deferred audited allocation, fail-closed SAVE, manual K4 overwrite, full-slot handling, DMA safety')
