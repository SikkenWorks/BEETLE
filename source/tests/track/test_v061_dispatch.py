from pathlib import Path
r=(Path(__file__).resolve().parents[2]/'firmware/fm1-native/track_runtime.h').read_text()
h=(Path(__file__).resolve().parents[2]/'firmware/fm1-native/beetle_help.h').read_text()
assert 'track_help_offset=beetle_help_scroll(track_help_offset,k1);' in r
assert 'track_help_offset=beetle_help_scroll(track_help_offset,k1+3*k4)' not in r
assert 'if(beetle_pattern_length_chord(track_page,buttons,released,k4))' in r
assert 'k4=0; /* already consumed' in r
assert 'beetle_edit_begin(&beetle_ed,&track_song,&track_state);' in r
assert 'if(k4 && track_page==0u && !(buttons&(1u<<11)))' in r
assert 'HELP: K1 SCROLLS ONLY' in h
print('PASS V0.61 native runtime dispatch static checks')
