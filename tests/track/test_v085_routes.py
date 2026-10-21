from pathlib import Path
r=Path(__file__).resolve().parents[2]
n=(r/'firmware/fm1-native/track_runtime.h').read_text()
w=(r.parent/'simulator.html').read_text()
assert 'track_step_oct_held' in n and 'track_seq_fired=1u; /* modifier release never navigates away */' in n
assert 'beetle_edit_begin(&beetle_ed,&track_song,&track_state);\n      if(track_adjust_octave' in n
assert 'track_scroll_preview_until=now+180u' in n
assert 'if(!track_state.record_armed && !(buttons&(1u<<11))){' in n
assert 'native_controls.octave<0 ||' in n and 'track_page==30u?track_live_q_mute' in n
assert 'native_controls.octave>0 ||' in n and 'track_page==30u?track_live_q_solo' in n
assert "if(model.screen==='tracker'){\n  const drumNote=" in w and "rememberEdit(()=>{const st=step(" in w
assert 'stepOctChordHeld.has(action) && model.screen' in w
assert "[['OCT-',model.keyboardOctave<0],['OCT+',model.keyboardOctave>0]]" in w
print('V0.85 native and simulator routes PASS')
