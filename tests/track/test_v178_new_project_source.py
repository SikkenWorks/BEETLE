# Earlier NEW-path assertions apply to their original versions only.
# From V1.7.13 the dedicated regression checks the deferred SAVE allocation.
from pathlib import Path as _Path
if 'BEETLE V1.7.13' in (_Path(__file__).resolve().parents[2]/'firmware/fm1-native/track_runtime.h').read_text() or 'BEETLE V1.7.14' in (_Path(__file__).resolve().parents[2]/'firmware/fm1-native/track_runtime.h').read_text():
 import runpy as _runpy
 _runpy.run_path(str(_Path(__file__).resolve().parents[1]/'track/test_v1714_new_project_gate.py'))
 raise SystemExit(0)
#!/usr/bin/env python3
from pathlib import Path
r=Path(__file__).resolve().parents[2]
t=(r/'firmware/fm1-native/track_runtime.h').read_text()
a=(r/'firmware/fm1-native/audio_target.c').read_text()
b=(r/'firmware/fm1-native/beetle_input_safety.h').read_text()
new=t.split('static void beetle_project_new_session(void){',1)[1].split('static void beetle_project_open_load_browser(void){',1)[0]
assert new.index('native_pause();')<new.index('beetle_first_unused_slot();')<new.index('track_init(&track_state);')<new.index('native_start();')
assert 'fm1_irq_off();' not in new and 'fm1_irq_enable_all();' not in new
assert 'beetle_new_tracker_paint_pending=1u' in new
assert 'if(beetle_new_tracker_paint_pending)' in t
assert 'if(track_page==0u){screen();track_last_paint=now;}' in t
assert 'BEETLE_IO_INPUT_QUIET_MS 320u' in b
assert 'beetle_v2_first_unused_project(&device,scratch)' in t
assert 'beetle_project_preview_slot(slot,name)' in t # older legacy storage remains
assert 'native_tripped=0;' in a and 'native_output_enabled=0' in a
assert 'beetle_master_safe_gain' in a
assert 'BEETLE V1.7.8' in t
print('PASS V1.7.8: pause/reset/rearm safeguarded, direct tracker redraw, input gate retained, V2 fast scan')
