# V1.4.10 Mono EDIT and Song Chains

## Confirmed source cause of missed mono notes

The old `track_runtime.h` rejected onsets within 45ms but still committed `track_edit_prev_keys=held`. This could permanently suppress a key while held. It also reset all synth voices on each mono note-on. Off events tracked mutable voice indices, risking release of an unrelated stolen/reused voice.

## Correction

`beetle_mono_audition.h` accepts new physical note-ons immediately, delays release by up to 8ms to reject brief contact bounce, implements last-note priority, and falls back to another still-held key. It reuses the existing EDIT-only keyboard arrays to avoid increasing tight RAM use. The currently playing synth voice is released by birth serial; later notes cannot be gated by an earlier key-off. A short release tail replaces hard voice clearing during a note transition. The polyphonic path remains independent, with previously stored poly editing behavior.

## Song Chains

New row defaults still point at source pattern `Pnn`; a lane visually displays `--` while that source lane contains no data. It becomes visible automatically when the user adds any note, stop, chord, step FX or sparse FX to the source. Manual explicit `--` is never auto-enabled. Serialised project format and per-lane pattern references are unchanged; no project migration required. The currently edited pattern is captured when entering Song Chains so fresh tracker edits appear.

## Development state

Tests: dedicated mono/5 SID types/song cases (UBSan C host); experimental JieLi storage+OTA+USB and read-only builds; existing DPCM4 host tests. WebInstaller must pin the exact candidate hash, and the original FM-1_2123 image is used for loader/boot assets. The physical device has NOT been tested. The global user-sample storage/prefetch path remains host-only; it is not available in this firmware. Do not flash without independent recovery and a verified full-flash backup.

## Reproduction

`bash tests/track/run_v1410.sh` from repository root. `python firmware/fm1-native/build.py --beetle-storage-lab --updater --usb-audio` with the proper JieLi toolchain.
