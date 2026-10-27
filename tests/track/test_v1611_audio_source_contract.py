#!/usr/bin/env python3
"""Static contract on the actual compiled firmware path, not a helper alone."""
from pathlib import Path
r=Path(__file__).resolve().parents[2]
a=(r/'firmware/fm1-native/audio_target.c').read_text()
b=(r/'firmware/fm1-native/bringup.c').read_text()
t=(r/'firmware/fm1-native/track_runtime.h').read_text()
l=(r/'firmware/fm1-native/track_lane_clipboard.h').read_text()
assert 'beetle_master_safe_gain(native_controls.gain' in a
assert 'native_master_last_good_ms' in a and 'native_song_stop_pending=1u' in a
late=a.split('static void native_budget_late(void){',1)[1].split('static void native_budget_ok(',1)[0]
assert 'native_stop();' in late and 'native_song_stop_pending=1u' in late
assert late.index('#else')>late.index('native_stop();'), 'tracker could fall through to ETCH voice-shedding'
assert '#ifndef TRACK_FIRMWARE\n  if(!muted && native_tripped' in b,'BEETLE auto restart guard missing'
assert 'beetle_master_publish_step(native_controls.gain,adc)' in b
assert 'beetle_master_service_if_due();' in b
assert 'beetle_master_service_if_due();' in t
assert 'native_controls.gain=0u' in b and 'native_song_stop_pending' in b
assert 'track_copy_blocked_until=now+1800u' in t
assert 'if((combo_edges&(1u<<1u)) && !track_state.transport.playing)' in t
assert 'if(track_state.transport.playing)track_copy_blocked_until=now+1800u' in t
assert 'if(lane==TRACK_DRUM_CHANNEL)pat->note3[i]=clip->note3[i]' in l
assert 'if(lane==TRACK_DRUM_CHANNEL)pat->note3[dest+i]=clip->note3[i]' in l
print('PASS: DMA trip, no BEETLE auto restart, ADC fail-closed, guarded clipboard, no live copy')
