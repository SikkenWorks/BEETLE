## V1.3.8 · 2026-10-20

- Fixed native USB-MIDI SysEx firmware ID: match FM-1_2123 to actual FWSC package (prior V1.3.7 app erroneously reported FM-1_2113).
- Installer now waits for the expected identity following reboot rather than rejecting the first stale firmware reply; preserves protocol handshakes and loader/hash verification.
- All-in-One BEETLE MANAGER uses same-origin Blob documents, an explicit top-level AUTHORIZE FM1 MIDI prompt and direct-opening option for browser Web-MIDI permission limitations. MIDI/fullscreen allowed in embedded tools.
- WebSimulator/Workbench adds fullscreen 240x240 LCD with integer nearest-neighbor scaling and optional REC TAB + AUDIO (MediaRecorder WebM, requires explicit tab audio grant; not hardware USB audio).
- README badge added for The Firmware Hub. V22 project format remains unchanged; firmware OTA gate remains fail-closed for untrusted loader layouts.
- Builds/tests: software only; physical FM1 flash/recovery unverified.

### V1.3.7 physical FM1 SEL/SETTINGS fix

- Implemented a dedicated native physical SEL button transaction. SETTINGS entry now consumes the select edge, has a trailing-edge fallback for an interrupted poll, and blocks USB toggles until a separate SEL press after release.
- Added a host regression for physical matrix bit 3, short press, slow press, release, duplicate prevention, and unrelated PROJECTS/SEQ/HOME navigation.
- The exact user-provided BEETLE-MANAGER.html design now embeds V1.3.7 versions of all three tools.

## V1.3.7 — USB settings clarification / unified 3-in-1 Manager (20 October 2026)

- Corrected native FM1 screen validation: **SETTINGS page 38** was accidentally rejected by a stale whitelist ending at page 37, immediately forcing the hardware back to TRACKER. Native LCD validation and input polling now explicitly recognize SETTINGS. Added a compiled test for the physical SEL press/release transaction.
- Replaced ambiguous **SIMULATION ONLY** on the WebSimulator USB Settings page with browser-specific labeling; the browser cannot capture FM1 USB multitrack, but the native firmware still carries the real 2/6-channel UAC1 switching code. No claim of verified FM1/DAW operation.
- Author-provided `BEETLE-MANAGER.html` becomes an always-included fourth HTML deliverable, with the exact matching emulator, Project Manager and embedded-firmware WebInstaller packaged inside one self-contained page. All three remain available as separate HTML files.
- Retained V22 project compatibility, three independent step FX, HOME/SETTINGS navigation and native six-stem capture logic. No deliberate audio DSP or USB descriptor behavior changes.

## V1.3.6 — HOME / SETTINGS navigation fix (20 October 2026)

- Fixed an undefined HOME/REC press-timing variable in the author's embedded BEETLE Workbench. This caused HOME to throw a browser error and left subsequent SEL handling in the wrong screen.
- The native firmware now routes PROJECTS → SETTINGS via a dedicated SEL press-edge guard, preventing the opening press from reaching another screen action. The existing USB Multitrack ON/OFF switch remains inside SETTINGS.
- Fixed an existing V22 WebSimulator import crash caused by modifying a `const` FX type during binary decode.
- On-device HELP and the embedded WebSimulator document K1 highlighting a menu item, SEL entering it, and SEL toggling USB mode inside SETTINGS.
- No changes to project schema (V22), synth/drum audio, FX, Song Chains, USB descriptors or project slots.
- Software-only regression tests and native builds; physical FM1 navigation, USB enumeration and flashing/recovery have not been verified.

## V1.3.5 — Independent FX1/FX2/FX3 and revised Workbench (20 October 2026)

- Adopted the author's revised `simulator(2).html` Workbench, preserving device geometry, styling and native `.beetle` project slots and import/export. Its embedded engine is refreshed from matching V1.3.5 source.
- **PER STEP FX:** K1 selects FX1/FX2/FX3 vertically, K2 scrolls available FX types, K3 changes the selected effect's parameter. All three slots can be populated independently from the beginning; OCT+ / OCT− support selection/removal. CH6 NOTE1/NOTE2 changes via SELECT.
- Fixed the root bug where FX2 required populated FX1 and FX3 required populated FX2. Also corrected missing DSP activation when FX1 is empty, isolated deletion, no ghost slot migration, and correct `MUL` indicator for 2+ active effects.
- **TRK1 V22** carries the explicit FX2/FX3 identity within the existing record size. V21 projects migrate on load, V19/V20 remain readable; V22 is not backward-readable by older firmware. 48-entry pool and `FX STORAGE FULL` overlay remain in place.
- The black FM1 piano keys continue to enter true chromatic tracker notes, with existing SEQ/OP and mixer shortcuts preserved.
- Firmware/installer remain experimental: firmware compilation and software-only regression tests do not replace physical FM1 recovery, flash and audio validation.

## V1.3.4 — Three Step FX Slots (experimental, 20 October 2026)

- FX1 + FX2 + FX3 in the existing PER STEP FX menu. Additional effects use a project-wide, 48-record, bounded pool; no silent overwrites. `FX STORAGE FULL` overlay on rejected insertion, `MUL` label for multiple FX, separate CH6 NOTE1/NOTE2 values.
- Synth DSP sequences primary and secondary effects; drums retain pitch and volume effects. FX edits, clearing lanes/ranges and full-pattern copy/paste include the additional FX and Undo/Redo state.
- TRK1 V21 project format stores sparse extra FX and supports reading V19/V20. Older firmware cannot read V21. Existing 12,256-byte project-size cap is unchanged.
- Source-level compilation for three JieLi variants, native audio and compatibility regressions, simulator syntax and package consistency tests. No physical FM1/USB/flash/recovery validation.

## BEETLE V1.3.3 — USB multitrack recording and range instrument/delete editing

- **USB Multitrack in SETTINGS:** ON enumerates a six-channel 44.1 kHz/16-bit UAC1 recording source, CH1–CH5 mono synth lanes and CH6 mono drums. OFF retains post-master stereo input, stereo panning and global effects. Re-enumeration occurs on toggle; host DAW must reopen its USB inputs. OFF is the power-on default; this mode is *not saved to flash or project data*.
- **Real lane stems:** native C audio engine captures six separate pre-master/pre-global-Punch-FX buses, including lane volume and mute/solo, without turning the stereo output into six copies. Mono stem outputs exclude lane panning by design. The common fractional USB read position keeps all six channels sample-synchronized. Physical USB enumeration, full-load DSP performance and DAW compatibility remain **unverified**.
- **RAM and USB safety:** allocated 6-channel PCM staging and USB rings in newly verified spare RAMTEXT-region space without reallocating the protected firmware header or OTA loader. Preserved the original 8-bit SIE EP4 register setup while advertising correct UAC1 full-speed 552-byte maximum isochronous packets. Real-hardware register behavior is still unverified.
- **Block instrument changes:** select a range with SEL+K1, move to instrument column and change K3 to assign one patch across all existing notes in the active synth lane. Does not create notes on empty/STP steps; one Undo/Redo entry.
- **Block deletion:** short SEL while a range exists clears all selected steps in one Undo/Redo entry, including drum NOTE2 step FX. SEL+K1 while a range exists starts a new range instead of deleting the old one.
- **Compatibility:** existing V20 project schema, eight active patches, 12 oscillator types, separate CH6 drum engine, 64 Song Chains rows, Punch-In, project transfer, analog stereo, USB MIDI and HELP chapters preserved. HELP has 13 chapters including the new selection and USB instructions.
- **Validation:** native JieLi read-only, OTA pilot and storage-lab builds; six-lane PCM isolation tests; range tests; app/OTA structural verification and embedded installer SHA-256 verification; JS simulator syntax and HELP parity. **No real FM1 USB/DAW/OTA/recovery validation.** Experimental flashing may cause data loss or render the FM1 unusable.

## BEETLE V1.3.2 — Fixed instrument identity, note/range OCT, Retro Wave audio

- **Patch identity bug fixed:** removed implicit copy-on-write allocation during EDIT. INST 1–8 are stable, fixed shared patches. Editing instrument 3 changes instrument 3 wherever referenced; opening EDIT on an existing note selects that step's INST. Other instruments do not silently become INST 1 or migrate to a free slot.
- **Eight factory patches confirmed:** 1 RETRO PULSE, 2 RETRO WAVE, 3 VA OSC, 4 VA OSC2, 5 VA BASS (VA OSC / octave B), 6 VA PAD (detuned VA OSC2), 7 SID 6581, 8 NES PULSE. These are editable startup sounds; *waveform types* are independently selectable within each patch. CH6 remains the separate ROM drum kit.
- **Patch parameters:** Audition (TRACKER + EDIT) and sequencer use the instrument's duty, cutoff and per-patch VOLUME, with the existing factory level (65) treated as unity for backward loudness compatibility.
- **OCT on stored notes:** when stopped/disarmed, OCT- and OCT+ transpose the selected synth note or all notes within an existing SEL+K1 selection by exactly -/+12 MIDI semitones. Each note is clamped to 0–127; empty/STP rows, instrument numbers, drum notes, other lanes and chord modes are untouched. One edit supports Undo/Redo. Existing SEQ+OCT shortcut also supports ranges. An empty tracker step without an active note/range leaves OCT in its legacy live-keyboard mode.
- **RETRO WAVE:** 32-step built-in wavetable now uses lightweight linear phase interpolation to mitigate stepped high-frequency/zipper artifacts. It is still retro 4-bit-like synthesis, not high-fidelity sample playback; the audio PCM regression hash changes intentionally.
- **HELP / WebSimulator / README:** all updated. Simulator offers Shift+click to select a range of steps and OCT buttons for range transposition. Native 12-chapter HELP contains current factory patch and octave instructions.
- **Project schema remains V20**; V19 compatibility, eight active patches, eight library presets, 64 Song Chains positions, Punch-In recording, USB audio, stereo panning and both VA oscillators are retained. Firmware flashing, USB sound and CPU timing remain unverified on a physical FM1; full-flash backup and independent recovery are required before experimental installation.

## BEETLE V1.3.1 — Eight active synth instruments, separate drums

- Expanded active synth instrument patch slots from **4 to 8**, each independently editable and addressable by tracker INST 1–8 (0–7 internally); all **12 waveform/oscillator types** remain available in *each* patch, including VA OSC and VA OSC2 with dual A/B waves.
- Five synth lanes (CH1–CH5) remain eight-voice shared polyphony. **CH6 is the dedicated drum kit**, and its tracker column says KIT/ROM rather than implying that INST 4 is the drums.
- EDIT and HELP now display INST 1–8; the existing eight-slot project-local Instrument Library remains separate and unchanged.
- **V20 project serialization:** adds 3-bit synth instrument references and eight actual synth patches and TYPE banks. Older V19 files load without changing the first four patch sounds. V20 cannot be opened by V1.3 and earlier firmware. Slot capacity remains 12,256 bytes, with preflight protection against oversized projects.
- Fixed the empty-step migration so legacy CH5 default instrument values do not inflate sparse projects.
- The simulator's binary import/export and JSON backup conversion also support 8 patches and migration from V19/four-slot backups.
- Retains Song Chains (64 positions), 48 patterns, two VA oscillators, drums, Punch-In FX, USB-C stereo audio, panning, effects, backups and credits.
- JieLi native build, OTA-package structure and original protected loader/header checks pass. Linker pool expands 3 KiB, reducing available stack reservation. **Hardware audio/stability, USB, OTA flashing and recovery remain unverified.**

## BEETLE V1.3 — Song Punch-In, ghost FX, polyphonic loudness and stereo PAN

- **Song recording:** REC+PLAY now records physical Punch-In keys into the active Song Chains **CH1 pattern**, not the unrelated tracker cursor pattern. A held, recorded FX spanning a Song position boundary closes in the prior CH1 source and opens in the next source. Source `--` deliberately does not save FX.
- **Ghost FX guard:** Fresh PLAY never inherits an FX key from a previous looping pattern. The REC key latch is initialized from actual hardware key state; only fresh physical down events may be recorded, and orphan up events are ignored. A new FX take removes only stale events for the same FX in that source pattern. Explicit stop closes held recorded effects; no stray data is inserted at playback start.
- **Chord/single-note balance:** Replaced nearly linear phrase polyphony attenuation with bounded power-style Q8 headroom normalization, smoothed in the render ISR. The note-on ADSR, velocity and individual oscillator balance remain unchanged; the existing 8-voice limit remains.
- **Real stereo panning:** Synth and drum lanes now use centered, deterministic equal-power L/R gains computed once per audio block. The stereo post-master `left,right` pair is copied unmodified to the 44.1 kHz, 16-bit UAC capture endpoint. The scope-only mono path is never used for USB capture.
- **Instrument numbering:** All instrument edit pages display the **INST 1–4** patch number at the top, matching the 1-based instrument cell in the tracker. HELP and WebSimulator updated.
- **Compatibility:** V19 project layout unchanged, 64-song-position model and 48 source pattern clips retained, no memory/flash partition changes, protected firmware header and OTA loader preserved. If multiple Song positions refer to the same CH1 source pattern, their Punch events remain shared, as before; this is not 64 independent Song FX lanes.
- **Tests:** Host PCM RMS and L/R isolation for synth/drum; fresh transport/CH1 recording route tests; native pi32v2/OTA-lab build and app audit; playback stability, V19 migration and UAC clock simulation. Physical USB recordings, device-side timing and safe OTA recovery are still **unverified**.

## BEETLE V1.2 — USB capture continuity and oscillator loudness balance

- **USB-C recording:** The 44.1 kHz UAC1 endpoint now delivers the regular 44/45-frame host cadence without inserting/removing whole packet samples to chase physical codec drift. A slowly adjusted Q16 fractional read position performs light linear PCM interpolation, with bounded correction and existing UAC underflow/overflow diagnostics. The legacy optional 48 kHz path is unchanged and **not enabled** in this release. USB Audio input, analog output and USB-MIDI remain enabled.
- **Native sound balance:** Per-shape gain calibration for all five VA waves, plus fixed gain compensation for the 12 oscillator/instrument types, based on measured C-renderer RMS. These gains are fixed by the selected type, not dynamic compression. Velocity, ADSR, mixer level, drive, per-track effects and patch parameters remain editable.
- **Measurements (software):** Before: clean C4 note across types from ~3,373 to ~17,028 RMS (over 14 dB variation). After: using shipped default clean patches, ~6,676–6,995 RMS (<0.5 dB); the ten VA type/shape tests also pass the same tolerance. A two-minute independent 44,117.6 Hz codec vs 44,100 Hz host ring test produced 5,292,000 samples with zero simulated underruns and overruns. These are fixed-configuration software measurements, not loudness matching across all notes, settings or mastered projects.
- **Memory:** Moved only USB audio state into its dedicated boot-zeroed `.usb_audio_ram` linker region. No protected loader/header changes; JieLi linker and app audit pass.
- **Compatibility:** No project format change (V19). Retains 12-slot storage, 64 Song Chains positions, 12 HELP chapters, five synth channels plus drum channel, Drum NOTE2, backup tools, mute groups, instrument library and existing FX.
- **Experimental hardware warning:** No real FM1 USB audio/analog A/B recording was supplied or tested. Endpoint electrical behavior, actual host OS recordings, DSP deadline performance, USB interruptions, boot and independent stock OTA restore still need hardware validation. An exact firmware hash is not proof of safe flashing.

## BEETLE 1.01 RC1.1 — HELP chapters + USB-C stereo capture

- Replaced HELP's continuous scrolling list with **12 topic chapters**. **K2** changes the topic and **K1** scrolls only within it; chapter change returns to its first line. On-device page counter and scroll thumb show progress within the selected topic. HOME and SEQ continue to exit safely, without altering any saved data.
- Updated the WebSimulator HELP view to match the native chapter structure and navigation. The simulator still uses WebAudio only, not a real USB audio peripheral.
- Enabled the existing **Felucca-derived USB Audio Class 1 endpoint** in the bundled BEETLE FM1 build (`--usb-audio`): 2-channel 16-bit PCM at **44.1 kHz**, recording the post-master BEETLE output to a computer via the FM1 USB-C port. No firmware-side screen capture or USB playback sink added. Analog output and USB MIDI remain in the build. The optional 48 kHz resampler is deliberately not enabled because it has not been hardware performance-tested.
- Conserved control RAM by moving only RC diagnostic counters into the existing RAM pool. JieLi linker/app audit, original OTA loader identity bytes, and the protected first 16 KiB of flash are checked. New firmware identity: `FM-1_2113` (base: `FM-1_2112`).
- Project format **V19 unchanged**, so the Song Chains, note/FX editing, patterns, library, storage and other RC1.0.11b functionality remain sourced from the same code base.
- **Experimental release**: no physical FM1 verification for USB enumeration, real-time audio/multi-effect headroom, or OTA/recovery. Software success is not a hardware-safety guarantee. The included installer pins the exact new `.fwsc` SHA-256 and preserves its device/firmware checks.

## BEETLE 1.01 RC1.0.11b — SONG CHAINS AUDIO STABILITY (experimental)

- Fixed the native DMA deadline overload policy: it previously reduced the inherited ETCH voice counter, which does not control BEETLE synthesizer rendering. BEETLE now reduces its real synth voice budget from 8 toward 4 only when recurring audio deadlines are missed, with gradual restoration after sustained healthy blocks. An emergency budget reduction may temporarily steal quiet/released voices to maintain audio continuity.
- Cached mixer mute/solo/volume/pan per audio-render chunk instead of repeating a six-lane solo scan for each voice on each sample. No changes to normal PCM mixer semantics.
- Optimized SONG CHAINS composition for full 16-step lanes with contiguous copies, preserving variable-length lane repetition and NOTE2 FX.
- Replaced inherited ETCH PLAY/AUDIO STOP fallback with a BEETLE-specific AUDIO STOP recovery screen. Automatic/manual restarts reset tracker DSP state without writing or modifying saved projects.
- Inspected user-provided V19 backup with 64 song rows; row 50 is valid, with 48 note events; row 52 contains 50 note events. This is consistent with high audio load but does not substitute for measured hardware DMA timings. The backup is not distributed in release files.
- Updated release firmware identity to `FM-1_2112`; protected boot header, OTA loader, V19 schema and storage regions unchanged. Native build and packet CRC/hashes verified; hardware installation, full-length endurance and glitch-free operation unverified.

## BEETLE 1.01 RC1.0.11 — SONG CHAINS (per-lane song arrangements)

- Reworked native Song Mode to reference a separate pattern for each of CH1–CH5 and DR per song position (01–64). Columns **POS, CH1, CH2, CH3, CH4, CH5, DR, REP** remain labeled at the top of the song screen.
- K1 navigates song positions; K2 selects a lane or REP; K3 edits its pattern reference (01–48; `--` mutes that lane) or repeat count 1–16.
- OCT+ inserts a new row defaulting all lanes to the pattern corresponding to the new displayed position. Positions 49–64 default to P48. Existing rows retain custom lane assignments; OCT- removes a row.
- Song playback composes six independent lane sources without modifying original 48 patterns; short lanes repeat within longer rows. NOTE2 drum FX are sourced with the drum lane. Global recorded Punch-In automation uses the CH1 source pattern.
- Native project schema **V19** stores six lane references per song row; native V1–V18 projects remain readable and each legacy row automatically maps its original pattern number to all six channels. V19 must not be imported into pre-V19 firmware.
- Updated native HELP, browser HELP, README controls table (original layout preserved), simulator and Project Manager. Added C source/codec tests for independent source isolation, V18 migration, row insert defaults, song mute and drum NOTE2 FX; browser tests for the song editor and V19 codec.
- Experimental firmware ID `FM-1_2111`; unchanged protected 16 KiB bootheader and OTA loader from RC1.0.10. Hardware boot, audio performance, USB storage and recovery **not** yet validated on an actual FM1.

## BEETLE 1.01 RC1.0.10 — Project SAVE / LOAD workspace fix

- Fixed native A/B journal SAVE and LOAD rejecting the FM1's actual 12,256-byte project workspace: the validation mistakenly required a 24,576-byte legacy decoder buffer. The defect affected all physical project saves and loads regardless of project data.
- Fixed project encoder receiving a false 24,576-byte capacity for a 12,256-byte physical workspace; now passes the real capacity to prevent out-of-bounds writes.
- LOAD verifies the selected project's payload fits the supplied workspace before reading it. Oversized saves are rejected before flash erase.
- Preserved 12-slot A/B partition, V18 project schema, ownership/foreign-sector protections, project-transfer protocol, OTA loader and original protected boot header.
- Added regression tests with the actual physical workspace, guard bytes, all 12 slots, repeat save/no-erase, A/B recovery and too-small buffer rejection.
- New firmware identity: `FM-1_2110`. Software builds/tests are not physical FM1 validation.

## BEETLE 1.01 RC1.0.9 — Cross-Firmware OTA Native Compatibility Audit

- Fixed the native APP_ONLY OTA CHECK bug that rejected compatible custom packages solely because the unused first 16 KiB protected boot header differed (`-5`).
- Native firmware now validates chip key `0x980F`, configuration CRC, `app.bin` at flash `0x4000` / XIP `0x02000120`, the directory, and full app/app-area CRCs BEFORE OTA staging is erased.
- Improved UFW bounds, duplicate-entry and payload-overlap checks. Unknown OTA-loader bodies are still rejected; the approved, SHA-256-pinned loader and official V15 separate restoration path remain unchanged.
- The compatible installer applies the same APP_ONLY tests for custom packages rather than requiring an unchanged unused boot header. Source identity alone never permits flashing.
- Added full native USB-MIDI CHECK replay regression cases using actual BEETLE/ETCH FWSC image bytes, plus corrupted app and altered loader negatives. New runtime firmware identity: `FM-1_2109`.
- **Hardware status:** real FM1 cross-firmware transitions, especially Felucca originals/stock and repeat reinstallation, remain unverified. This is a software-validated experimental candidate.

## BEETLE 1.01 RC1.0.8 — Confirmed Current Lane Clear

- **SEQ + rightmost black key (F#5)** in TRACKER opens a YES/NO confirmation. NO is default; K1 selects and SEL confirms; HOME cancels. Only while transport and REC are stopped.
- YES clears all sixteen rows of the selected lane in the current pattern, including synth notes, chords, FX, dynamics and groove, or CH6 NOTE1/NOTE2 and their independent FX. Other lanes/patterns remain unchanged. The operation is undoable.
- WebSimulator and on-device HELP updated. The existing README layout and controls table have been preserved and extended.
- Firmware build / OTA package must be verified separately before hardware use; physical testing remains pending.

## BEETLE 1.01 RC1.0.7 — Inclusive Step Selection

SEL+K1 now includes the original cursor row in every selection, in both directions, and captures the anchor when SEL is pressed. Simulator and HELP updated. The V18 project format and the existing OTA flow are unchanged; target firmware ID is FM-1_2107.


## RC1.0.6 HF2 — B1 WebInstaller transport & protected header correction

- Compiled the RC1.0.6 tracker source as `FM-1_2106` for reliable post-boot identity checks.
- Repacked firmware with the byte-exact 16 KiB protected header and OTA loader from previous compatible BEETLE release (matches B1 HF2).
- Integrated the verified firmware into a BEETLE-branded WebInstaller built on the user-provided B1 HF2 transfer workflow.
- Retained V18 independent drum FX, lane rotation, selection/copy/paste, Project Transfer, and existing tracker features.
- OTA CHECK/loader transitions have been software-simulated, but no physical FM1 recovery or install has been demonstrated.

## 1.01 RC1.0.6 — Development / tracker editing and V18 drum FX

- SEQ+ALGORITHM step rotation, SEL+K1 range selection, partial copy/paste.
- Drum NOTE1 and NOTE2 now have independent FX settings; native V18 serialization, V17 migration and updated simulator.
- Project Manager rejects V18 projects sent to V17 firmware.
- OTA loader-transition failure from installed RC1.0.4 remains unresolved; RC1.0.6 is not hardware-certified or flash-ready.

# RC1.0.5 — Double-Confirmed BEETLE Project Storage Wipe

- FM1 PROJECTS adds **WIPE ALL STORAGE**; K1 choose NO/YES, SEL, two independent dialogs **ARE YOU SURE?** and **REALLY?** with default NO.
- Explicitly erases/verifies the 288KiB BEETLE project banks and reinitializes its 4KiB marker only after owner/marker audit; all 12 slots become `[EMPTY]`.
- Explicit double-YES permits recovery of damaged/unknown marker in the fixed project region, including destruction of any legacy ETCH/Felucca user data there; SAFE/OTA/playback gates; sector/marker verification. Not a generic whole-chip flash reset.
- WebSimulator mirrors both dialogs for browser-local projects; firmware/project-file format unchanged.
- WebInstaller rebundled with the audited RC1.0.5 firmware; Project Manager version label updated.
- Host NOR-flash test and legacy project-delete/LED/crypto tests pass; physical hardware not validated.

---

# BEETLE 1.01 RC1.0.4 – Reverse Search / Corrupt Storage Diagnostics

- Fixed Reverse Key Search immediately returning to Tracker. Page 32 is a valid modal page and is no longer rejected by the old >30 validator. Page 31 remains invalid.
- Project Transfer V4: read-only audit of the global BEETLE storage marker, every A/B bank and marker sector; optional, double-confirmed marker-only restoration when independently verified BEETLE-owned and safe. It does not remove corruption of unknown origin.
- Refuses marker repair for foreign or unreadable flash, no owned project bank, or a marker with unknown bytes. Physical slot recovery V3 remains protected and can be retried only after the marker is verified.
- WebSimulator and WebInstaller display RC1.0.4; Project Manager V1.7 uses the same firmware protocol version.
- Firmware binaries have not been validated on real hardware. Before installing or performing marker repair, obtain a verified full-flash backup and independently working recovery route. OTA/stock compatibility policies are unchanged.

# BEETLE 1.01 RC1.0.3 — LED ripple cleanup and consistent version labels

# BEETLE 1.01 RC1.0.3

## Wijzigingen

- Opstartanimatie op de fysieke FM1-LEDs is nu **eenmalig**, niet herhalend.
- De ripple krijgt de volledige tijd om door alle 41 LEDs te lopen (1320 ms splash; 1200 ms eenmalige animatie), inclusief de buitenste functietoetsen.
- Bij de overgang naar de tracker worden alle resterende dim-, mid- en breath-LED-lagen gewist. De normale toets-, mute-, octaaf- en KEYLOCK-indicatoren werken daarna weer.
- De WebSimulator hanteert dezelfde niet-herhalende animatie en verwijdert alle CSS-wave-statussen bij het afsluiten.
- Zichtbare versienamen in WebInstaller, WebSimulator, Project Manager en releasebestanden zijn bijgewerkt naar **1.01 RC1.0.3**. De Project Manager blijft protocol V3 gebruiken; zijn UI heet V1.6.
- Project Transfer V3, echte DELETE, `[READ ERROR]`-recovery, reverse key search, KEYLOCK en het `.beetle`-formaat zijn behouden.


---

## RC1.02 — Reverse KEY/SCALE search, boot LED ripple, READ ERROR recovery

- Reverse analyzer checks every saved melodic note and full original chord tones across 48 patterns, presents ranked keys/modes with fit and ambiguity indication.
- Central two musical keys trigger a spreading boot LED ripple over all 41 physical key/button LEDs.
- Project Manager V1.5 and USB Project Transfer V3: explicit CRC-damaged, BEETLE-owned project slot recovery; protected foreign flash stays locked. Normal import/export and physical DELETE preserved.
- Project schema, protected bootloader and user file encryption unchanged. Experimental: hardware not independently validated.

## BEETLE RC1.01 · KEYLOCK note and linked-chord scroll correction

- LOCK scroll steps through in-key pitches only; OFF/GUIDE stay chromatic.
- Chord voices attached to a root are fitted to the selected scale under LOCK.
- Simulator behavior matches native code. Project Manager V1.4, true DELETE and format unchanged.
- Experimental firmware: no independent physical FM1 validation.

## BEETLE 1.01 RC1.01 · Project Transfer V2 TRUE DELETE

- New Project Manager V1.4: physically delete one occupied slot to `[ EMPTY ]` via USB MIDI V2; optional `.beetle` backup and double confirmation.
- New opcode 0x59: full A/B verification, slot-isolated flash erase, full erased-state verification, refuse foreign data and currently loaded project.
- V1 firmware continues to support export/import; DELETE is disabled unless V2 responds.
- Updated embedded WebInstaller firmware (matches standalone `.fwsc` byte for byte).
- No changes to simulator sound engine / project codec / unrelated FM1 functionality.
- Hardware validation of physical flash erase is pending. Preserve verified backups and rescue procedure.

## Project Manager V1.1 / RC1.01 Simulator update (web-only)

- No-password AES-256-GCM binary `.beetle` files, integrity tag and fresh nonce per export. Shared compatibility key is public and not a secrecy mechanism.
- Project Manager V1.1 exports native project payloads; reads V1 password-protected and V2 password-free files.
- RC1.01 WebSimulator reads native TRK1 V12–V17 `.beetle` files and legacy JSON backups. It exports FM1-compatible native V17 `.beetle` files via the real C wire layout (CRC32 and bounded format).
- Regression tests: genuine native V17 fixture, JS-to-C decoder compatibility, encryption integrity, browser markup/JS and simulated MIDI slot discovery.
- No firmware or `.fwsc` bytes changed. Hardware use remains experimental and unvalidated.

## BEETLE 1.01 RC1 — Independent drum columns, audio STOP, and simulator menu spacing

- **CH6 NOTE1 + ENV:** inserts `[STP]` into NOTE1 without clearing an existing NOTE2 drum hit or NOTE2 `[STP]`.
- **CH6 NOTE2 + ENV:** inserts `[STP]` into NOTE2 without changing NOTE1 (regression retained).
- **Audio playback:** a drum NOTE1 STOP releases only the last NOTE1 voice; NOTE2 STOP releases only the last NOTE2 voice. In the WebSimulator and on the FM1 they no longer stop each other.
- **Synth lanes:** unchanged ENV STOP behavior, including chord reset.
- **WebSimulator PROJECTS:** selected-row background corrected to fit inside its own 20-pixel row; SAVE selection no longer overlaps SAVE AS.
- **Other simulator menus:** standardized list selection backgrounds and SONG row frames to remain strictly within their rows; no user-facing item removed.
- **Credits:** keep author attribution, replace the overly emphatic disclaimer with a neutral technical reference; source-level provenance review in `CODE-PROVENANCE-AUDIT.md`.
- Project storage format **V17** and all existing features retained. The new native build requires physical FM1 qualification before final release.

## BEETLE 1.0 RC1 — Live Mixer isolation, independent quantizers, NOTE2 STOP

- Live Mixer remains SONG-only. Turning SONG OFF now removes all transient live mute/solo overrides and pending queues without writing to or changing the ordinary saved Mixer; playback also uses the temporary overlay for both new note triggering and the audio rendering paths.
- Native screen now explicitly displays `[X] S` or `[Q S]` for solo; mapped white-key LEDs track live SOLO. Black-key LEDs track live MUTE.
- OCT− toggles quantized MUTE; OCT+ toggles quantized SOLO while on Live Mixer. Each defaults ON in Live Mixer, is independent, commits only at a pattern boundary during playback, and becomes immediate when disabled. Queued second taps cancel.
- Native and WebSimulator Punch-In K1 REPEAT/RING MOD/FILTER/TAPE STOP increments changed from 3 to 1. Stutter remains stepped BPM fractions.
- CH6 NOTE2+ENV now encodes chord=15 STOP, preserves NOTE1 and does not insert a STOP in the first drum cell. Matching WebSimulator ENV behavior included.
- Fixed WebSimulator playback mixer resets: live performance overrides now survive individual step scheduling without being serialized to the project. Also corrected the 120% simulated mixer gain limit and no-audio-context Punch parameter adjustment.
- Updated on-device HELP, browser HELP and bilingual README. V17 project data stays unchanged. JieLi hardware-test, pilot, and storage/UAC build compiled; the protected first 16 KiB and OTA loader are unchanged.
- **RC qualification:** firmware image retains experimental project-write/storage risks and requires the project maintainer's physical FM1 tests, including project backup/recovery, before final 1.0 publication. No unperformed hands-on test is claimed.

## V0.88 — Song Mode Live Mixer gate & CH6 NOTE2 entry

- Live Mixer is available only with Song Mode enabled. A second ARP press while SONG is OFF leaves the Song editor visible; when SONG is ON it opens Live Mixer. If SONG gets disabled while Live Mixer is active, the firmware returns to the Song editor and refuses Live Mixer key actions.
- In the Tracker, the physical keyboard on CH6 now writes NOTE2 when that column is focused. NOTE2 uses the existing packed chord/second-hit representation (0 empty, 1–14 drums, 15 STP). A note entry never changes NOTE1, instrument, effect, velocity or the selected step, and previews the inserted drum hit. NOTE1 input from the other columns still advances to the next step.
- WebSimulator implements matching NOTE2 entry and Song gating, including note preview and project-backup persistence. No project-format migration (V17 retained), new flash partitions or bootloader edits.
- Three JieLi build variants, C NOTE2 regression, native route tests, existing native regressions and Chromium browser note-entry/screen tests passed. Specific new behavior awaits the project maintainer's usual physical FM1 testing before publication.

## V0.87 — Punch-In ergonomics, Live SOLO, waveform presets

- K1 controls held Punch-In FX; ALGORITHM returns to pattern selection. Ring Mod defaults to 20 and Stutter to BPM-relative ×1.
- Mute on six black keys and SOLO on six corresponding white keys in both MIXER and Live Mixer, including active FM1 LED feedback.
- MASTER FX uses SELECT for row selection and K1–K4 for EQ, compressor and reverb editing; SEL toggles FX power.
- Project schema V17 persists ADSR, cutoff and oscillators independently for each waveform TYPE per patch slot. V16 and previous supported projects can still be loaded. Project saves are capacity-checked before flash writes; a sufficiently dense project can exceed the fixed slot capacity.
- HELP end indicator corrected. Additional invalid-type wrapping check added. Physical FM1 verification remains part of the maintainer's release process.

## V0.86 — Live Mixer, New Mix Defaults, 120% Faders

- Synth channels CH1–CH5 start at mixer **50%**, CH6 digital drums remain **80%**; applies to new projects only.
- Quantized mutes default **OFF** in the normal Mixer for new projects. Existing stored preferences remain intact.
- **Live Mixer** opens with a second **ARP** press from SONG MODE; pressing ARP again returns. The first six white keys mute/unmute CH1–CH6. During playback they are always queued to the next pattern boundary, independently of the saved quantized mute setting; stopped transport applies immediately.
- Conventional mixer K3 volume range extended to **0–120%**. Both save preflight and V16 decoder accept 120% (reject 121%). Fixed-point velocity scaling remains bounded at MIDI 127; at high levels and dense arrangements clipping is still possible and must be checked on hardware.
- On-device HELP, README and WebSimulator updated. No project schema or reserved flash partition changes. The project maintainer physically tests beta releases before public publication.

## V0.85 — Tracker Keyboard Input, Step Octave & LED Feedback

- **Tracker note entry:** pressing any musical key now writes to `NOTE` regardless of the edit cursor being on `NOTE`, `INS`, `CHD` or `FX`. The other cell values remain untouched, and step advancement still respects the active pattern length. On CH6 keyboard entry targets drum NOTE1, never NOTE2.
- **SEQ + OCT− / OCT+:** on an existing synth NOTE, shift only that selected step's saved pitch by −12 or +12 semitones (MIDI 0–127 bounds) and immediately preview the transposed sound. Undo/Redo records the modification. Empty `[---]`, stop `[STP]` and CH6 drums are unchanged. This shortcut never changes the global performance-keyboard octave.
- **Keyboard OCT LED:** on the physical FM1, OCT− stays illuminated when the keyboard octave is negative and OCT+ stays illuminated when it is positive; neither stays lit at octave zero. Pressed button illumination remains intact. The WebSimulator mirrors this indication.
- **Documentation:** release history appears in `CHANGELOG.md` only; the README stays a current user manual with the two GitHub Pages install/simulator badges and complete third-party credits and licence links.
- **Compatibility:** project V16, protected loader, six lanes, firmware storage layout and existing instruments unchanged. New actions checked in native C tests and browser simulator; device-owner hardware tests remain the public beta release gate.

## BEETLE V0.84 — MASTER Controls, Ring Mod & Project UX

- K1 now selects MASTER FX; SEL toggles the selected effect with `[X]` / `[ ]` indicators. SELECT rotary edits knob/parameter 1; K2–K4 control the remaining parameters. Obscuring EQ help text removed.
- Reverse Punch-In replaced by true stereo Ring Mod, adjustable and persistent default 20. No extra audio delay memory required.
- BPM-linked Stutter now synchronizes for isolated CH6 drum transients even with no synth voices sounding.
- Stronger step Vibrato modulation; corrected sinusoid amplitude normalization.
- Startup LOAD browser HOME now returns to the startup two-option chooser.
- WebInstaller simplified, with integrity checks, mandatory confirmation and advanced options preserved.
- Firmware identifier `FM-1_2084`. Existing project format V16 preserved. Older Reverse events will produce Ring Mod in this version.
- Three native build variants compile; host regression, browser, and firmware package tests passed. Device-owner hardware confirmation required for these specific changes prior to public release.

## V0.83 — OCT Punch-In & Copy-on-Write Instrument Focus

- OCT UP and OCT DOWN now retune every sounding native synth voice, including dual oscillators and NES NOISE, rather than only newly scheduled sequencer notes. Release restores the original pitch without changing recorded notes.
- Instrument editing automatically clones any already-used patch to a free slot before changing its type/parameters, leaving existing tracker/song instrument IDs untouched. New keyboard-entered notes use the last focused instrument.
- If all four available V16 synth patches are occupied/reserved, EDIT refuses the operation with `4 PATCH SLOTS IN USE`. It never destroys existing song timbres to make room.
- Native host tests cover all twelve synth types; interactive Chromium confirms old/new waveform isolation and full-slot rejection.
- All three JieLi build variants compile; OTA Freedom pinned ETCH package simulation passes. USB-audio support, original OTA loader, V16 schema, project compatibility, credits and licensing maintained. No physical FM1 test has been performed by the assistant; maintainer validates before public beta release.

## V0.82 — BPM-Synced Stutter, Reverse Speed, DMG Timbre, SEL Input Interlock

- Seven beat-related STUTTER gate rates, controlled by ALGORITHM; saved within the existing five Punch-In parameters.
- Frozen 128 ms reverse snapshot now reads at 1× by default; ALGORITHM changes 0.5×–2×, and resets without saving after key release.
- Stronger DMG RETRO PULSE color with AC-coupled/filtered output. NES TRIANGLE closer in rendered level to NES PULSE.
- Additional SEL release/re-arm latch after both successful and failed project LOAD/SAVE. Step clearing is permitted only for a new Tracker-local SEL press.
- Three JieLi build variants, V0.82 full regression, previous audio and flash-fault-injection tests passed in software. Protected loader, V16 project codec and credits unchanged.

## V0.81 — Boot chooser, first-free project slots, EDIT preview, SAVE return, USB Audio Class

- Power-on only: choose NEW PROJECT or LOAD PROJECT. Normal PROJECTS menu remains unchanged.
- NEW PROJECT finds the lowest empty of 12 project slots without replacing occupied/invalid data; a full volume cannot be silently overwritten.
- EDIT on CH6 auditions the current synth patch instead of drums. DRUM EDIT retains the component audition keyboard.
- PRESET note-cell cycle uses only the original note (if any), EMPTY and STP.
- Song Mode footer now reads `SEL "TOGGLE ON/OFF"`. Successful saves return to the invoking screen.
- Optional compiled USB Audio Class capture path enabled for the storage build; 44.1 kHz stereo final mix, separate RAM region, real FM1 USB enumeration and recording not yet qualified for this change.
- Three JieLi target builds and targeted software/OTA/Chromium tests passed. No changes to project schema V16, existing synths, OTA loader or licences.

## V0.80 — Game Boy DMG-style RETRO PULSE and Drum Component Preview

- Replaced RETRO PULSE's generic pulse comparator with DMG-inspired eight-position duty patterns, 11-bit divider note table, 4-bit stepped amplitude and a bounded high-pass output stage. The NES PULSE engine remains separate and unchanged.
- Drum Engine Editor now interprets key-on edges from all 27 FM1 keys as read-only audition triggers for the selected component, including when REC is armed; no step is entered or cleared.
- WebSimulator adds Drum Engine Editor preview and editing controls; its sampled-drum WebAudio timbre approximation is not identical to the FM1 DSP.
- All instrument indices and V16 project compatibility retained. Firmware product: `FM-1_2080`.
- Native JieLi builds, regression tests, OTA integrity and browser checks performed; user conducts the normal pre-release physical FM1 validation.

## V0.79 — Project I/O SEL safety / Tracker-only delete

- Fix: physical SEL can only clear a step if the input polling cycle **starts and ends** on the Tracker page, and no project-I/O input guard is active.
- Project SAVE now fences queued SEL presses during WRITING and VERIFYING, including direct return to the Tracker, overwrite/provisioning prompts, and error paths.
- Project LOAD now fences queued SEL for successful and failed reads, including READING and VERIFYING.
- A released SEL must remain quiet for 320 ms before Tracker deletion can be re-enabled. Real new presses work normally afterwards.
- WebSimulator project save/load/new transitions follow the same brief SEL fence.
- New native C guard regression test and project transition static assertions; all previous features, NES instruments and credits preserved.
- No project schema, firmware flash layout or protected OTA loader changes. Physical-device behavior of this specific fix must be checked before public beta publication.

## V0.78 documentation / on-device credits correction

- The FM1 PROJECTS → CREDITS screen and WebSimulator now explicitly label `Nes_Snd_Emu` as `NES TECHNICAL REF ONLY` and `NO NES_SND_EMU CODE USED`.
- This credits clarification does not change the NES oscillator DSP or project format. The patched native firmware image has been rebuilt because the on-device display strings live in firmware code.

## V0.78 — NES pulse / triangle / noise and on-device CREDITS

- Three new instrument types (IDs 9–11) added to the existing nine, preserving old IDs and project schema V16.
- Lightweight 8-step pulse, 32-step 4-bit triangle and 15-bit NES-style noise with NTSC period selection. No DMC, expansion chips, or claim of cycle-accurate emulation.
- Native hardware UI PROJECTS: CREDITS under HELP and above DIAGNOSTICS, read-only and safely exiting with HOME/SEQ.
- WebSimulator adds corresponding three oscillator voices and CREDITS page.
- Attribution for SikkenWorks (Joppe Sikken), Felucca (Leo Kuroshita/Hügelton Instruments), reSID (Dag Lem), Nes_Snd_Emu (Shay Green, James Athey). Appropriate upstream licence copies and links included. Original Nes_Snd_Emu C++ code is not linked.
- Maintainer's FM1 hardware tests acknowledged accurately; new features require normal pre-release physical validation.

# BEETLE Changelog

## V0.77 — Firmware Freedom (installer opt-in, RC qualification)

- Separate disabled-by-default **Firmware Freedom** advanced mode with a second consent checkbox.
- SHA-256 pinned stock V15, ETCH V1.36 and ETCH V1.37 firmware images; rejects mismatches, unknown devices and loader-only initiation.
- Felucca and other custom firmware slots remain blocked until individually audited `.fwsc` images are available. No generic OTA safety bypass.
- Source `.fwsc` and WebInstaller regenerated for `FM-1_2077`; unchanged flash layout, bootloader, native DSP and project format V16.
- GitHub README includes exact user-supplied INSTALL and SIMULATOR badges/URLs.
- BEETLE's real-hardware beta testing by its maintainer is acknowledged. New cross-firmware transfer routes still need distinct hardware sign-off.

# BEETLE V0.76 — Offline dump inspection and guarded project salvage (pre-RC)

**2026-10-15.** Full V0.75 source retained. No changes to project codec V16, firmware flash layout, DSP, normal strict A/B LOAD/SAVE policy or protected OTA bootloader.

- `source/tools/beetle_v071_backup_audit.py` can scan an independent exact 1MiB dump (`scan-dump`) and classify all twelve A/B project pairs with ownership-marker warnings.
- `salvage-dump` can write a **new local CRC-checked TRK1 extract**, with an explicit acknowledgement option, from a valid bank even when the companion bank is foreign. Never writes to FM1; refusing overwrites and ambiguous equal generation split-brain states; output does not imply native semantic decode or device restore.
- New regression suite `test_v076_dump_salvage.py`; all earlier tests preserved. Three native builds and simulated OTA passed. New identity `FM-1_2076`, V0.75 predecessor allowlisted. Protected loader preserved byte-for-byte.
- **RC1 remains BLOCKED** by independent rescue, physical flash ownership, real hardware audio, MIDI and power-loss testing. Experimental storage can erase prior firmware user data.

---

# BEETLE V0.75 — Final RC Hardening (pre-RC)

**2026-10-15.** Based on complete V0.74. No features removed; V16 project
serialization and the 12-slot physical flash layout remain unchanged.

1. `track_journal.c`: verify the nested 24-byte TRK1 project header, version,
   slot, length, header CRC and payload CRC while scanning the journal bank.
   Prevent a CRC-consistent but internally invalid newer record from being
   selected as the committed source during the next save.
2. `test_v075_atomic_recovery.c`: 219 injected read/erase/program failures,
   including partial NOR operations on a dense multichannel project.
   Assert latest known-good bank never changes. Under torn foreign headers,
   saving and potentially loading remain fail-closed, not automatically
   recovered.
3. `test_v075_audio_endurance.c`: render 60 seconds at 44.1 kHz with active
   SID-flavored voices, drum triggers, MASTER and Punch-In. Host DSP integrity
   checks pass; **no hardware cycle measurements** implied.
4. Rebuild all 3 target variants; package an audited experimental firmware
   `FM-1_2075`. Keep reference loader and first 16KiB identical. Installer
   predecessor allowlist now includes the released V0.73 and V0.74 IDs.
5. Existing V0.74/V0.73 regression suites and simulated OTA checks pass.

**Remaining blockers:** validated independent ROM/recovery path, verified full
flash backup and restore, real power-loss behavior, FM1 timing/30+ minute
hardware audio test, MIDI clock and physical flash ownership. **NOT RC1.**

---

# BEETLE V0.74 — Stability & Recovery (pre-RC)

**2026-10-15.** Complete V0.73 source retained. No musical features removed;
project schema **V16**, 12 slots, 48 patterns and protected loader unchanged.

- **Fail-closed native project preflight:** reject BPM outside 30–300, invalid
  scale/key options, song enabled/loop flags, song chain entries/repetitions,
  pattern lengths, lane mixer parameters, drum controls, and invalid MASTER FX
  settings/order. These values previously could be serialized with a valid CRC
  but fail validation on subsequent load. Preflight runs before the journal
  performs even its first flash read/erase/program in a SAVE.
- **Restore input safety:** V16 project decoder also rejects malformed song
  loop/enable flags. No changes to the encoded binary layout or CRC scheme.
- **New tests:** 23 invalid project settings, zero I/O on failed saves, unchanged
  previous journal bank, CRC rollback, valid V16 save/load, all prior V0.73
  regressions and rebuilt FM1 target variants.
- **Firmware:** `FM-1_2074`; original pinned ETCH V1.37 16KiB header and
  OTA loader are byte-identical; source-level package/integrity tests pass.

**Safety:** this is not hardware RC1. Physical flash ownership, erase behaviour,
boot reliability, rescue capability, MIDI and audio margins are unverified.
The experimental storage variant may overwrite older ETCH/Felucca data if
user deliberately provisions it. Do not flash a valuable device without an
independently tested recovery route and verified full-flash backup.

---

# BEETLE V0.73 — Hardware Diagnostics & Performance (pre-RC)

**2026-10-15. All existing six-lane tracker features and V16 project compatibility preserved.**

## Native FM1, read-only diagnostics

- **HOME → DIAGNOSTICS**, then turn **K1** to switch AUDIO/MIDI views; HOME returns to the project menu, SEQ returns to the tracker. This view does not edit any project parameters or write to flash.
- AUDIO: max DSP / wall / nested ISR time in **tenths of a percent** of the **128-frame / 44.1 kHz DMA deadline**; processed DMA half-blocks; wall time above 85% or 95%; late DMA ownership; pre-publish guard refusals; digital clip frames; voice-budget shedding.
- MIDI INPUT: count inbound USB/TRS MIDI packets, received 0xF8 clock messages and overflow resets. Shows coarse arrival intervals in milliseconds (min, max, latest); a gap over 500ms clears the old interval baseline. **This does not prove clock-sync accuracy, MIDI latency, nor physical wiring.**
- Every new counter is bounded and updated without allocating, logging, formatting or dividing in the ISR. Reads take a short IRQ-disabled snapshot for display.
- Relocated the always-initialized `track_state` into the existing guarded RAM pool to pass the tighter 96-KiB JieLi pilot build without increasing memory boundaries or touching HAL/loader layout.
- **No changes to the V16 project serializer, 12-slot A/B flash mapping, protected V1.37 loader or destructive storage confirmation policy.**

## Software validation

- Three native pi32v2 builds compile, including read-only diagnostic, OTA pilot (no project-storage writes), and *experimental* storage + OTA variant.
- Host unit tests cover wrapped TIMER4 measurements, nested ISR clamping, wall utilization, DMA guard counts, saturating counters, wrapped MIDI millisecond timestamps, an actual 0ms-resolution interval and >500ms clock pause.
- Original V0.72 RC audio tests, earlier edit, project journaling, backup and recovery tests pass. Simulated OTA validates V0.73 against pinned ETCH V1.37 loader and stock/predecessor identities, rejecting corrupt packages.
- WebSimulator stays browser-only (WebAudio does not show fictional hardware load). No real device testing was performed.

**Release gate: NOT RC1.** Flash partition ownership, full-flash backup and independent rescue, real audio-DMA margin, USB MIDI timing, analogue performance, OTA update, stock restoration and power-interruption recovery all remain unverified on the physical FM1. The `.fwsc` storage build is **not recommended for installation on a valuable device**.

---

# BEETLE Changelog

## V0.72 — RC Hardening: MASTER FX Guardrails & Native Audio Stress (15 October 2026)

- **MASTER FX state recovery:** validate the eight fixed-point EQ filter states and two reverb low-pass states before arithmetic. Bad RAM values now trigger the pre-existing bounded dry recovery path and reset all crossover accumulators; this prevents signed-overflow cascades from a damaged DSP state without touching user instrument/project data.
- **Conservative parameter clamping:** even an unexpected 101–255 value reaching audio processing is capped at 100, once per processing block, for EQ gains/frequencies/widths, compressor threshold/ratio/makeup/attack, and reverb wet/feedback/tone/spread. Valid project values 0–100 preserve the original DSP parameter calculations.
- **Real native DSP host stress:** added UBSan run exercising eight synthvoices, up to six drum voices, compressor/EQ/reverb and Punch-In, plus a separate chunk-parity check (128 frames versus 4×32 frames), damage recovery, extreme parameter validation, and 271 BPM settings (30–300). No host sanitizer failures in this test. A short synthetic host render **is not** a real-time CPU benchmark or an extended FM1 soak test.
- **Compatibility:** unchanged V16 project codec, 12-slot A/B bank map and save format, existing controls and all previous BEETLE features. Three separately compiled JieLi images: read-only diagnostic, OTA pilot with project writes disabled, and experimental storage-capable variant. Firmware identity `FM-1_2072`; previously supported `FM-1_2071` remains on the installer source allowlist.
- **OTA/package checks:** new V0.72 `.fwsc` checksum/embedded installer pairing validated; the protected ETCH V1.37 loader and first 16 KiB are byte-for-byte unchanged. Source-level simulated update/identity/corruption tests pass. Physical FM1 installation, flash wear/power loss, fallback boot/USB recovery, sustained audio timing and analogue output **have not been validated**.
- **RC decision:** this is **V0.72 experimental / pre-RC**, not RC1. The attached `RC-HARDWARE-CHECKLIST.md` defines mandatory evidence before promoting any firmware to RC1.

## V0.71 — Backup Integrity & A/B Recovery Qualification (15 October 2026)

- **WebSimulator verified backups:** EXPORT PROJECT becomes VERIFIED BACKUP with a self-contained `.beetle-backup.json` envelope, SHA-256 over the complete JSON project, a byte-length bound and explicit format/version metadata. RESTORE BACKUP checks integrity and project structure **before** replacing browser working memory; damaged, unknown or oversized backups are refused. Older plain browser JSON project files can still be imported, explicitly marked unverified. This is **browser-side only**, not a native FM1 backup transport or flash restore.
- **Offline TRKJ read-only audit:** new `source/tools/beetle_v071_backup_audit.py` validates user-supplied individual A/B bank images, ownership markers, bank CRC32, project header CRC32, generation, and slot identifiers. It can extract the newest valid TRK1 project record to a new `.trk1` file without overwrite. It **cannot acquire** these bank images from the FM1 and **never writes** to a device; verifying structural CRC is not a substitute for semantic firmware decoding or hardware recovery.
- **Journal recovery regressions:** twelve project slots checked with current V16 fixtures; no-op saves cause no erase; interrupted second-bank writes preserve the committed first bank; successful next commit and corrupt-bank CRC fallback tested; foreign-bank protection refuses further writes. Host simulation only.
- **Compatibility:** V16 project format, six-lane engine, instruments, Mute Groups, Snapshots, performance modes, MASTER FX and V0.70 Extended Undo/Redo retained unchanged. Native three build variants use the V0.71 identity. Updater/experimental storage package continues to require proven hardware recovery and explicit destructive confirmation on first use.
- **Limits:** there is no verified physical FM1 project extraction path, no independent recovery proof, and no certified on-device storage safety. A cryptographically verified browser JSON backup cannot be directly flashed into the native firmware.

## V0.70 — Unified Extended Undo/Redo (15 October 2026)

- **One chronological six-action Undo/Redo history** across tracker patterns (including Punch-In events), synth instrument parameters, mixer parameters, Song Mode arrangement, MASTER FX, and instrument library STORE/LOAD/COPY/RENAME.
- **SEQ + OP5** Undo and **SEQ + OP6** Redo now work from TRACKER, EDIT, MIXER, SONG, MASTER, and LIBRARY screens while playback is stopped; in the WebSimulator use the existing Undo/Redo buttons or Ctrl+Z / Ctrl+Y.
- Native firmware reuses the existing six pattern-history snapshots for smaller, typed snapshots instead of allocating another six-entry pool. Instrument restoration reapplies patch parameters to the synth DSP; mixer Undo cancels pending mute changes to avoid stale quantized commands.
- **No project file format change:** still V16. The Undo/Redo history remains in RAM and clears when a project is loaded or a new session is started.
- **Scope:** direct mixer/instrument/song/master/library edits while stopped are supported. Live queued mute events, changes made during PLAY, and other unrelated global settings are not automatically undoable.
- WebSimulator updated to match the extended history, with browser-based regression testing of five distinct editing domains and Undo/Redo. Native mixed-domain host tests and the prior V0.56 pattern editing and V0.69 library storage regressions pass.
- Native target builds and simulated OTA tests remain **software-only**; actual FM1 flash safety and realtime CPU load are not physically certified.


## V0.69 — Instrument Library (project-local patches)

- Added an **eight-slot instrument library** for complete synth patches. Each slot holds the oscillator/SID type, VA oscillator parameters, ADSR, duty, cutoff and level, plus an **eight-character name**. The four actively editable instruments and six-lane tracker remain unchanged.
- From a synth-lane **EDIT**, hold **SEQ + EDIT** to open the new **INSTRUMENT LIBRARY** screen. **K1** selects slot 1–8; **K2** selects STORE / LOAD / COPY / RENAME; **K3** selects target instrument 1–4; **K4** chooses destination slot in COPY; **SEL** performs the operation. RENAME opens the native character editor (K1 position, K2 character, SEL confirm). HOME returns to EDIT, SEQ to TRACKER.
- STORE copies the selected active patch to a library slot; LOAD replaces the chosen active patch; COPY duplicates the selected library slot into another slot; RENAME changes the eight-character label. All operations work only while transport is stopped, to avoid changing the patch during audio playback. UI actions are non-destructive to sequencer notes.
- **Project format V16:** the library's validity mask, occupied names and occupied complete patches are serialized after the V15 Mute Groups data, covered by the existing CRC32; read-only preview and fail-closed A/B journal remain intact. Old V15 records open with eight empty library slots. A library is **stored inside its project**, not in shared flash. To move patches to another project use the WebSimulator's complete project JSON export/import; on-device cross-project patch import remains unimplemented pending a verified safe flash design.
- Updated FM1 HELP and WebSimulator (including a dedicated browser Library button for testing). Original six lanes, V0.58 moon boot animation, DSP, Mute Groups and Snapshots are unchanged.
- **Validation:** all three JieLi build variants compile; V16 roundtrip and V15 migration, copy/store/load/name and CRC host tests pass; prior V0.63–V0.68 groove/mute regression paths pass; Chromium UI and simulated OTA checks are software-only. Hardware validation is still absent; the storage firmware remains experimental.

## V0.68 — Mute Groups (Live Performance)

- Added **four independent six-lane Mute Groups** alongside V0.67 Performance Snapshots.
- Open **GLO → SEQ+SEL (PERFORMANCE) → FX (MUTE GROUPS)**. K1 selects group 1–4; K2 selects lane 1–6; SEL adds/removes the lane; K3 mutes/unmutes all members together; HOME returns to PERFORMANCE. GLO returns to mixer; SEQ returns to tracker.
- A group only changes member lanes. Its toggle computes the target from effective queued/live mute states: if all members are muted, unmute them; otherwise mute them. Empty groups are safely ignored.
- Respects **Quantized Mutes ON/OFF** during PLAY and keeps pending changes for lanes outside the group. Shares the existing pattern-boundary mute queue and works alongside full Performance Snapshots.
- Project schema **V15** saves four compact group membership masks (4 bytes total). Supported older V13/V14 projects migrate with four empty groups. Existing snapshots remain intact. Browser project export/import also carries group membership.
- Added GROUPS display, HELP instructions, WebSimulator controls and tests. The audio voice engine, V0.58 moon/Beetle boot animation and protected OTA loader are unchanged.
- **Validation:** three JieLi native builds, bounded host tests (including V14/V13 migrations, group queue and snapshot interactions), simulator browser interaction, and software-only OTA integrity simulation. Physical FM1 behaviour remains unverified; storage and firmware updates are experimental and can cause data loss or an unbootable device.

## V0.67 — Performance Snapshots: Four Live Mute Scenes (2026-10-15)

- **Four performance snapshots:** the MIXER now opens a dedicated PERFORMANCE screen with **SEQ + SEL**. Each slot stores an independent six-channel mute combination. **K1** selects a slot, **K2** stores the currently active mute mask, **K3 or SEL** recalls it, **HOME/GLO** returns to MIXER, and **SEQ** returns to TRACKER.
- **Visual display:** all four snapshot rows fit the FM1's native 240×240 screen, with six blocks per saved snapshot. **Pink means muted; white means unmuted.** Empty slots are identified as `EMPTY` and cannot accidentally overwrite existing channel states when recalled.
- **Live quantization:** while PLAY is running and QUANT MUTES is ON, recalling a snapshot replaces any existing queued mutes with a complete target arrangement, which commits at the next pattern boundary. With QUANT MUTES OFF (or transport stopped), all six mute values change immediately. A recall does not alter volume, solo, pan, swing, audio effect settings or recorded note data.
- **Project persistence:** new fixed-size **V14 project record**, appending a validity bitmap and four six-bit mute masks (five bytes). V13 and older supported projects open with four empty snapshots; CRC and out-of-range values are rejected. No changes to the five-byte sequencer step encoding or audio sample processing.
- **WebSimulator:** matching PERFORMANCE screen, shortcut, store/recall flow and project JSON migration/export. The browser's quantization is timing-approximate; native firmware uses the existing sample-boundary queue.
- **Tests:** standalone snapshot host tests, V14 save/load and V13 migration, prior groove and mute regression tests, interactive Chromium tests and simulated OTA tests. All three JieLi build variants compile. **Real FM1 behaviour and storage safety remain unverified.**
- **Release cleanup:** generated object files, obsolete V0.66 web tools, build logs and caches excluded. Complete C source, licensed reference files, previous regression tests, moon loading screen and ReSID credits remain included.

**Safety:** the physical storage-enabled firmware is still an experimental build. Do not format or flash without an independent verified backup and recovery route; an app build and simulated OTA test are not proof of safe recovery.

## V0.66 — Mixer Quantized Mutes ON/OFF (2026-10-15)

- **MIXER SEL toggle:** GLO opens MIXER; press **SEL** to switch **QUANTIZED MUTES ON/OFF**, clearly shown at the bottom of the 240×240 screen. K1/K2/K3 mixer editing and the six black-key mutes are preserved.
- **ON (default):** during PLAY, a changed lane mute is queued to the next pattern boundary. `Q1` and `Q0` show pending mute/unmute. During STOP, mute changes apply immediately.
- **OFF:** changing the mixer MUTE field or pressing a mapped black key applies immediately, including during PLAY. Turning the mode OFF while queued changes exist **commits those queued changes immediately**, rather than losing or delaying them. Turning it ON again does not retroactively queue earlier direct changes.
- **Project persistence:** quantize setting is stored per project in the new **V13 project format**, with a single appended byte. Older V12 and earlier projects open with Quantized Mutes **ON** by default; the existing step encoding, audio DSP and drum/synth engines are unchanged. Invalid flag values and broken CRC are refused.
- **WebSimulator:** identical mixer SEL interaction and ON/OFF display. Export/import preserves the setting, including compatibility with previous browser projects that have no flag. Browser audio timing is an approximation of native sample-boundary timing.
- **Tests:** host mute mode and V13 roundtrip / V12 migration tests, existing V0.63–V0.65 regression tests, browser interaction and simulated OTA integrity; read-only, pilot and experimental storage-native builds compile. No physical FM1 validation.
- **Cleanup:** only V0.66 release artifacts in the clean ZIP; the original source, current webtools, licensed upstream assets, moon loading art and historical regression tests are retained.

**Safety:** experimental FM1 update/storage can brick the device or erase user data. Flash only with independent verified backup/recovery. Firmware/software tests do not establish on-device safety.

## V0.65 — Live Performance: Quantized Lane Mutes (2026-10-15)

- **Quantized mutes:** while PLAY is running, changing a MUTE field in MIXER with K3 or using the six hardware black-key lane toggles arms a mute/unmute to be applied exactly at the **next pattern boundary**. Other mixer controls (volume, pan, solo and swing) retain their existing editing behavior.
- **Clear pending state:** `Q1` means a mute is queued and `Q0` means an unmute is queued; `ON`/`--` indicate committed state. Toggling a pending lane back to its original state cancels that lane's pending change. Queues are cleared on STOP, project loading, or NEW. Changes committed at the next boundary are persisted by the existing project SAVE; the transient queue is never serialized.
- **Realtime safety:** the pending state occupies two bytes, uses no dynamic allocation, and commits inside the existing audio step-boundary callback. No change to PCM render, 5-byte step size, or V12 project format. This was checked in host tests and compiled on the JieLi toolchain, **not verified on a physical FM1**.
- **WebSimulator:** MIXER displays `Q1`/`Q0` and schedules queued mutes for the following pattern start. Browser timing approximates hardware timing with WebAudio and a host timer; it is not sample-identical to FM1 firmware.
- **Documentation:** HELP, README, and this CHANGELOG explain the behavior. Prior source files, license notices and ReSID attribution remain preserved; old release installers/binaries are not included in the new ZIP.

**Safety:** all OTA and persistent flash writes remain experimental; a complete flash backup and independent recovery route are still required for device testing.

## V0.64 — Groove Copy/Paste, Lane Fill & Clear (2026-10-15)

- **STEP GROOVE tools:** on STEP GROOVE (SEQ+ENV), hold SEQ and press OP1 to copy the selected step's MICRO/RATCHET/PROB settings, OP2 to paste those settings to the selected step, OP3 to fill every *active step in the current lane* with the current step's groove, or OP4 to reset all active steps in the current lane to defaults. The shortcuts are page-local; the tracker retains its original lane and full-pattern copy/paste shortcuts.
- **Non-destructive:** these actions edit only the existing 7-bit groove value; note, drum NOTE2, instrument, chord, step FX and velocity/gate remain untouched. Fill/Clear respect pattern length (1–16) and do not alter stored events in hidden steps.
- **Edit safety:** Paste/Fill/Clear run only while playback is stopped, under the existing audio IRQ lock and pattern history recording. Each multi-step fill/clear is a single Undo/Redo action. The clipboard contains one byte and is cleared when loading a different project.
- **UI:** the STEP GROOVE page now explains the new combinations without text clipping. HELP documents both the new and existing shortcuts. K1 remains the only scroll encoder in HELP.
- **WebSimulator:** matching actions added as dedicated STEP GROOVE buttons. Simulator project saves retain V12 compatibility; no new project fields or audio format changes.
- **Regression tests:** bounded 1–16 fill, empty clipboard, repeated paste, preserving musical step fields, plus existing V0.63 groove/legacy project migration fixture.
- **Build status:** no performance claim for physical FM1. The JieLi toolchain and OTA package are still experimental and require independent hardware recovery validation.

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
