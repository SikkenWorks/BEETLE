# BEETLE V0.63 - Sequencer 2.0: Step Groove + Drum NOTE2 PRESET

* Native 5-byte steps now have 7-bit groove fields: micro offset 0-7/64 step (late only); ratchets 1-4 triggers; probability 100/75/50/25%. Values are encoded in project V12; previous project versions V1-V11 remain readable.
* SEQ+ENV: STEP GROOVE editor. K1 step, K2 parameter, K3 adjust, SEL reset focused parameter, SEQ/HOME exit. Microtiming is **positive delay only**, not anticipation/negative timing.
* Physical PRESET now handles drum NOTE2 with DRUM -> [---] -> [STP] -> DRUM; NOTE1 unchanged. STP stops the previously triggered NOTE2 voice when possible.
* Real-time scheduling uses preallocated state and sample-accurate timestamps. No allocation/storage/UI in the audio interrupt.
* WebSimulator approximates equivalent behavior via scheduled WebAudio events and mirrors the editor and NOTE2 PRESET.
* FM1 firmware and project flash/OTA remain **experimental**, not physically validated. Verify backup and recovery before installing.

## V0.62 — Reliable SEQ + K4, K1-only HELP

- In HELP, **K1 is the only encoder that scrolls**. K4 is inactive on that screen, and does not change BPM.
- While the tracker is stopped, **hold SEQ and turn K4** to change the selected pattern length from 1 to 16 steps. The rotary events are consumed before the BPM path and are undoable. K4 alone continues to control BPM.
- The WebSimulator supports SEQ hold + K4, **Shift+click K4**, and explicit PAT LEN buttons.
- Source format and audio DSP are unchanged; existing V11 projects remain compatible.
- See root `CHANGELOG.md` for release history. No hardware validation of this experimental firmware.

## V0.60 — Sequencer 2.0 and post-load input safety (historical)

- Historical V0.60 behavior: HELP K4 jumped chapter headings. **Removed in V0.62: only K1 scrolls HELP.**
- Project LOAD clears buffered SEL and requires release plus 320 ms quiet period before the tracker accepts a new SEL press.
- Per-pattern length is 1–16 steps; native stopped transport shortcut SEQ+K4, fixed in V0.62. HELP uses K1 only. WebSimulator includes PAT LEN ± controls.
- ALGORITHM during playback queues a pattern change at the next pattern boundary, not immediately.
- No changes to the V11 on-flash project format, voice DSP, 6 lanes or moon boot animation.
- Run `bash tests/track/run_v060.sh`, `python tests/web/test_beetle_v060_browser.py` and `node tests/ota/test_stock_install_v061.cjs`.
- Native build and OTA tests are software-only; external recovery and safe flashing remain unproven.

## V0.59 — Step Dynamics (velocity and gate)

The current native firmware and WebSimulator add `SEQ+FX` STEP DYNAMICS: K1 step, K2 velocity/gate, K3 value, SEL default, SEQ/HOME exit. Five-byte packed in-RAM step is unchanged; new project schema V11 preserves 4-bit velocity/gate fields. V10 legacy compatibility has a binary migration fixture. The native audio render applies variable gate release at sample granularity; WebAudio is an approximation. See `../CHANGELOG.md`.

## V0.58 Moon boot artwork

Static 240×240 user-provided moon/tree backdrop with the unchanged two-frame Beetle animation looping until the tracker opens. Generate with `python tools/generate_beetle_boot.py`. See `../CHANGELOG.md` for release notes.

# BEETLE V0.57 — Archived release notes

See [README.md](../README.md) for controls and installation warnings and [CHANGELOG.md](../CHANGELOG.md) for all release changes.

V0.57: native Block Editor page guard, pixel-consistent BEETLE block UI, clearer lane/full-pattern HELP. No changes to audio DSP or project format.

# BEETLE V0.55 — WebSimulator layout & ARP navigation

De native FM1 ARP/GLO mapping bestond al. De V0.55 WebSimulator corrigeert de navigatie, mixer en andere schermlayout. Zie de root CHANGELOG.md voor details.

# BEETLE V0.54 — HELP & MASTER FX pixel knob visuals

- New scrollable on-device PROJECTS > HELP screen (K1 per line, K4 page step, HOME back to menu, SEQ tracker).
- Redrawn circular filled MASTER FX Compressor/Reverb knobs, native and WebSimulator. No change to audio DSP or project format.
- ReSID and original Dag Lem credits preserved.

# BEETLE V0.53

The SID 6581 / SID 8580 instrument voices now have two separate fixed-point filter and saturator characters, with approximately matched output energy. This is BEETLE's SID-inspired DSP, **not** the original reSID code. See credits in `CREDITS-AND-LICENSES.md` and root `README.md`.

Tests: `bash tests/track/run_v053.sh`; `node tests/ota/test_stock_install_v053.cjs`. All 12 project slots and original project schemas remain unchanged.

# BEETLE V0.52 · SID instrument flavors + MASTER FX stability / UI

This source project is built from V0.51, preserving the six-lane tracker, project workflow, keyboard octave, audio, boot animation and web simulator.

## Changes in V0.52

- **TYPE selection in EDIT:** encoders queue individual detents and repaint after each step. Left and right browsing retain proper wraparound across all nine instrument types, without skipping types on accelerated encoder input.
- **Five SID-flavored synth types:** `SID 6581`, `SID 8580`, `SID DIRTY`, `SID BASS`, `SID NOISE`. They are distinct character modes on the embedded SID-style synthesizer, not the original reSID C++ library. Existing `RESID-LITE` project instruments map to the first SID variant with the same numeric type ID.
- **MASTER FX screen:** four visible pixel-art rotary knobs for compressor and reverb, with independent K1/K2/K3/K4 control. Compressor adds Attack. Reverb adds Width. Hold SEQ and turn K4 to reorder the chain. MASTER footer fits inside 240x240 display.
- **More robust master DSP:** skip flat EQ calculations, move per-block dynamics/filter coefficients outside the per-sample loop, validate DSP state before dereferencing a reverb index and recover a damaged DSP state without restarting firmware. The audio deadline-stop path automatically retries with capped progressive backoff rather than permanently giving up after the third attempt.
- **Project compatibility:** project encoding schema V10 saves the extra two MASTER parameters; V9 and older supported formats remain readable and initialize missing parameters to defaults. This changes the format only for newly saved files; old project data are not silently rewritten.
- **Web simulator:** latest visual controls and SID type selection; simulated WebAudio remains distinct from native FM1 DSP.
- **reSID credit:** Original reSID was authored by **Dag Lem**. BEETLE uses its own SID-inspired code, not the original library. See the acknowledgment below and `CREDITS-AND-LICENSES.md`.

## reSID credits and acknowledgment

**reSID — original author: Dag Lem.** reSID is a reverse-engineered emulation of the MOS6581 SID (Sound Interface Device), developed for Commodore 64 audio. See the original project: **[libsidplayfp/reSID](https://github.com/libsidplayfp/resid)** (a mirror of VICE's reSID), including its `AUTHORS` and licensing notices.

BEETLE acknowledges **Dag Lem and the reSID project** as an influence on its SID-inspired instrument design. BEETLE V0.52 includes five SID-style instruments (`SID 6581`, `SID 8580`, `SID DIRTY`, `SID BASS`, and `SID NOISE`), implemented in BEETLE's separate lightweight synth engine. **BEETLE does not bundle, compile, link, or run the original reSID C++ source code.** It is not cycle-accurate reSID emulation.

The full attribution and distinction between incorporated code and inspirations are documented in [`CREDITS-AND-LICENSES.md`](CREDITS-AND-LICENSES.md).

## Important limitations

Read-only/native/OTA/store builds are **software validation only**, not physical FM1 validation. Source and .fwsc are experimental; a tested recovery method and full verified backup are strongly recommended before use. Any audio fault involving an unrecoverable CPU, flash, stack or power failure is outside the scope of the DSP self-heal mechanism, so a "never crashes" guarantee would not be truthful.

## Build

Set `JIELI_TOOLCHAIN` to the directory containing `pi32v2` and `common`. Then run:

```sh
python source/firmware/fm1-native/build.py --hardware-test
python source/firmware/fm1-native/build.py --pilot
python source/firmware/fm1-native/build.py --updater --beetle-storage-lab
```

`source/tools/clean_release.py --apply` removes generated intermediates and stale versioned installer files, but preserves sources, assets, tests, and necessary firmware references.
