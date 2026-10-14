# BEETLE Changelog

## V0.63 — Sequencer 2.0: microtiming, ratchets, probability + drum NOTE2 PRESET (2026-10-15)

- **STEP GROOVE editor:** hold **SEQ** and press **ENV** while stopped to open a separate BEETLE-styled editor. K1 chooses the step; K2 switches MICRO/RATCHET/PROBABILITY; K3 adjusts; SEL resets the selected parameter; SEQ/HOME return to the tracker. Existing NOTE/INST/CHD/FX columns remain unchanged.
- **Microtiming:** 0–7 positive offsets, each exactly **1/64 of a step**. Sample-clock scheduling within the step avoids LCD/encoder timing dependence. V0.63 does not support negative (early) offsets; no potentially incorrect pattern-boundary lookahead was introduced.
- **Ratchets:** 1–4 triggers of a programmed step. Events are distributed across the remaining step duration, respecting the micro-delay. Gate values are bounded to prevent ratchet tails from unreasonably overlapping; the original legato setting remains for single notes.
- **Probability:** 100%, 75%, 50%, or 25% per step, applied to note/drum triggers. Instrument FX and STOP events are still processed. A bounded deterministic PRNG is used by the native audio engine without allocation or flash I/O in the audio interrupt.
- **NOTE2 PRESET fix:** on drum channel CH6, physical PRESET now cycles the selected second drum hit → `[---]` → `[STP]` → the previously selected hit. First drum hit/NOTE1 is not edited; NOTE2 STP stops the prior NOTE2 voice where applicable.
- **Compact storage:** 7 previously unused bits in the existing 5-byte step represent groove parameters; the sequencer RAM stride and six-step Undo/Redo capacity do not grow. Sparse project serialization is now **V12**, preserving decoders for V1–V11 (including genuine V0.58 V10 fixture). NOTE2 STOP and groove survive SAVE/LOAD, clipboard and history.
- **WebSimulator:** added STEP GROOVE screen, PRESET encoder and the NOTE2 three-state cycle. Audio scheduling approximates microtiming/ratchets/probability with browser WebAudio. Native FM1 DSP still differs.
- **Validation:** all three JieLi variants compile; portable groove+V12 regression, V10 migration, existing dynamics tests, browser workflow and simulated OTA integrity tests pass. Complete historical suite and physical FM1 performance/recovery remain unverified.
- **Release cleanup:** removed old versioned installers/binaries, unused logs, object files and duplicate websimulators; retained current source, tests, boot art, license notices and release artifacts.

**Safety:** firmware and persistent flash remain experimental; dense projects may exceed the safe A/B journal size. No physical device recovery is proven. Flash only with a verified backup and independent recovery capability.

## V0.62 — Pattern-length-aware cursor navigation (2026-10-14)

- **Tracker K1 cursor:** navigation stops at the current pattern's last active step. Example: length 8 allows steps 00–07 and never focuses steps 08–15. Increasing length makes previously hidden steps selectable again without deleting their data.
- **Dynamics K1 and Block Editor K1/K4:** use the same pattern-length limit. Pasting a block stops at the last active step, preserving hidden content.
- **Cursor recovery:** switching to shorter patterns, Undo/Redo, whole-pattern paste, project restore and playback stop clamp an out-of-range cursor into the active pattern.
- **WebSimulator parity:** same limits, including pattern/undo clipboard and alternate pattern-length editor; browser tests verify these controls.
- **Compatibility:** no audio DSP change and no change to V11 project serialization. Existing 6-step undo history, moon loading animation and all earlier functions remain.
- **Safety:** software validation is not physical FM1 validation. Experimental flash storage and OTA still carry risks.


## V0.61 — HELP K1-only + SEQ/K4 pattern-length correction (2026-10-14)

- **HELP controls:** K1 is the sole scrolling encoder. K4 has no effect while HELP is displayed, including no BPM change. Removed the on-screen K4 heading/page hints in both native FM1 firmware and WebSimulator.
- **SEQ + K4 fixed:** the physical encoder combination is handled before the normal BPM path in the native tracker. While stopped and not recording, it changes the selected pattern length by one step per encoder detent (bounded 1–16), with the change recorded by the existing six-step Undo/Redo history. Turning K4 while SEQ is held can never change BPM, even when playback prevents editing.
- **WebSimulator parity:** holding SEQ while rotating K4 now changes pattern length rather than BPM. Browser PAT LEN −/+ shortcuts remain. A released SEQ chord does not trigger a second SEQ navigation action.
- **No project-format or audio DSP change.** V11 project compatibility, six lanes, Punch-In events, animated moon loading screen, other V0.60 features and existing credits are preserved.
- **Verification:** host tests for the encoder's chord recognition and 1–16 bounds, web UI checks, JieLi builds and simulated OTA checks. These do not establish safety on physical FM1 hardware.

Release notes for documented BEETLE firmware versions, newest first.

For current features, controls, safety information, and licence credits, see [README.md](README.md).

## V0.60 — Sequencer 2.0 & Project LOAD Input Safety (14 October 2026)

- **HELP K4 chapter navigation:** K4 jumps to the next/previous actual section heading, while K1 still scrolls one line. The native FM1 HELP control already had a chapter-search helper; the WebSimulator's outdated three-line K4 paging was corrected. Simulator HELP content now derives from the native `beetle_help.h` section list, including the new Sequencer controls. Scroll limits stay inside the 240×240 frame.
- **SEL after project LOAD:** fixes buffered SEL input from a blocking flash read being interpreted as `CLEAR STEP` once the tracker reappears. A release-aware post-load input barrier consumes the stale SEL edge and requires the button to be released and a minimum 320 ms quiet interval before a new deliberate SEL press can edit notes. LOAD itself retains CRC checking and no unwanted flash writes. The WebSimulator prevents immediate repeated SEL action for the same navigation transition.
- **Sequencer 2.0: variable pattern lengths:** the native tracker supports **1–16 steps per pattern** using **SEQ + K4** while stopped; the current pattern's length survives existing V11 project saves. The WebSimulator now offers the same range and direct `PAT LEN −/+` controls rather than incorrectly advertising 16/32/64 steps that the native engine cannot run. Length changes are undoable.
- **Quantized manual pattern changes:** turning **ALGORITHM during playback** queues another pattern, committed at the current pattern's next step-00 boundary. The queued pattern is visible as `NEXT` and Song Mode stays in control when active. This is bar/pattern-boundary switching, not mid-step switching. WebSimulator timing is approximate; the FM1 uses audio render sample-clock transitions.
- **Stability:** native selectors, display and settings are not rewritten during blocking LOAD. A focused host test covers stuck/queued SEL, held-then-released SEL, timer wrapping, HELP section jumps, pattern-length bounds and boundary switching; Chromium checks the WebSimulator flow. V0.59 project format V11 and existing audio engine are unchanged.
- **Validation:** all three experimental JieLi builds compiled, the V0.59 project migration/CRC regression passed, the new V0.60 host/browser tests passed, and simulated stock/previous-firmware OTA routes passed with `FM-1_2060` and matching SHA-256. **No physical FM1 validation, independent ROM recovery or proven safe flash ownership.**

## V0.59 — Per-step Velocity & Gate / Dynamics (14 October 2026)

- **Per-step dynamics:** `SEQ + FX` opens **STEP DYNAMICS**. K1 selects a step, K2 switches between velocity and gate, K3 changes the selected value, SEL restores its default; SEQ/HOME returns to the tracker.
- **Velocity:** 16 settings including a legacy default, mapped to a bounded note-on velocity. Synth and drum notes receive velocity at the step trigger; it does not silently change mixer faders or existing pattern notes.
- **Gate:** 16 settings including legacy **legato** (0) and shorter 1/16–15/16 step durations. The native synth gate releases notes during the audio render, rather than only at the following sequencer step. Drums adjust their trigger length.
- **RAM-safe five-byte step:** the existing in-memory step size remains **5 bytes** by compacting the instrument, chord, effect, value and dynamics fields; six-level RAM Undo/Redo and the six tracker lanes are preserved. Pattern import/export now has a versioned extended wire representation.
- **Project format V11:** newly saved project records preserve dynamics. A real V0.58 **V10** fixture is used to test migration: old projects load with legacy velocity/gate defaults, with no automatic rewrite. CRC corruption is rejected. Very dense projects can still exceed experimental storage limits and are refused without erasing flash.
- **WebSimulator:** matching DYNAMICS screen, step controls, copy/paste preservation, instrument playback approximation and corrected lane-6 drum routing. Browser rendering and timing remain simulations, not bit-identical native FM1 audio.
- **Validation:** three JieLi native builds, focused ASan/UBSan project compatibility tests, WebSimulator browser test and simulated OTA integrity checks passed. Physical FM1 audio timing, write safety and recovery remain unverified.

## V0.58 — Moon loading screen, original Beetle animation retained (14 October 2026)

- **Static background updated:** replaced only the original tree backdrop with the user-provided `LoadingScreen-background2.png` (240×240) showing the moon in the upper-left and the same tree. The displayed background is not scaled, filtered or animated.
- **Original Beetle unchanged:** the exact two magenta 240×240 sprite masks from the original animation remain in their original position and alternate every 100 ms. The splash loops until the tracker screen is ready. The audio IRQ and bootloader are unchanged.
- **Native + WebSimulator parity:** the native RGB565 compositor and browser simulator use the same user assets. New tests compare both frames over every one of the 57,600 pixels, ensure the background stays fixed and check the animation loop.
- **Release packaging:** firmware ID `FM-1_2058`, rebuilt application variants, experimental `.fwsc` and embedded WebInstaller with matching hashes. No project-file/DSP/shortcut changes.
- **Validation:** software-only compile, per-pixel framebuffer tests and OTA simulations. No proof of physical FM1 timing, flash safety or independent recovery.

## V0.57 — BEETLE Block Editor UI & HELP clarification (14 October 2026)

- **Native Block Editor page guard:** corrected the screen-state validator to accept page 20. Previously it could reset the active Block Editor to the tracker.
- **Block Editor styling:** use BEETLE's existing black/white/magenta pixel UI. Added lane/pattern context, 16-step range visualization, magenta selection frame and visible step occupancy. No PICO-8 styling or external artwork. Native FM1 and WebSimulator use the same 240×240 layout.
- **HELP clarifications:** `SEQ+OP1/OP2` copy/paste **only one selected lane**. `SEQ+OP3/OP4` copy/paste the **entire six-lane pattern plus recorded Punch-In events**. Existing `SEQ+OP5/OP6` Undo/Redo and `SEQ+SEL` Block Editor remain unchanged.
- **No DSP or project format changes:** native firmware recompiled and WebSimulator refreshed. UI is software-tested, not physically validated on an FM1.

## V0.56 — Professional Pattern Editing (14 October 2026)

### Added
- **Full Pattern Copy/Paste:** `SEQ + OP3` copies all six tracker lanes **and** the current pattern's recorded Punch-In events (virtual FX lane). `SEQ + OP4` pastes this entire pattern to the destination pattern. Existing per-lane `SEQ + OP1/OP2` remains available. Pasting is undoable.
- **Undo/Redo:** `SEQ + OP5` / `SEQ + OP6` restore pattern edits. The native FM1 uses a **6-operation bounded RAM history** to fit the actual hardware memory map. Redo is invalidated by a new edit; history is cleared on project LOAD/NEW but retained when switching patterns. Changes are undoable across pattern switches. No flash writes are performed by Undo.
- **Block Editor:** `SEQ + SEL` while stopped opens a dedicated screen for a step selection inside the current lane. K4 sets the start row, K1 the end, K2 chooses COPY/PASTE/CLEAR/TRANSPOSE/SHIFT/UNDO/REDO, K3 changes the amount and SEL applies. HOME or SEQ returns to tracker. Drum notes are deliberately not transposed.
- **Browser controls:** WebSimulator includes working buttons for full-pattern copy/paste, lane copy/paste, Undo/Redo and Block Editing, plus Ctrl+Z/Y/C/V/B keyboard shortcuts.
- **HELP** updated with the new shortcuts.

### Technical and safety notes
- No change to the project serialization format or instrument and audio DSP. Pattern clipboard includes all 16 steps in all six lanes and the stored Punch-In clip. The in-memory undo/clipboard are deliberately not saved to project files.
- Scratch edit buffers use a separate bounded non-initialized RAM section with explicit initialization of ownership flags. No allocation or copy occurs in the real-time audio interrupt, and no flash erase/write is triggered by pattern editing.
- Older project formats and existing audio behavior are preserved. **Physical hardware testing and independent OTA recovery validation are still outstanding; flash-enabled builds are experimental.**

## V0.55 — WebSimulator layout and ARP shortcut parity

Based on V0.54; **UI and web workflow repair**, without changes to the native synthesizer, DSP, project formats, or protected loader.

### Fixed

- **Six-channel MIXER layout:** compact 23-pixel channel spacing fits all six lines and leaves room for the K1/K2/K3 footer. The selected channel, selected parameter, and the bottom row no longer overlap or spill off the 240×240 display.
- **MIXER encoder controls:** K1 now reaches all six lanes rather than stopping at lane 4. K2 only selects valid editable columns. K3 now changes the selected Volume, Pan, Mute or Solo parameter rather than the next field.
- **SONG MODE:** footer and step-row layout adjusted so the eighth visible item does not overlap transport controls.
- **Project slot browser:** all 12 slots fit above a separate footer with a clear divider; no visual overlap at slot 12.
- **Dedicated ARP key in WebSimulator:** enabled as a physical-button simulation and opens SONG MODE immediately. GLO opens/closes MIXER only; second GLO returns to TRACKER. SEQ returns to TRACKER without the obsolete long-SEQ Song Mode gesture. The native FM1 firmware already had the ARP/GLO mapping in V0.54.
- **Restricted browser storage:** a blocked localStorage does not make every slot appear corrupted. Session-only save fallback displays a warning to export a backup.
- **UI audit:** 19 simulator screens rendered in Chromium and audited for graphics outside the 240×240 display. Specific tests cover ARP/GLO/SEQ routing, Mixer K1/K2/K3 actions, and full six-channel visibility.

### Release and safety

- Rebuilt the native FM1 variants with identification **FM-1_2055**, retaining the V0.54 native audio/UI behavior. Updated WebSimulator, experimental WebInstaller, firmware hashes, and `CHANGELOG.md`.
- Software simulation does not establish safety of flashing, data preservation or physical FM1 timing. Keep a verified independent recovery path before installing experimental storage firmware.

## V0.54 — On-device HELP and circular MASTER FX controls

Built on V0.53. No project serialization or audio-DSP algorithms were changed.

### Added

- **PROJECTS → HELP:** new fifth item under NEW PROJECT, opening a dedicated full-screen 240×240 reference containing 43 concise lines covering tracker, shortcuts, per-lane copy/paste, instrument editing, Punch-In, projects, song and MASTER FX.
- **Scrollable help:** K1 scrolls line-by-line; K4 jumps three lines at a time. HOME returns to PROJECTS; SEQ returns directly to the tracker. Both LCD and websimulator bound the visible content to ten rows with scrollbar and position feedback. The copy shortcut explicitly refers to **one lane**, not every lane in a pattern.

### Fixed

- **MASTER FX visual knobs:** replaced scattered point/doodle controls for Compressor and Reverb with filled circular pixel knobs, a coherent outline and attached magenta needle; same K1–K4 parameter mappings and values.
- **WebSimulator parity:** new fifth Projects action, help navigation and circular MASTER control rendering. The audio engine and project format are unchanged.
- **Housekeeping:** V0.54 version identity and installer package updated; historical compiled outputs and superseded release tools cleaned from the source distribution.

### Validation and cautions

- Compiled native JieLi builds and checked on-device help scroll bounds, 5×7 typography and simulator interaction; see `TEST-RESULTS-V0.54.md` for exact scope.
- The experimental persistent-storage OTA build is not hardware-certified. Do not flash without a tested full-backup and recovery route.

## V0.53 — Distinct SID 6581 and 8580 sound characters

Built on V0.52. This release corrects the near-identical sound of the two SID chip-inspired instrument types without changing the existing six-lane tracker workflow or project format.

### Changes

- **SID 6581:** separate, darker nonlinear filter-cutoff response, different resonance behavior and asymmetric saturation to give it a warmer, rougher character.
- **SID 8580:** separate, brighter and cleaner filter response and resonance behavior, without the 6581-style nonlinear distortion.
- **Output level matching:** compensated differences in loudness to keep the comparison focused on timbre rather than gain. Native host tests compared four cutoff positions with near-matched output energy.
- **WebSimulator:** distinct 6581/8580 filter and coloration behavior in the browser. WebAudio is an approximation, not a bit-identical rendering of the FM1 DSP.
- **Regression/compatibility:** existing SID flavors, audio/MASTER FX, tracker, project serialization and reSID/Dag Lem attribution are retained. The original reSID C++ emulator is **not** linked or included.
- **Release cleanup:** removed obsolete generated release artifacts in the V0.53 package and retained the current sources, build outputs, tests and web tools.

### Testing and limitations

- Three native JieLi FM1 build variants compiled successfully in the V0.53 release process.
- Gain-normalized waveform comparisons at four cutoff positions and selected project-storage, MASTER FX and browser-simulator checks passed.
- **Not physically validated on an FM1.** The storage-capable firmware and OTA update path remain experimental; flashing can cause data loss and may not be recoverable.

## V0.52 — SID instrument flavors + MASTER FX stability / UI

This source project is built from V0.51, preserving the six-lane tracker, project workflow, keyboard octave, audio, boot animation and web simulator.

### Changes

- **TYPE selection in EDIT:** encoders queue individual detents and repaint after each step. Left and right browsing retain proper wraparound across all nine instrument types, without skipping types on accelerated encoder input.
- **Five SID-flavored synth types:** `SID 6581`, `SID 8580`, `SID DIRTY`, `SID BASS`, `SID NOISE`. They are distinct character modes on the embedded SID-style synthesizer, not the original reSID C++ library. Existing `RESID-LITE` project instruments map to the first SID variant with the same numeric type ID.
- **MASTER FX screen:** four visible pixel-art rotary knobs for compressor and reverb, with independent K1/K2/K3/K4 control. Compressor adds Attack. Reverb adds Width. Hold SEQ and turn K4 to reorder the chain. MASTER footer fits inside 240x240 display.
- **More robust master DSP:** skip flat EQ calculations, move per-block dynamics/filter coefficients outside the per-sample loop, validate DSP state before dereferencing a reverb index and recover a damaged DSP state without restarting firmware. The audio deadline-stop path automatically retries with capped progressive backoff rather than permanently giving up after the third attempt.
- **Project compatibility:** project encoding schema V10 saves the extra two MASTER parameters; V9 and older supported formats remain readable and initialize missing parameters to defaults. This changes the format only for newly saved files; old project data are not silently rewritten.
- **Web simulator:** latest visual controls and SID type selection; simulated WebAudio remains distinct from native FM1 DSP.
- **reSID credit:** reSID was originally authored by **Dag Lem**. The BEETLE SID types are independent SID-inspired implementations; original reSID source is not embedded. See the [acknowledgment in README.md](README.md#resid-credits-and-acknowledgment) and `source/CREDITS-AND-LICENSES.md`.

## V0.51 — Responsive keyboard + WebSimulator

Complete source for the M-VAVE FM1 experiment, based on V0.50.

- **Shared OCTAVE**: EDIT and TRACKER use the same keyboard octave (-4...+3). Press and release OCT- or OCT+ to move an octave. In EDIT both together still toggle MONO/POLY without shifting the octave. Playing notes from the physical keyboard uses the selected octave on both screens. Existing pattern notes are never transposed by changing the keyboard octave. K3 remains the note/instrument cell editor.
- **Correct note-offs**: live EDIT/TRACKER voices remember the actual MIDI note and release that exact note after changing octave or channel. Held EDIT notes are retriggered under the updated octave.
- **WebSimulator**: `BEETLE-V0.51-WebSimulator.html` is a browser-only simulator, updated to six lanes and 12 project slots. The same shared keyboard-octave control is simulated, and the original animated beetle boot art is included. It uses WebAudio rather than the exact native JieLi DSP, so audio parity with the FM1 is not claimed.
- **Source cleanup**: only current release firmware, latest webinstaller, simulator, source, fixture/reference files, production toolchain scripts, and retained regression tests. Old generated binaries, historical installers, duplicate art and build logs excluded.

## V0.49 — Fast Project Workflow

Based on the **complete V0.48 source**. All previous audio, tracker, animated boot, cursor and six-lane indicator functionality remains included.

### Changes

1. **LOAD → unsaved confirmation → DISCARD → full project picker**. DISCARD creates a clean session and opens the 12-slot browser. It never loads the currently highlighted slot implicitly. Selecting a slot in the browser then loads it with full project CRC validation.
2. **Faster project browser**. Only the two 32-byte A/B bank headers and at most 36 bytes from each selected bank are read for a *provisional* title preview (up to 136 bytes per slot, rather than scanning ~24 KiB plus complete decode). One selected slot and one other are processed per poll. Full reads/CRC/ownership checks are still used for actual LOAD and SAVE. No flash writes are allowed from a preview.
3. **SAVE and SAVE AS respond visibly before flash operations** with a reading/writing status screen. Flash operations are synchronous for integrity; interruptions/power-off during a write are still unsafe.
4. **Successful save returns directly to the tracker** with an approximately 1.3-second `PROJECT SAVED` confirmation, without needing SEQ. Errors still show an explicit failure page. First-time storage provisioning retains the five-second hardware confirmation and returns to tracker only if the save succeeds.
5. **Saving the currently opened project** skips the redundant overwrite dialog and unnecessary full project decode for checking slot occupancy. SAVE AS to another occupied slot still requires confirmation; the full journal safety checks remain unchanged.
6. Browser scans now reuse the boot-time flash ownership audit instead of reading the marker sector repeatedly.

### Tests and limitations

- Native builds: read-only diagnostic, OTA pilot, experimental BEETLE storage.
- Host tests: prior V0.46-through-V0.35 regression paths, native V0.48 UI checks, V0.49 provisional preview A/B tests and static navigation-path checks.
- OTA installer: simulated routes + cryptographic hash and package checks. **No hardware validation**.
- Project title preview is deliberately unverified. A slot is only trusted after a full CRC-checked load; journal writes still inspect banks and reject foreign data.
- Physical flash erase/program may still take noticeable time. The status screen gives immediate feedback, but this does not imply interruptible/background flash writes.
- **WARNING**: experimental storage requires explicit first-use destructive initialization, may overwrite older ETCH/Felucca user-storage data, and firmware flashing has nonzero brick risk. An independent rescue route remains unproven.

GPL-3.0-only; existing licenses retained.

## V0.45 — Isolated Overwrite, Persistent Step FX & Stable Loop Loudness

This version fixes three concrete regressions in BEETLE V0.44:

1. **SAVE / OVERWRITE is slot-local.** The UI now states "ONLY THIS SLOT CHANGES". A journal commit addresses only the selected slot and its two A/B banks; the other eleven slots are untouched. On an already-provisioned volume, a failure to recognize flash ownership causes a **non-destructive error**, never a prompt to format everything. A genuinely blank, first-use volume can still be initialized separately with an explicit five-second SELECT+SAVE confirmation; **that initial format erases all twelve slots** and may destroy pre-existing ETCH/Felucca data. There is no automatic erase at boot.
2. **Synth step effects sustain over empty steps** until `[STP]`, a new note, or another step effect on the same lane. The latter replaces the current effect on an already sounding voice. An effect-only PITCH row retunes active oscillator(s) without retriggering ADSR. Each lane has an independent effect-age counter that does not restart at every step boundary. Stop and project load clear stale automation.
3. **Loop loudness no longer falls because the voice-pool high-water count increases.** Headroom normalization uses currently sounding *unreleased* synth voices, not previously allocated voice slots. The regression renders fourteen consecutive note retriggers and asserts a stable late-window energy.

The existing **V8 project serialization is unchanged**. V0.37/V0.40 migration fixtures remain in the regression suite. The V0.44 ROM digital drum kit and stock-V15 installer identification remain included and unchanged.

**Software validation only.** The three JieLi builds and host/OTA integrity checks pass, but the FM1 hardware, physical flash map, and real stock-flashing/restore path have **not** been proven safe. The storage-enabled build can erase data when the user deliberately provisions the volume. Make a complete verified backup and establish independent recovery before hardware flashing.

Run `bash tests/track/run_v045.sh` from `source/` and `node tests/ota/test_stock_install_v045.cjs` for the new tests.

## V0.44 — A sample-ROM digital drum kit (14 instruments / 16 one-shots)

V0.44 replaces the former short sine/noise-based drum sounds with 16 fixed
**original, offline-synthesized** drum one-shots (14 distinct instrument pads,
plus one alternate snare and one alternate closed hi-hat for round robin).
These are digitally generated sources, not acoustic drum recordings; they aim
for a punchy early-digital-drum-machine sound rather than acoustic realism.
The source generator is `tools/create_v044_kit.py`. No commercial or external
sample packs are incorporated. The native voice engine performs 22,050 Hz
µ-law-ROM decoding, 44,100 Hz linear interpolation, live pitch conversion,
three editable tone bands, gain, envelope and hi-hat choke. There are six
software drumvoices; the ROM is not copied to RAM or saved into projects.

**Before installing, listen to** `audio/BEETLE-V0.44-Factory-Kit-Groove.wav`
and the 14 individual WAVs. Both are rendered by the *compiled BEETLE C audio
engine* on a desktop, not recorded from physical FM1 hardware. This does not
prove sound quality or absence of real-time glitches on the device.

New projects start with the existing per-instrument digital-kit control values.
Older V6, V7 and V8 projects can still load; stored EQ, pitch, gain and envelopes
are not erased by the new audio engine. A custom project may need the DRUM EDIT
factory-reset command to get the intended default mix. The on-disk project
format remains V8. ROM data sits in existing firmware space and does not change
the BEETLE storage partition or protected bootloader.

Host test: `bash tests/track/run_v044.sh`. Generate the same sample ROM and
preview references: `python tools/create_v044_kit.py` (requires NumPy/SciPy).
Render through the native C engine:
`cc -std=c11 -O2 -Ifirmware/fm1-native tests/track/preview_beetle_v044.c firmware/fm1-native/{track_drum,beetle_drum_rom,track_mixer}.c -o /tmp/beetle-preview && mkdir -p /tmp/beetle-wavs && /tmp/beetle-preview /tmp/beetle-wavs`.

**Legal/credits:** The PCM one-shots are newly generated by the included Python
script. They do not use third-party audio recordings; the existing core project
still derives from ETCH and upstream GPL-3.0 Felucca code, as documented in the [README credits](README.md#open-source-origins-credits-and-licences). The toolchain remains the separately provided JieLi toolchain.

**Safety:** hardware validation, stable OTA flashing, ROM recovery, and physical
project-storage partition ownership are **not** proven. Existing installation
remains an experimental operation with risk of data loss or bricking.

## V0.42 — Boot graphic and OTA restore safeguards

- **Power-on animation:** the BEETLE play/insect glyph is the only artwork, displayed at 3× nearest-neighbour scale on black. Its two frames are shared with the tracker and app-owned OTA visuals.
- **BEETLE app-controlled OTA progress:** dark violet/magenta Matrix pattern and animated play glyph. The text `ETCH` has been removed from `native_ota_matrix.h`. This is *not* the separately executed protected SPL/OTA-loader interface. The original loader image is deliberately unchanged.
- **V15 factory restore gate:** app-side firmware-check logic no longer compares the first 16 KiB with installed custom firmware **only** when it first identifies a stock package with the exact known V15 flash.bin SHA-256 and official OTA-loader length/CRC. For ordinary custom-firmware updates, both strict custom-loader SHA-256 pinning and the 16 KiB header comparison remain active. Unknown packages are refused.
- **Legacy OTA ownership:** earlier ETCH OTA stage markers remain recognized, while freshly staged BEETLE updates use a BEETLE-specific marker.
- **Installer:** BEETLE package is embedded and SHA-256 checked; an optional original V15 restore requires a user-provided, exactly SHA-256-pinned factory .fwsc. The restore flow is **not** independently proven safe on a physical FM1.

### Safety and limitations

This is a **source-level and host-tested correction**, not a verified real-device recovery feature. A matching SHA-256 only authenticates the supplied stock *flash.bin* image. The stock OTA loader is checked via known length, entry structure and CRC16 rather than a separately available SHA-256 reference; CRC16 is not a cryptographic signature. Verify the factory distribution and a full flash recovery path independently before using the restore option.

The protected loader and first 16 KiB from the ETCH V1.37 baseline are **intentionally retained bit-for-bit** in the V0.42 BEETLE firmware container. Once control passes from BEETLE to that loader, it can still display its own inherited branding. The app-level Matrix screen cannot replace its LCD implementation. Bootloader replacement requires a distinct high-risk recovery-validated project.

**On an FM1 running BEETLE V0.41 or another older firmware, the old CHECK restrictions remain active until V0.42 itself is successfully installed.** An updated web page alone cannot fix the installed V0.41 firmware's checks.
