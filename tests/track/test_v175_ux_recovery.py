# Earlier NEW-path assertions apply to their original versions only.
# From V1.7.13 the dedicated regression checks the deferred SAVE allocation.
from pathlib import Path as _Path
if 'BEETLE V1.7.13' in (_Path(__file__).resolve().parents[2]/'firmware/fm1-native/track_runtime.h').read_text() or 'BEETLE V1.7.14' in (_Path(__file__).resolve().parents[2]/'firmware/fm1-native/track_runtime.h').read_text():
 import runpy as _runpy
 _runpy.run_path(str(_Path(__file__).resolve().parents[1]/'track/test_v1714_new_project_gate.py'))
 raise SystemExit(0)
#!/usr/bin/env python3
"""Source invariants for v1.6.10-responsive UX with bounded master/NEW transaction."""
from pathlib import Path
import hashlib
r=Path(__file__).resolve().parents[2]
original_hashes={
 'track_core.c': '386058575cf9037ac9ba7500320b228b00d1232b9c6db6656b9af5e77d8745cc',
 'track_chip.c': 'a3bab76bfce2cf99e690f97fe40fdfc1a1d30d7078ae283b6d377f7168160cef',
 'track_drum.c': '81e8f077e4f8f4730ecfd92bedc4e5a2e94c425d6f5be0f301dc91b172811a9d',
 'track_mixer.c': '6dbea1ecf3d385c5c3e73666aa69c6a89a717ab3c0f7a1f137726ba90aa9ba68',
 'track_song.c': '95c6bab97e6e6ffe429eb4e6c4ec026f4915fe0f4c8de533034224c0751aedfe',
 'lcd.c': 'bd2e7421d1e309ba3addd3e7071ef41694fc2083ccf23467cf1b6b99a19bb50b',
}
a=(r/'firmware/fm1-native/audio_target.c').read_text()
b=(r/'firmware/fm1-native/bringup.c').read_text()
t=(r/'firmware/fm1-native/track_runtime.h').read_text()
assert 'beetle_master_safe_gain(native_controls.gain' in a
assert 'native_song_stop_pending=1u' in a
assert 'beetle_master_service_if_due();' in t
assert 'track_panel_begin_slice(0u,120u)' in t and 'track_panel_begin_slice(120u,120u)' in t
assert 'BEETLE_LCD_PLAY_TILE' not in t
assert 'beetle_step01_probe' not in t
assert 'track_song_prepare_next' not in t
assert 'beetle_master_last_poll_ms' in b
assert 'beetle_master_service_if_due();' in b
assert 'BEETLE_MASTER_STALE_MS' in (r/'firmware/fm1-native/beetle_audio_safety.h').read_text()
new=t.split('static void beetle_project_new_session(void){',1)[1].split('static void beetle_project_open_load_browser',1)[0]
assert new.index('native_pause();')<new.index('beetle_first_unused_slot();')<new.index('track_init(&track_state);')
assert 'fm1_irq_off();' not in new and 'fm1_irq_enable_all();' not in new
assert 'if(resume_audio)native_start();' in new
# Verify regression-prone original DSP/sequencer and UI resources retained.
for name in ('track_core.c','track_chip.c','track_drum.c','track_mixer.c','track_song.c','lcd.c'):
 p='firmware/fm1-native/'+name
 assert hashlib.sha256((r/p).read_bytes()).hexdigest()==original_hashes[name],name
print('PASS V1.7.5: original responsive transport+LCD unchanged; master serviced between halves; DMA quiesced for NEW PROJECT')
