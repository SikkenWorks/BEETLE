#!/usr/bin/env python3
"""V0.81 regression: source-level UI safety and explicit slot allocation invariants.

These checks are NOT a replacement for physical FM1 USB/flash validation.
"""
from pathlib import Path
root=Path(__file__).resolve().parents[2]
r=(root/'firmware/fm1-native/track_runtime.h').read_text()
u=(root/'firmware/fm1-native/usb.c').read_text()
a=(root/'firmware/fm1-native/audio_target.c').read_text()
ld=(root/'firmware/fm1-native/beetle_app.ld').read_text()
sim=(root.parent/'simulator.html').read_text()

for text in ['if(track_page==29u){','"NEW PROJECT","LOAD PROJECT"',
             'track_page=29u;track_boot_menu_choice=0u',
             'track_page=track_storage_result==TRACK_JOURNAL_OK?track_save_return_page:8u;',
             'track_page=track_storage_result==TRACK_JOURNAL_OK?track_save_return_page:8u;',
             'beetle_first_unused_slot(void)',
             'result==TRACK_JOURNAL_NO_PROJECT)return slot',
             'track_new_project_full=(uint8_t)(next>=TRACK_PROJECTS)',
             'if(track_new_project_full)',
             'track_preset_focus_original=255u',
             'track_page==16u',
             'track_state.transport.edit_channel==TRACK_DRUM_CHANNEL?0u:track_state.transport.edit_channel',
             'SEL \\"TOGGLE ON/OFF\\"']:
    assert text in r,text
assert 'track_page=track_storage_result==TRACK_JOURNAL_OK?0u:8u;' not in r
assert 'uint8_t *value=&st->chord' not in r
assert 'const uint8_t ch=track_state.transport.edit_channel==TRACK_DRUM_CHANNEL?0u' in r
assert 'static uint32_t ua_ring[UA_N] __attribute__((section(".usb_audio_ram")))' in u
assert 'static int32_t native_usb_stereo[ETCH_NATIVE_FRAMES*2] __attribute__((section(".usb_audio_ram")))' in a
assert 'UACRAM (rw)' in ld and '0x01C06000' in ld and '0x1F00' in ld
assert 'track_new_project_full=0u;' in r
for text in ['screen:\'boot\'','function bootChooser()','function firstUnusedSlot()','function startNewAuto()','model.screen=model.saveReturn','model.screen===\'boot\'','model.presetFocus.original','amp.connect(audio.bus[model.ch===5?0:model.ch].filter)']:
    assert text in sim,text
assert 'if(model.ch===5){audio.drumHit(i%DRUM_NAMES.length' not in sim
print('PASS V0.81 startup-only chooser, first-free refusal, save screen, CH6 synth audition, preset, USB RAM map')
