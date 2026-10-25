# V1.4.33 chord-preview regression / hardware follow-up

## Native behavior
- TRACKER synth CH1–CH5, K2 chooses CHD column, K3 triggers immediate short audition with currently saved NOTE and INST.
- `beetle_chord_scroll_pitches()` delegates to `track_chord_keylocked()` (same as sequencer playback), honors FULL KEYLOCK. For empty (`---`) chord the existing root sounds alone.
- The hardware preview owns a bounded list of five notes. Previous notes release before new chord preview; timeout 240 ms. Changing row or column releases preview; page/PLAY ends it as well.
- CH6, PLAY, STOP, EMPTY, invalid chord -> silent. No effects on row position, recorder/REC or project serialization.
- Mono mode intentionally sounds only root due to global voice-count limit. Tests cover all 12 types, max five notes, saved data unchanged.

## Validation
- Host: `cc -std=c11 -O2 -ffunction-sections -fdata-sections -I firmware/fm1-native tests/track/test_v1433_chord_scroll_preview.c firmware/fm1-native/track_core.c -Wl,--gc-sections -o /tmp/chord && /tmp/chord`
- Firmware: native build `--beetle-storage-lab --beetle-v2 --updater --usb-audio`; 44.1kHz stereo/multitrack code unchanged.
- JieLi native compilation and .fwsc integrity checks do NOT prove on-hardware timing, audio tails or OTA safety. Back up storage and verify recovery path before experimental flash.

## Hardware checklist
- CH1 root C4: scroll --- / MAJ / MIN / MJ7 / MN9 and hear correct voicing on each detent.
- Holding successive K3 detents must not stack old chord voices or cause stuck notes.
- CHD preview should be silent while playing; CH6 CHD column must stay kit parameter.
- Empty step and STP should not sound. Test REC on/off; scrolling CHD must not insert any NOTE or alter other rows.
- Test mono: root-only preview; poly: full chord; keylock scale-aware voicings; USB recording simultaneously.
