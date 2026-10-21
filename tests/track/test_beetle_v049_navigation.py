#!/usr/bin/env python3
"""Regression guard for on-device project state transitions and UX."""
from pathlib import Path
p=Path(__file__).resolve().parents[2]/'firmware/fm1-native/track_runtime.h'
s=p.read_text()
assert 'if(action==1u){' in s and 'beetle_project_new_session();beetle_project_open_load_browser();' in s
assert 'else if(action==3u)beetle_project_load_selected();' in s
assert 'track_unsaved_action=3u;track_page=13u;' in s
assert 'track_unsaved_action=1u;track_page=13u;' in s
assert 'track_unsaved_action=2u;track_page=13u;' in s
assert 'track_session_saved_slot=track_state.project_slot' in s
assert 'track_state.project_slot!=track_session_saved_slot' in s
assert 'track_page=track_storage_result==TRACK_JOURNAL_OK?0u:8u' in s
assert 'track_saved_toast_until=etch_ms+1300u' in s
assert 'track_page=18u;screen();' in s and 'track_storage_busy_kind=2u;track_page=18u;screen();' in s
assert 'track_journal_preview(&beetle_hal_journal_io,slot,preview)' in s
assert 'beetle_storage_last_audit=beetle_hal_probe_ownership();\n  if(beetle_storage_last_audit' not in s
print('PASS V0.49 UI navigation: discard -> project list, existing project load separate, save -> tracker, immediate feedback')
