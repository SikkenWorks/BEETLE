#!/usr/bin/env python3
"""Check native/web routes agree about Song Mode and CH6 NOTE2."""
from pathlib import Path
root=Path(__file__).resolve().parents[2]
n=(root/'firmware/fm1-native/track_runtime.h').read_text()
c=(root/'firmware/fm1-native/track_core.c').read_text()
w=(root.parent/'simulator.html').read_text()
assert 'if(track_page==9u){if(track_song.enabled)track_page=30u;}' in n
assert 'if(track_page==30u && !track_song.enabled)track_page=9u;' in n
assert 'if(track_page==30u && track_song.enabled && mixer_key_pressed)' in n
assert 'track_enter_drum_note2(&track_state,note)' in n
assert 'e->active.ch[TRACK_DRUM_CHANNEL][e->transport.edit_step].chord=(unsigned int)(drum_note+1u)' in c
assert "model.screen==='song'?(model.songMode?'livemixer':'song')" in w
assert "const note2=model.ch===5&&model.col===2;" in w
assert 'if(note2)st.chd=drumNote+1' in w
assert 'if(!note2)model.row=' in w
print('V0.88: native/web route static checks PASS')
