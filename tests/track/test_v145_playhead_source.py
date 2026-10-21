"""Verify the real full-frame production code latches the displayed position.
This guards against accidentally restoring the V1.4.4 post-SPI ISR read.
"""
from pathlib import Path
p=Path(__file__).resolve().parents[2]/'firmware/fm1-native/track_runtime.h'
s=p.read_text()
main=s[s.index('static uint8_t painted_page __attribute__'):]
full=main[main.index('if(do_full){'):main.index('}else if(paint_initialized')]
assert 'screen();track_last_paint=now;' in full
assert 'painted_step=track_ui_step_snapshot;' in full
assert 'painted_song_pos=track_ui_song_pos_snapshot;' in full
assert 'painted_step=track_state.transport.play_step' not in full
assert 'painted_song_pos=track_song.position' not in full
assert 'track_ui_song_pos_snapshot=track_song.position;' in s
assert 'track_ui_step_snapshot=track_state.transport.play_step;' in s
assert 'beetle_playhead_grid_delta(painted_step,play_step);' in s
print('PASS V1.4.5: full-frame state matches the committed LCD step and song snapshot')
