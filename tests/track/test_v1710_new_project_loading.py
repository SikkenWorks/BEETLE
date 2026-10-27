# Earlier NEW-path assertions apply to their original versions only.
# From V1.7.13 the dedicated regression checks the deferred SAVE allocation.
from pathlib import Path as _Path
if 'BEETLE V1.7.13' in (_Path(__file__).resolve().parents[2]/'firmware/fm1-native/track_runtime.h').read_text() or 'BEETLE V1.7.14' in (_Path(__file__).resolve().parents[2]/'firmware/fm1-native/track_runtime.h').read_text():
 import runpy as _runpy
 _runpy.run_path(str(_Path(__file__).resolve().parents[1]/'track/test_v1714_new_project_gate.py'))
 raise SystemExit(0)
#!/usr/bin/env python3
"""V1.7.10 source-level loading UI & audio/input safety regression."""
from pathlib import Path
s=(Path(__file__).resolve().parents[2]/'firmware/fm1-native/track_runtime.h').read_text()
helper=s.split('static void beetle_new_project_loading_indicator(void){',1)[1].split('static void beetle_project_new_session(void){',1)[0]
new=s.split('static void beetle_project_new_session(void){',1)[1].split('static void beetle_project_open_load_browser(void){',1)[0]
assert 'track_panel_begin_slice(96u,46u)' in helper
assert 'etch_panel_commit()' in helper
assert 'CREATING PROJECT' in helper and 'PLEASE WAIT...' in helper
assert 'track_fm1_draw(' not in helper and 'screen(' not in helper
assert 'fm1_irq_off(' not in helper and 'native_start(' not in helper
assert new.index('beetle_storage_input_lock()') < new.index('native_pause()') < new.index('beetle_new_project_loading_indicator()') < new.index('beetle_first_unused_slot()') < new.index('track_init(&track_state)') < new.index('if(resume_audio)native_start()')
assert new.count('beetle_new_project_loading_indicator()') == 1
assert 'track_page=0u;' in new
assert 'track_state.dirty=0u' in new
assert 'beetle_new_perf_paint_pending=1u' in new
assert 'fm1_irq_off(' not in new
print('PASS: visible 46-row loading indicator before NOR scan, DMA paused, clean tracker handoff, no extra full frame')
