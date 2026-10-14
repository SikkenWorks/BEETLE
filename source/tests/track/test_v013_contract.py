from pathlib import Path
root=Path(__file__).resolve().parents[2]
n=(root/'firmware/fm1-native/track_runtime.h').read_text()
w=(root/'TRACK-Web-V0.13.html').read_text()
c=(root/'firmware/fm1-native/track_chip.c').read_text()
assert 'track_page=10u' in n and 'now-track_rec_since>=1500u' in n
assert 'track_song_record(' in n and 'track_punch_edit_index' in n
assert 'K2 NOTE1 / NOTE2' in n
assert 'track_edit_poly^=1u' in n and 'track_edit_prev_keys' in n
assert 'track_overwrite_wait' in n and 'HOME = NO' in n
assert 'etch_led_scale_guide_targets' in n and 'GUIDE: SCALE KEY LEDS' in n
assert 's->last_voice_index=index' in c and 's->voices[index].active' in c
assert 'OVERWRITE?' in w and "action==='LONGREC'" in w
assert 'function punchEventEditor()' in w and 'function overwriteScreen()' in w
assert "classList.toggle('guide'" in w
assert 'REC: PUNCH FX' in w and 'id="track-installer-frame"' in w
assert 'INSTALL NOT AVAILABLE' in w or 'INSTALL NOT AVAILABLE' in __import__('base64').b64decode(__import__('re').search(r"const encoded='([^']+)'",w).group(1)).decode()
print('TRACK V0.13 source contracts PASS')
