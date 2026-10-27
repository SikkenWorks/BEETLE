#!/usr/bin/env python3
"""V1.7.12 source-level assertions for the actual FM1 project-menu branches.
The C state-machine regression checks the same header helpers dynamically.
"""
from pathlib import Path
import re
s=(Path(__file__).resolve().parents[2]/'firmware/fm1-native/track_runtime.h').read_text()
assert ('beetle_new_project_loading_indicator();' in s) if not any(v in s for v in ('BEETLE V1.7.13','BEETLE V1.7.14')) else ('beetle_first_unused_slot_with_owner(beetle_storage_last_audit)' in s)
fn=s[s.index('if(track_page==13u){',s.index('static void track_fm1_poll(void)')):]
act1=fn[fn.index('if(action==1u){'):fn.index('}else if(action==2u){')]
assert 'beetle_project_new_session' not in act1
assert 'beetle_project_open_load_browser();' in act1
assert 'beetle_load_navigation_approve(&track_load_navigation);' in act1
assert 'beetle_load_input_arm(&track_load_input_guard,now);' in act1
assert 'beetle_project_new_session();' in fn[fn.index('}else if(action==2u){'):fn.index('else if(action==3u)')]
assert 'track_page=action==3u?14u:1u;' in fn
load=fn[fn.index('if(track_page==14u){'):fn.index('/* Undo/Redo')]
assert 'track_state.project_slot=beetle_load_navigation_cancel(&track_load_navigation);' in load
assert re.search(r'else if\(track_load_status\[track_state\.project_slot\]==1u\)\s*\{\s*if\(beetle_load_navigation_needs_discard',load)
assert 'beetle_project_load_selected();' in load
assert 'beetle_load_navigation_begin(&track_load_navigation,track_state.project_slot);' in s
assert 'if(track_storage_result==TRACK_JOURNAL_OK){\n   beetle_load_navigation_end(&track_load_navigation);' in s
print('PASS: no create/reset/CREATING PROJECT during DISCARD -> LOAD, guarded real LOAD, cancel restores slot, new project unchanged')
