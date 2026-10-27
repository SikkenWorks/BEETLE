# Earlier NEW-path assertions apply to their original versions only.
# From V1.7.13 the dedicated regression checks the deferred SAVE allocation.
from pathlib import Path as _Path
if 'BEETLE V1.7.13' in (_Path(__file__).resolve().parents[2]/'firmware/fm1-native/track_runtime.h').read_text() or 'BEETLE V1.7.14' in (_Path(__file__).resolve().parents[2]/'firmware/fm1-native/track_runtime.h').read_text():
 import runpy as _runpy
 _runpy.run_path(str(_Path(__file__).resolve().parents[1]/'track/test_v1714_new_project_gate.py'))
 raise SystemExit(0)
#!/usr/bin/env python3
"""Verify NEW PROJECT uses one V2 ownership probe, no duplicate LCD frame,
and retains fail-closed DMA, input fencing and flash index domain audit."""
from pathlib import Path
root=Path(__file__).resolve().parents[2]
t=(root/'firmware/fm1-native/track_runtime.h').read_text()
a=(root/'firmware/fm1-native/audio_target.c').read_text()
r=(root/'firmware/fm1-native/beetle_storage_v2_reader.c').read_text()
fn=t.split('static uint8_t beetle_first_unused_slot(void){',1)[1].split('static void beetle_project_new_session(void){',1)[0]
new=t.split('static void beetle_project_new_session(void){',1)[1].split('static void beetle_project_open_load_browser(void){',1)[0]
assert fn.count('beetle_hal_probe_ownership_ui()')==1
assert 'beetle_v2_mode()' not in fn
assert fn.count('beetle_v2_open(')==0
assert 'beetle_v2_audit_and_first_unused_project_fast' in fn
assert fn.index('beetle_hal_v2_marker_prefix()') < fn.index('beetle_v2_audit_and_first_unused_project_fast')
assert 'audit_domains_checked_index(report,scratch)' in r and 'first_unused_index(selected)' in r
assert 'first_unused_index(scratch)' in r
assert 'beetle_new_tracker_paint_pending' not in t
assert new.index('native_pause();') < new.index('beetle_first_unused_slot();') < new.index('track_init(&track_state);') < new.index('native_start();')
assert 'fm1_irq_off' not in new and 'fm1_irq_enable_all' not in new
assert 'beetle_storage_input_lock();' in new
assert 'if(resume_audio)native_start();' in new
assert 'native_output_enabled=0u;fm1_audio_output_enable(0u);fm1_audio_stop();' in a
assert all(s in t for s in ['RC NEW PROJECT','FLASH SCAN MS','RESET RAM MS','DRAW LCD MS','TOTAL TO LCD MS','beetle_new_perf_total_ms'])
assert 'BEETLE V1.7.11' in t
print('PASS V1.7.11: single owner/marker lookup, audited index, one frame, fail-closed DMA and on-device timing diagnostics')
