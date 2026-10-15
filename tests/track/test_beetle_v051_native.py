from pathlib import Path
s=(Path(__file__).resolve().parents[2]/'firmware/fm1-native/track_runtime.h').read_text()
assert 'beetle_keyboard_pitch(k,native_controls.octave)' in s
assert 'beetle_keyboard_pitch(newest,native_controls.octave)' in s
assert 'track_preview_note[k]=note' in s
assert 'track_chip_note_off(&track_chip,track_preview_channel[k],track_preview_note[k]);' in s
assert 'track_edit_voice_note[k]' in s
assert 'etch_octave_scan(&track_key_oct_chord,buttons)' in s
assert 'if(track_page==0u || track_page==5u)' in s
assert 'track_adjust_octave(&track_state' not in s
assert 'track_edit_poly^=1u' in s
print('PASS BEETLE V0.51 native octave shared between EDIT/TRACKER and note-offs latched')
