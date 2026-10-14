# BEETLE Changelog

Release notes for the documented BEETLE firmware versions, newest first. Not every intervening version has its own entry in the supplied release notes.

For current features, controls, safety information, and licence credits, see [README.md](README.md).

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
