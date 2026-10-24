# BEETLE V1.4.26 — Punch-In DSP, NOTE2 user samples and streaming diagnostics

Status: **experimental, source-verified and host-tested only**. No connected FM1 was available for physical audio, DMA, USB or OTA verification.

## Corrections

1. **SCRATCH** (PUNCH-IN 2/2, key 9): reads a held stereo audio slice backwards and forwards at a parameter-adjustable velocity. The existing 12-KiB shared Punch-In history is reused and limits real source material to approximately 128 ms; this cannot emulate a full-length DJ vinyl scratch. The soft braking near turnarounds mitigates clicks. This is no substitute for testing the audible result on FM1.
2. **BITCRUSH** (key 10): deterministic reduction to 4–12 effective bits, using signed-safe symmetrical rounded quantization, instead of a potentially inaudible 16-bit setting.
3. **PHASER** (key 14): four all-pass stages in series, followed by equal-parts dry/wet sum for moving comb notches. Independent LFO rate from the PHASER parameter.
4. **RADIO** (key 15): voice-band high-pass plus low-pass and bounded saturation, with a subdued hiss component. No noise-only routing.
5. **DJ ISOLATOR** (key 16): dynamic three-band crossover with seven selectable band-kill masks, including low, mid, high and combinations.
6. **PUMP, GLITCH and WOW**: retained. All 16 Punch-In parameters now survive a DSP recovery.
7. **User samples looping**: the Storage V2 BD4S live reader tracks validated immutable sectors on a per-voice basis while retaining the single existing ~4-KiB scratch cache. When two voices alternate between prevalidated sectors, it now reads only the requested compressed subblock rather than re-verifying the complete sector each audio block. This reduces main-thread flash read work but does not claim a measured improvement on real hardware.
8. **NOTE2**: all CH6 notes `0..26` (14 factory and U01–U13) are encodable in NOTE2. 4-bit `chord` holds base value and the previously unused third KIT bit marks the extended thirteen notes. KIT's lower two bits, primary NOTE, step effect, length and project bytes remain unchanged. The browser codec mirrors this layout. STOP and empty remain reserved codes.

## Compatibility

- TRK1 **V25** project schema unchanged. Old NOTE2 hits decode identically. **V1.4.25 will refuse projects containing U01–U13 encoded in NOTE2**, because its drum KIT validator accepts only 0–3; export a backup and remove those hits before a downgrade.
- Fixed-source firmware and installer retain the exact package identity `FM-1_2123` and the original protected header and OTA loader. This does **not** prove real-world OTA success or reliable cross-firmware switches.
- Full 16-FX button/page focus logic is unchanged.
- Sample library `*.beetlebank` and Project Manager continue to support existing global sample names, upload/reconnect safeguards and the Storage V2 project reserve.
- The WebSimulator has matching NOTE2 editing, but cannot download the physical FM1's encoded global audio samples for local playback.

## Reproduce software checks

From project root (Linux, host compiler installed):

```sh
cc -std=c11 -O2 -fsanitize=undefined -fno-sanitize-recover=all \
  -Ifirmware/fm1-native tests/track/test_v1426_audio_note2.c \
  firmware/fm1-native/track_chip.c firmware/fm1-native/track_core.c \
  firmware/fm1-native/track_drum.c firmware/fm1-native/beetle_drum_rom.c \
  firmware/fm1-native/track_mixer.c -o /tmp/beetle-v1426-test
/tmp/beetle-v1426-test
python tests/dpcm4/run_v1422_live.py
python tests/dpcm4/run_prefetch.py
python tests/dpcm4/test_v1415_pinned_storage.py
python tests/storage_v2/test_project_reserve_1425.py
```

Native build (externally supplied JieLi toolchain):

```sh
export JIELI_TOOLCHAIN=/path/to/jieli-linux-toolchains-20260730.1
python firmware/fm1-native/build.py --beetle-storage-lab --updater --usb-audio --beetle-v2
```

## Required real FM1 validation

1. Back up all `.beetle` projects and the global `.beetlebank` on your computer and verify you can restore the FM1 before flashing an experimental build.
2. Compare held SCRATCH at several K1 speeds against a real rhythmic source, not just a sine wave. Listen for correct forward/backward pitch motion and any audible edge click.
3. Check BITCRUSH at 0/50/100 and PHASER/RADIO/ISOLATOR on both drums and sustained chords, with the other Punch-In FX released.
4. Enter U01–U13 on both NOTE1 and NOTE2, save/reload the project and verify both notes and separate step FX. Also load an older project containing factory NOTE2 hits.
5. Loop a 16-step pattern for at least 10 minutes with up to six overlapping user sample voices, then repeat with MASTER FX, 8 synthvoices, and six-track USB recording. Observe DMA deadlines, stream underrun counters, crashes, sample corruption, pitch/wobble and tempo drift.
6. Verify save/restore, project reserve protection and switching from and back to V1.4.25 via a previously proven recovery route. Cross-version project reading should be checked on the real device before any downgrade.

A successful build, browser check or synthetic host test is not proof that these acceptance cases pass on a physical FM1.
