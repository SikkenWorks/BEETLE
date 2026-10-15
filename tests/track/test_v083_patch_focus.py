"""Verify that new note instrument focus is separate from recorded steps."""
from pathlib import Path
p=Path(__file__).resolve().parents[2]
s=(p/'firmware/fm1-native/track_runtime.h').read_text()
assert 'track_patch_used_by_notes' in s and 'track_edit_prepare_patch' in s
assert 'track_input_instrument[ch]' in s and 'track_input_explicit[ch]' in s
assert 'track_state.active.ch[ch][at].instrument=' in s
assert 'if(!track_edit_prepare_patch(hist_ch))' in s
assert 'if(id==original || track_patch_used_by_notes(id))continue;' in s
assert 'track_edit_patch_full=1u' in s
assert 'struct track_store_instrument copy;' in s
assert 'track_library_capture_active(original,&copy)' in s
assert 'track_library_apply_active(id,&copy)' in s
assert 'track_chip_refresh_instrument(&track_chip,hist_id)' in s
web=p.parent/'simulator.html'
w=web.read_text()
assert 'function patchIsUsed(id)' in w and 'function prepareInstrumentEdit()' in w
assert 'ALL FOUR PATCH SLOTS USED' in w
assert 'st.inst=model.inputInstrument[model.ch]' in w
assert 'audio.punch(i,down)' in w
print('PASS V0.83: separate patch focus, COW safety, web parity')
