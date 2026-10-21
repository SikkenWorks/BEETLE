from pathlib import Path
root=Path(__file__).resolve().parents[2]
s=(root/'firmware/fm1-native/track_runtime.h').read_text()
assert 'beetle_load_input_arm(&track_load_input_guard,etch_ms);' in s[s.index('static void track_complete_save(void)'):s.index('static void beetle_project_new_session(void)')]
assert 'beetle_load_input_arm(&track_load_input_guard,etch_ms);' in s[s.index('static void beetle_project_load_selected(void)'):s.index('static void track_fm1_poll(void)')]
assert 'const uint8_t input_entry_page=track_page;' in s
assert 'const uint8_t can_delete_step=(uint8_t)(track_sel_tracker_ready &&' in s
assert 'beetle_sel_rearm(&track_sel_tracker_ready,input_entry_page,' in s
assert 'if(can_delete_step){\n    (void)track_clear_step' in s
assert 'if(pressed&(1u<<3)){\n    (void)track_clear_step' not in s
print('SEL/project transition static checks: OK')
