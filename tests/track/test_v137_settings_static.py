"""Static checks for native FM1 menu dispatch (browser tests alone are insufficient)."""
from pathlib import Path
s=Path('firmware/fm1-native/track_runtime.h').read_text()
assert s.index('Read the real K1 menu movement before consuming SEL') < s.index('if(beetle_settings_enter(') < s.index('/* RC hardware telemetry is read-only')
assert s.index('if(beetle_settings_enter(') < s.index('if(track_page==1u && (pressed&(1u<<3)))')
assert 'track_page=BEETLE_SETTINGS_PAGE' in s
assert 'beetle_settings_toggle(&track_settings_nav,buttons,pressed,released)' in s
assert 'if(track_page==1u && (pressed&(1u<<3)))' in s
assert 'SETTINGS already handled above; never mistake this menu index for LOAD' in s
print('PASS V1.3.7 native FM1 button route, same-poll K1+SEL, SETTINGS isolation')
