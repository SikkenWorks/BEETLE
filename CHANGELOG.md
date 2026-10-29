## BEETLE V1.8 — BLUE HUD / HELP and deterministic sample streaming (EXPERIMENTAL)

- Adapted BLUE RC12's Felucca-derived FM1 voltage ADC bands (531/561/591), validated foreground reads, invalid-sample fallback, four-bar pixel HUD and non-charging-claim USB indicator. Battery percentage is NOT measured.
- PAT counter matches BLUE right-aligned footer placement; BEETLE retains its original global pattern length and six-channel synthesizer/drum architecture.
- HELP uses three easy-to-read instruction cards per page, retaining all 13 original BEETLE topics and all original instructions. K1 changes page and K2 changes topic.
- CREDITS now has two read-only pages (K1 navigation) listing SikkenWorks, ETCH, Felucca/Hügelton/Leo Kuroshita, JieLi, reSID/Dag Lem, Nes_Snd_Emu/Shay Green/James Athey, and preserved upstream attributions. Full licensing remains in README and LICENSES.
- In the BD4S sample ISR, merged window-ready and PCM fetch into one RAM-only pass; avoid advancing source time when prefetch data is not ready. Foreground snapshots retrigger serial/pending under short IRQ lock; preserve immutable generation-pinned verified sectors on identical sample retriggers, and defer decoder cleanup to foreground.
- No change to the TRK1 V26 project format, storage partition layout, firmware ID, protected bootloader or the five synth lanes. No new sample cache allocations in RAM; BLUE-specific six-drum and BDR1 project changes intentionally NOT imported.
- Native full and read-only JieLi builds, baseline user-sample PCM, 12-slot V2 storage interruption tests PASS in software. A preexisting legacy audio fingerprint expectation differs from the baseline and is not considered PASS.
- NO physical FM1 audio DMA, volume peak, USB multitrack, OTA or recovery validation. DMA DEADLINE may still occur under heavy hardware workloads.

## V1.7.14 — Warm DMA NEW PROJECT (experimental)

NEW no longer cold-restarts the DAC when the stopped tracker is already safely initialized and USB capture is inactive. Instead the running ALNK0 DMA receives silence during the RAM reset, then restores DSP state with a sample fade-in. PLAY/USB capture/safety-fault cases retain the cold route. Free-slot scan still occurs at first SAVE with existing CRC validation. Hardware latency/audio safety remain unverified; details: [V1.7.14 validation](V1.7.14-VALIDATION.md).

## BEETLE V1.7.13 — Faster new project creation

- NEW PROJECT now resets its RAM session without synchronous Storage V2 index reads. Flash ownership and the complete CRC/domain audit occur only on the first SAVE, inside the existing paused-audio, input-quarantined saving transaction.
- Removes the temporary 46-row CREATING PROJECT LCD transaction. Keeps the same 240×240 tracker layout, DMA pause/restart safety, retained audio DSP and TRK1 V26 schema.
- Automatic first-free allocation occurs on SAVE; K4 explicitly chooses a slot and retains the existing overwrite confirmation. Empty title defaults use the *actual* chosen slot. Full, corrupt and foreign volumes remain fail-closed. Loaded/project-transferred songs clear the pending allocation.
- Diagnostics still report NEW elapsed phases; FLASH SCAN MS and LOADING UI MS are now zero by design, and any physical speedup must be measured. Two native JieLi build variants and host storage/index regressions provided. This release is experimental and not hardware validated.

## V1.7.12 — DISCARDED LOAD must never create a project

- Fixed **DISCARD → LOAD PROJECT** accidentally running the entire NEW PROJECT routine, causing a false CREATING PROJECT overlay, flash scan, RAM reset and DMA restart.
- LOAD picker now opens immediately. Confirmed discard is deferred until a *valid* selected project loads, and does not trigger another prompt.
- HOME from LOAD restores the original unsaved project's SAVE slot; cancelling the load confirmation returns to the picker. Playback safely stops when entering the project browser.
- Actual NEW PROJECT retains its existing loading overlay and safety gate. DSP, global samples, storage schema, MASTER and DMA safety remain unchanged.
- Added C and source-level flow regression tests, both JieLi native builds pass, and all 48 legacy oscillator PCM scenes remain bit-identical to V1.6.10. Physical FM1 operation, peak safety and OTA recovery remain unverified.

## V1.7.11 — Faster NEW PROJECT index audit (experimental)

- NEW PROJECT now reads only the two 4 KiB Storage V2 index copies (8 KiB total), versus the previous 12 KiB including a redundant third read. Each index still requires full CRC validation; the newest valid generation is selected, then the same project/sample sector collision audit and free-slot check run on RAM-resident data.
- Fail-closed behavior remains: corrupt indexes, overlapping project or sample areas, and unreadable flash are never treated as free project slots. Existing project files and global sample banks are untouched. No flash format, ownership permissions, or project schema changes.
- Preserved the immediate 46-row CREATING PROJECT loading indicator, DMA pause/restart order, input quarantine, MASTER fail-safe, V1.6.10 oscillator PCM output and V26 project compatibility.
- Host regression covers full/empty storage, two index generations, partially corrupted banks, CRC errors and overlapping sectors. Both native FM1 JieLi configurations compile. **No on-device millisecond improvement or analog audio peak safety is claimed.**

## V1.7.10 — NEW PROJECT visible loading status

- Adds native 240×240 LCD loading indication **CREATING PROJECT / PLEASE WAIT...** ahead of flash scan, placed after DMA suspension with input quarantine active.
- Uses a 46-line RAM-composited LCD band instead of a new full-frame redraw; shows activity bars, not an invented percentage.
- Preserves V1.7.9 single-ownership/single-index scan and one full destination tracker draw. No project flash writes occur during the indicator.
- Adds `LOADING UI MS` to on-device `RC NEW PROJECT` diagnostics so physical cost is measurable.
- Retains V1.6.10 legacy oscillator PCM, V26 project compatibility, and V1.6.11 MASTER/DMA safety.
- Native JieLi storage/OTA/UAC and read-only builds passed; **physical NEW PROJECT speed and extreme analog peak safety remain unverified.**

## V1.7.9 — NEW PROJECT latency: duplicate LCD + redundant V2 reads (experimental)

- Found a **real V1.7.8 visual regression**: the menu handler already renders the new tracker once, but a new `beetle_new_tracker_paint_pending` flag caused a SECOND 240×240 full-screen LCD repaint on the next poll. Removed the redundant repaint without weakening the 320 ms input quarantine.
- Found a **second avoidable flash cost**: V1.7.8 repeatedly called the full ownership/V2-mode preflight and opened the two superblocks again during NEW PROJECT. Now one flash ownership probe and one marker check precede the V2 two-bank selection, followed by one CRC-validated index fetch that performs both domain collision audit and free-slot lookup. No occupied project body is decoded; corrupt, foreign, unreadable or overlapping references fail closed. SAVE/LOAD remain fully verified.
- **New third on-device diagnostics tab:** HOME → PROJECTS → DIAGNOSTICS → K1 to `RC NEW PROJECT` displays last NEW attempt's DMA pause, flash scan, RAM reset, DMA restart, LCD draw and end-to-end time (milliseconds), plus run count. No timing hooks in the audio ISR.
- Keeps the original V1.6.10 oscillator gain table / DSP, fast legacy LCD compositor, original sequencer, unchanged Storage V2 project format, crash latch and MASTER watchdog.
- Host test: audited V2 index uses 12 KiB total reads and zero packed project reads; corruption and project/sample sector overlap rejected. Other tests: 48/48 native oscillator PCM scenes unchanged; input/MASTER safety and native JieLi storage+USB and read-only builds pass.
- **Hardware performance and earlier extreme output peak remain unverified.** This is an experimental candidate, not a guarantee that V1.7.9 reaches V1.6.10 physical speed. Use a safe attenuated hardware measurement setup and independent recovery before experimental flashing.

## V1.7.8 — Fast NEW PROJECT / No Audio Regression (experimental)

- **Immediate visual response:** NEW PROJECT now requests one immediate TRACKER repaint while the 320 ms button-release quarantine remains enforced. This corrects the UI delay that previously made the tracker look stuck even after project setup had completed. Input quarantine is not weakened: SEL/PLAY/HOME/encoder and note edges cannot fall through to a new empty tracker.
- **Faster Storage V2 free-slot lookup:** Inspect the two project-reference entries for each of the 12 slots in one *CRC-verified superblock* instead of repeatedly opening/decoding whole packed project records. Two-bank occupancy is preserved and neither damaged metadata nor an unreadable index is considered free. Legacy TRKJ projects retain their existing preview behavior. Actual SAVE/LOAD still perform complete integrity validation; no project layout or flash region changes.
- **Audio safety unchanged:** NEW PROJECT still suspends DMA before any flash scan and bulk reset, and calls the existing safe rearm only if there was no prior safety trip. The physical MASTER watchdog, no-auto-restart fault policy, and input gates remain intact. The 12 legacy oscillator gains and original V1.6.10-responsive LCD/transport code are unchanged.
- **Verification:** JieLi full Storage V2/USB-audio and read-only builds; native 12KiB superblock allocation regression with full, empty, one-bank occupied and corrupt fixtures; static reset/draw/guard contract; existing 48/48 oscillator PCM parity and MASTER/input-gate regressions.
- **Not yet physically demonstrated:** actual NEW PROJECT elapsed time, glitch-free UX after flash scanning, absence of DMA faults and the previously observed extreme analog volume event. Do not claim hardware-ready status. Use a hardware-safe recording/measurement setup and independent recovery route before flashing experimental firmware.

## V1.7.7 — Original 12-Oscillator Sound Restore (experimental)

- Compared V1.6.10 and V1.7.6 native source. The native track_chip, track_drum, track_mixer, project decoder and Song Mode source are identical; the 12-entry oscillator gain table differed in TEN entries. V1.7.6 only restored both VA values.
- Restored ALL twelve type gains to the V1.6.10 values: 105,245,256,345,410,403,631,347,525,214,181,244 (Q8). The five VA waveshape gains are unchanged.
- Checked user-supplied PALLET TOWN (TRK1 V25, slot 04, Song ON, BPM 116) via authenticated local AES-GCM decryption and native V25 decoder. Eight stored instrument-patch note renders are PCM-identical to V1.6.10; the private project is NOT included in the distribution.
- New synthetic regression: 48 C-rendered cases spanning all 12 oscillator engines compare bit-for-bit with V1.6.10 PCM fingerprints. This is a host test, not proof of physical analog parity.
- Retains V1.7.5 / V1.7.6 audio fail-safe and DMA crash handling and the fast V1.6.10-based UI / Song Mode. No project format change (TRK1 V26 writes / V25 readable).
- The old equal-loudness target is NOT used as an acceptance gate for legacy sound parity, because it would force destructive sound changes to existing music. The legacy 12-oscillator loudness-balance test may fail by design.
- IMPORTANT: the physically observed extreme volume fault has not been proven eliminated. The restored high gains can produce higher peaks than V1.7.6. Do not connect headphones or an amplifier for experimentation without an appropriately limited measurement setup and an independently verified recovery method.

## V1.7.6 — Restore legacy VA sound (experimental)

- Restored only **VA OSC** and **VA OSC2** output gain to the exact V1.6.10 Q8 values, **256** and **345** (V1.7.5 used 204 and 276). Existing VA patches no longer lose ~20% amplitude from the V1.7 table.
- The actual original VA oscillator DSP, waveshape gains, envelopes, patch storage, UI, Song Mode, and project schema remain unchanged. The other 10 synth-type gains retain the V1.7.5 values.
- **NEW PCM regression:** 40 VA scenes across five shapes, four patch/velocity/note combinations and both VA types match the original V1.6.10 PCM hashes bit for bit. Fixture generated from the supplied V1.6.10 source.
- Existing MASTER ADC stale-mute / fail-closed DMA protection and V1.6.10 fast-UX recovery remain included.
- The **legacy all-12-type RMS-balance** regression is now **not satisfied** (power ratio 1.583 against threshold 1.30), because preserving the original VA tone/level takes priority. Do not relabel this test as passing. This is not the extreme peak/hardware safety test.
- Both JieLi configurations build; no physical FM1 analog/speaker safety verification has been performed. OTA / storage remain experimental. BACK UP PROJECTS; do not connect headphones or powered monitors for testing.

## V1.7.5 — V1.6.10 Responsive UX Baseline + Audio Safety (experimental)

- Restores the original V1.6.10 audio/transport/song/LCD code paths. Later V1.7.x timing probes, Song Mode precomposition and slow deferred 16-row LCD tiles are intentionally not included in this recovery branch.
- Retains the V1.6.11 physical MASTER watchdog: invalid/stale ADC produces digital mute, upward gain slew limited, first confirmed DMA deadline failure latches audio off until deliberate HOME+PLAY rearm. Original hardware peak root cause still unproven.
- Calls the existing foreground MASTER sampling routine between the two original 120-row LCD renders. The full-screen compositor is otherwise unchanged; no ADC read is added to the realtime IRQ.
- NEW PROJECT stops DMA before flash scans and memory resets, avoids bulk interrupt-off project initialization and rearms only if no safety fault was latched.
- The V1.6.10 physical control workflow, fixed DELETE modal, sample/USB V26 project compatibility and all instrument algorithms are preserved. The gain lookup table uses the previously tested V1.7 oscillator loudness calibration. No new device features are claimed.
- Native full-feature and read-only builds passed, along with critical audio safety, song/clipboard/drum and static UX regression tests. The prior native oscillator loudness balance test passes after applying only the small gain-table calibration (measured RMS max/min 1.012); algorithms remain unchanged. Real FM1 audio performance, dangerous analog peak and OTA recovery remain unverified.

## V1.6.11 — Critical output safety and drum-copy / Song Mode isolation (experimental)

- **Severe audio incident response:** Reported massive uncontrolled volume and crackling while copying CH6 during a Song Chain. The physical failure has not been reproduced on a real FM1; root-cause investigation found multiple **confirmed firmware hazards** (below) that can produce unsafe behavior under load. Do not interpret a passing host test as hardware clearance.
- **Realtime deadline fail-safe:** BEETLE previously tried to shed unrelated ETCH synth voices on a late DMA block (ineffective for tracker audio) and retried/restarted after additional faults. A **single verified DMA ownership/deadline failure now immediately stops playback and disables the analogue output**, and automatic firmware audio restart is disabled. Recovery is deliberate HOME+PLAY, never unattended repeated restart.
- **Independent final MASTER failsafe:** The physical potentiometer was polled in the same foreground loop as expensive flash/sample prefetch. If that loop stalled, the DMA interrupt used the previous, possibly much higher gain indefinitely. New final output gate rejects ADC readings outside 0–1023, mutes immediately on an invalid read, clamps stale (>80 ms) gain to digital zero in the ISR, and limits upward ADC gain changes to 2048 Q15 counts per 20 ms poll while applying reductions immediately. No flash I/O was added to the ISR.
- **Copy/edit concurrency:** Disallow SEQ+OP1 lane/range copy and SEQ+OP3 whole-pattern copy while PLAY/Song Mode is running, preventing lengthy interrupt-off clipboard snapshots from competing with audio DMA and the active song pattern. The existing playback-safe paste guard stays in place. Informational `STOP PLAYBACK TO COPY` overlay replaces destructive realtime changes.
- **NOTE3 clipboard isolation:** A synth lane copy/paste must not silently overwrite CH6 NOTE3 drum events. NOTE3 data are copied or pasted exclusively when the lane is CH6; non-drum copy initializes clipboard NOTE3 to EMPTY.
- **Monitoring caveat:** USB six-stem mode intentionally sends pre-MASTER audio to the DAW and therefore does not follow the FM1 physical MASTER setting; distinguish this from the analog/stereo output when investigating reports of gain that ignores the knob.
- **Testing:** native JieLi firmware build, master fail-closed sanitizers over all 1024 ADC codes, copy isolation regression, source-level safety checks, legacy tests, HELP parity, FWSC integrity and boot/OTA asset equivalence. Real FM1/audio oscilloscope verification still required before using connected headphones or amplified output.
- TRK1 V26 project format and global sample bank remain unchanged. README contains operating guidance; release history remains only here.

## V1.6.10 — Native page-validator root cause of invisible DELETE popup

- **Confirmed root cause:** `beetle_ui_page_valid()` accepted screen IDs 0–30, 32 and 33–38, but the EDIT+SEL `DELETE PROJECT?` popup is screen **39** and the flash-delete progress screen is **40**. `screen()` calls `track_runtime_validate()` before drawing, which silently reset both unlisted screens to screen 0 (TRACKER). This made a recognized DELETE chord look like an empty/new project. Earlier key-delay fixes could not possibly correct this UI-validator defect.
- Added both DELETE pages (39 and 40) to the allowed native UI page range, preserving rejection of genuinely invalid page IDs (e.g. 31 and 41).
- Added a regression test reproducing the exact V1.6.9 failure against the old header and passing after the fix; also updated the existing SETTINGS page-validator regression to cover DELETE confirmation/progress.
- Input hardening: the FM1 physical matrix scanner now latches EDIT+SEL independently of the 18 ms UI loop. The LOAD browser consumes the event even if the chord is released between UI polls. Standalone NEW PROJECT is vetoed following an observed EDIT to prevent accidental blank-session transitions.
- A visible **BEETLE V1.6.10** version indicator appears on the on-device startup selector. LOAD PROJECT temporarily shows raw EDIT/SEL/chord observation after key activity, helping distinguish hardware key events, UI navigation and outdated firmware.
- Updated README and native/simulator HELP; project format remains **TRK1 V26**, global U01–U13 bank and unrelated functionality are unchanged. Hardware flashing, actual DELETE and independent recovery still require testing on a physical FM1. Back up projects before any destructive action.

## V1.6.9 — NEW/LOAD selector timing safety and project delete shortcut

- Addressed another NEW PROJECT input-race left in V1.6.8. The old selector checked other *held* buttons but ignored a separate EDIT press-and-release completed entirely between UI polls. That meant a pending SEL could still commit NEW after an attempted chord. The new input transaction checks held buttons, *all pressed/released edges*, and an additional 180 ms clean release window before any boot-menu action.
- LOAD PROJECT also requires a clean standalone SEL transaction (110 ms quiet after release). EDIT events cancel normal LOAD even when they appear only as queued edges or shortly after SEL-up. The native DELETE shortcut stays **EDIT+SEL** and the existing popup remains default NO; no automatic project creation is part of deletion.
- Added a host regression covering the formerly untested between-polls modifier case, both physical press orders, late EDIT events, short plain SEL taps, clean LOAD, and DELETE taking precedence over LOAD.
- The on-device HELP and WebSimulator HELP now clarify that project selection uses a short standalone SEL tap. No storage format change: TRK1 V26; other firmware functionality and global samples retained.
- The native JieLi compilation and firmware container/integrity validation were run, but **actual simultaneous hardware-key behavior is unverified**. Do not consider project deletion confirmed until it works on a physical M-VAVE FM1.

## V1.6.8 — Project selector input precedence and DELETE repair

- Reverted LOAD PROJECT deletion shortcut to **EDIT+SEL**. **EDIT+GLO** no longer deletes projects. The delete dialog still defaults to NO and requires a second, fresh SEL confirmation for YES.
- Fixed the upstream **NEW PROJECT / LOAD PROJECT boot selector**: SEL must now be released on its own before a new project is created or the load browser opens. Starting a chord with SEL (then EDIT), or with EDIT (then SEL), cancels the boot action instead of creating a new session. This is a root-level input-precedence change, not simply remapping the shortcut.
- LOAD PROJECT prioritizes the held EDIT+SEL chord over ordinary SEL release-to-load. No project is created or overwritten by either chord. Added host tests for both button orders, incidental GLO presses and the NO/YES confirmation gate.
- In-device HELP, WebSimulator HELP and browser button labels updated. No changes to TRK1 V26, samples, SID, USB audio, or project storage format. Physical button testing remains outstanding.

## V1.6.7 — LOAD hotkey, CH6 preview and sample buffering

- **LOAD PROJECT:** DELETE shortcut is now **EDIT+GLO** (EDIT and GLO held in either order). EDIT+SEL is no longer a delete shortcut. SEL remains the ordinary LOAD button and confirms choices in the modal. The confirmation defaults to NO; HOME cancels.
- **CH6 NOTE2/NOTE3:** Turning K3 on an eligible note plays the selected drum sound, as NOTE1 already did. This supports all factory drum IDs and the thirteen stored global FM1 samples. EMPTY and STP are silent; nothing is written or advanced by audition.
- **User-sample playback:** Two PCM decode windows per sample voice now each buffer 128 source frames rather than 64, doubling the per-voice prefetch horizon without additional voices, flash allocations or storage-format changes. The RAM increase lives in the existing temporary project workspace, which is exclusively owned by playback. This should reduce mid-song stalls when the USB and synth workload is busy. Verified software-wide; physical long-project recordings remain to be confirmed.
- **HELP and tools:** Native/Simulator HELP updated; HTML tools and installer aligned. TRK1 V26, user sample formats and protected OTA areas unchanged. Software-tested only; FM1 validation is needed.

## V1.6.6 — LOAD PROJECT EDIT+SEL input fix

- Fixed the on-device EDIT+SEL project deletion shortcut being ignored when a rising button edge is consumed by the input-safety filter. Chord detection now evaluates the debounced physical *held* keys, in either press order.
- Requires both buttons to be released before arming the shortcut (also after navigating from the NEW/LOAD boot screen). Prevents double triggers, accidental project load, or reuse of the popup-opening key press as confirmation.
- DELETE PROJECT? still opens with NO selected. The user must release EDIT and SEL, explicitly choose YES with K1 and press SEL again before a delete is attempted. Other projects and global samples are not part of the delete operation.
- Added isolated host regressions for suppressed key edges, EDIT-first/SEL-first, boot transition and modal confirmation lockout; physical FM1 still unverified.

## V1.6.6 — Boot Project Menu: SELECT LED Only

- In the initial **NEW PROJECT / LOAD PROJECT** chooser (FM1 native UI page 29), the only lit hardware function button is **SEL**. All other buttons and note-key LEDs stay off, even if scale guide, keylock, octave or lingering splash LED states were previously active.
- Applies only after the boot splash hands off to the chooser. The boot animation itself and all other screens keep their normal lighting; after confirming NEW or LOAD, the regular LED policy resumes.
- Implemented in native `track_fm1_leds()` with full removal of dim, mid and breathing LED planes; a hardware-independent regression verifies all 41 LED IDs across pages and splash boundary.
- WebSimulator hardware-key indicator and in-device HELP synchronized. The feature adds no project data or flash writes; TRK1 V26 remains unchanged.
- Native build and offline tests only. Real M-VAVE FM1 button brightness and transitions still need testing.

## V1.6 — Load Project: Confirmed Delete

- In **LOAD PROJECT**, hold **EDIT + SEL** on an occupied project slot to open a dedicated **DELETE PROJECT?** confirmation popup.
- The popup is in English, starts at **NO**, shows the slot/project name and offers **YES / NO**. Turn K1 to choose, press SEL to confirm, or HOME to cancel. Both physical EDIT+SEL press orders are recognized; LOAD is now triggered on SEL release to avoid loading accidentally while pressing the delete chord.
- YES deletes only the selected project slot (Storage V2 or safely verified legacy journal). Other projects and global U01–U13 samples are preserved. Read/write and safe-operation checks remain enforced; delete failures display a storage error.
- On-device HELP, WebSimulator HELP and simulator project picker updated. No project schema change (TRK1 V26).
- Experimental native firmware. Physical FM1 deletion, power-loss recovery and flash timing have not been validated. **Back up projects before using DELETE**.

## V1.5 — Sample Auto-Conversion, NOTE3 First-Frame Streaming & Clean WebInstaller (25 October 2026)

- Version-label correction: the immediately preceding release was mistakenly labeled V1.4.4; this is that same build, correctly designated V1.5. Firmware payload, project format and user sample storage are unchanged.

- Global sample import in Project Manager now decodes browser-supported audio (WAV/MP3/FLAC where supported), downmixes to mono, resamples to 22,050 Hz and writes 16-bit PCM WAV before storage/BD4S device upload. Existing incompatible sample-bank WAVs are converted in memory before device transfer. Clips longer than 20 seconds require explicit confirmation to keep only the first 20 seconds; unsupported/corrupt files are rejected before modifying slots. The FM1 still uses the existing BD4S 4-bit storage codec.
- The first-step NOTE3 user-sample bug persisted in V1.4.37 because initial-prefetch gating did not protect subsequent PCM-window starvation. The live streamer now probes BOTH interpolation source frames before advancing the 44,100 Hz output cursor; a missing window stalls the drum voice phase and envelope until background prefetch completes. This applies to all three CH6 NOTE lanes. No flash reads were added inside the DMA audio interrupt and no additional PCM buffers allocated.
- WebInstaller refreshed to a compact, dark BEETLE-style magenta UI with neutral safety notices and fewer repetitive high-visibility warning blocks. Consent, checksums, device verification, recovery information and confirmation controls remain functional.
- Project format remains TRK1 V26. On-device HELP content continues to cover NOTE3, SONG LOOP, sample playback and project controls; unchanged controls need no new HELP entries. README stays an operation manual, not a release log. Physical FM1 playback, USB audio and OTA recovery remain unvalidated.

## V1.4.37 — FM1 HELP and NOTE3 first-step sample startup (25 October 2026)

- Updated FM1 on-device HELP for CH6 SEL+FX NOTE3/FX view, independent NOTE3 drums and global samples U01–U13, focused-note deletion, own STP, muted (but preserved) CH6 FX, and note entry without step advance.
- Updated SONG CHAINS HELP: SEL short press, 650 ms hold for SONG LOOP, SEL+K1 range, and looping precisely selected rows when pressing PLAY.
- Updated project SAVE/LOAD HELP to explain persisted NOTE3 mode and note content in TRK1 V26.
- Kept WebSimulator HELP text identical to the native help array. Added mandatory automated HELP parity, 36-glyph width, chapter count and essential topic checks to clean-release verification, plus an explicit per-iteration HELP review rule.
- Fixed global BD4S sample voices progressing their drum phase, age, amplitude envelope and gate while first waiting for background prefetch. The entire attack now remains intact instead of only playing a later fragment.
- On transport PLAY, the first DMA frame waits (up to 0.5 seconds) for initial NOTE1/NOTE2/NOTE3 user-sample PCM windows to be ready; all six channels and USB capture remain aligned. The wait is silent and bounded; the audio IRQ never reads flash or decodes samples.
- If a sample fails validation during preparation, its startup state is cleared so it cannot indefinitely block PLAY.
- The existing `.beetle` project schema TRK1 V26, NOTE3 FX-view persistence, sample bank, 6-voice drum architecture and protected OTA assets remain unchanged. The FM1 firmware is experimental and physical playback/USB/OTA recovery remain unverified; README remains a user guide, not a version history.

## V1.4.36 — CH6 third drum lane, FX view toggle and fixed tracker note cursor (experimental)

- On CH6 TRACKER with transport stopped, **SEL + FX** toggles between `STEP · NOTE · NOTE2 · KIT · FX` and `STEP · NOTE · NOTE2 · NOTE3 · KIT`. In the second mode, the old CH6 FX column is hidden; existing CH6 FX data and events are preserved but their effects are muted during playback until the user returns to FX mode.
- NOTE3 supports all 27 drum IDs (14 factory + U01–U13 global samples), independent keyboard entry, K3 note selection, focused SEL erase, empty and independently scoped `STP`; third drum hit is mixed through existing voices (no new allocated drumvoices), and is retained during song composition and pattern/lane clipboard transfers.
- **NOTE entry does not advance** the editing cursor on any channel. NOTE2 and NOTE3 remain non-advancing. Other step navigation still uses K1.
- Projects now store the CH6 NOTE3 enabled flag and sparse NOTE3 per-pattern entries in **TRK1 V26**. Supported V25 and legacy V23 projects still decode, initializing NOTE3 disabled and empty; V26 cannot be read by V25 firmware. The independent pattern codec is now TR5, with older TR1–TR4 formats readable.
- WebSimulator mirrors NOTE3 display/editing/recording and native TRK1 V26 project import/export. The current Project Manager and bulk ZIP tools accept the device-reported V26 project schema; global sample data remain stored separately.
- Regression test `tests/track/test_v1436_note3.c` verifies 27 drum IDs, cursor stays on row, V26 project mode on/off persistence, independent NOTE3/STP, hidden FX preservation, pattern V4-to-V5 migration and actual V25 legacy-project decoding. The previous V23 migration regression is run against an actual V23 fixture produced by the V25 baseline project encoder instead of treating V26 bytes as V23.
- JieLi native build, firmware container/header/OTA loader validation, relevant host tests and browser JavaScript syntax pass. Real FM1 control timings, 3-drum hit load, live USB capture and OTA flashing/recovery **not hardware validated**. Back up before flashing.
- README is version-history-free (stable user guide and credits only); all release-specific notes are here. Release ZIP contains only current firmware and supported project files.

## V1.4.35 — Punch-In FX bank LEDs, SONG SEL long hold, secure projects bulk ZIP and V23 fixture fix (experimental)

- Eight physical FM1 white-key LEDs display the eight effects of the current Punch-In page. Existing held keys and Punch-In performance behavior preserved. WebSimulator mirrors page illumination.
- Hold SEL >=650ms in SONG CHAINS to toggle SONG LOOP. A short SEL retains V1.4.34 range clear / SONG toggle behavior; SEL+K1 always remains range selection; selected-row PLAY loops its own range regardless of global loop status.
- Project Manager supports EXPORT PROJECTS ZIP and IMPORT PROJECTS ZIP in addition to individual encrypted `.beetle` operations. Only occupied project slots are exported; original slot numbers and checksums included in manifest; contents authenticated and verified before write; explicit two-step confirmation required for replacements; no implicit deletion of absent slots. Bulk import is sequential, not atomic. **Global U01–U13 samplebank is not included.**
- Corrected V23 test fixture instead of loosening production decoder. The test now removes later V25-only punch settings and creates an authentic V23 byte layout before roundtrip migration assertions. No project storage-format change.
- Tests: native JieLi build; V23 song migration; V1.4.34 range looping; ZIP roundtrip and hostile ZIP rejects; JS syntax; package roundtrip and first 16 KiB/OTA loader identity. Physical hardware/OTA tests not yet performed.
- Separate old `test_rc1011b_pcm_compat.c` checksum in `run_rc1011b_audio_stability.sh` also fails on identical V1.4.34 baseline (`dd0f3681` measured vs stale expected `43c0f1f6`); not modified or counted as a pass.
- Project release remains clean: one current `.fwsc`, current HTML tools, source, tests, docs, and licenses; no old generated releases.
- Documentation-only cleanup: moved chronological "New in V…" release narratives out of README; functional user instructions, installation and credits stay in README. All release changes belong in this CHANGELOG.

## V1.4.34 — SONG selection, range looping and UI/transport race fixes (experimental)

- SONG row selection is now a persistent transient inclusive range: SEL+K1 anchors correctly, works forward/backward and across screen pages, and uses the tracker-selection magenta fill and cursor outline.
- PLAY from SONG CHAINS with a range starts at the first selected row and loops back exactly after its last row, regardless of ordinary Song Loop. REP per song row, lane-to-pattern composition and 64-row bounds are retained. Starting PLAY without a range retains the full song semantics.
- Single tap of SEL clears a song range instead of accidentally toggling SONG mode. A fresh SEL tap without a selection still enables/disables SONG normally.
- SONG row editing, insert and delete are blocked during PLAY to prevent concurrent ISR/ringbuffer corruption; stopped structural editing invalidates the old transient range.
- On STOP, return to pre-play cursor; range remains selected for replay until the user clears or replaces it. Range is not serialized, keeping TRK1 V25 project compatibility.
- WebSimulator's WebAudio scheduler now starts and wraps inside the exact same range; UI highlighting and pointer/shift selection updated. Current tools embedded into the standalone installer and All-in-One Manager.
- Added `tests/track/test_v1434_song_selection_loop.c`: forward/reversed/single-row/repeat/global-loop tests and unchanged row-memory assertions. Pre-existing V23 test fixture regression in `run_rc1011_song_chains.sh` still fails in V1.4.33 baseline; documented separately rather than reported as passing.
- Experimental native JieLi build (USB 44.1kHz, V2 storage), protected OTA asset and package equality audit pass; physical FM1 audio and flash safety not verified.
- Clean release contains only current firmware and web files, full source/tests/docs/licenses, retaining original README styling and credits.

## V1.4.33 — Tracker chord scrolling audition (experimental)

- On TRACKER CHD (CH1–CH5), K3 now auditions the saved root note using the newly selected chord type. All 12 chords, up to five simultaneous voices, use `track_chord_keylocked()` and the existing stored synth instrument.
- Uses one transient multi-note preview record: K3 changes stop all former preview tones first, and K1/K2 movement, page changes, PLAY and the preview timeout release the audition. No new notes are recorded, no extra project fields, no new flash writes or changes to saved note/chord encoding.
- EMPTY/STP root, drum lane and running transport never produce an unwanted chord audition. Mono voice mode intentionally respects its single-voice limit.
- WebSimulator now auditions the same chord shapes via WebAudio, separately from pattern playback and project serialization.
- Native JieLi 44.1 kHz USB/Storage V2 build, chord policy host regression, JavaScript checks and fwsc protected-asset validations pass. Physical FM1 audio/flash/OTA remains untested.
- Clean source ZIP per release-hygiene rule; README GitHub banner/shields/credits layout preserved.

## V1.4.32 — USB-C stereo/multitrack capture recovery and late-DMA margin (experimental)

- Increased 44.1 kHz UAC ring priming from 104 to 224 frames, with the existing 512-frame stereo/multitrack rings unchanged. Optional 48 kHz stereo priming grows from 113 to 244 frames. This intentionally adds about 2.72 ms to startup capture latency to tolerate late 128-frame renders.
- Stereo now uses the slowly filtered, bounded shared per-DMA SRC controller used by six-stem USB recording. No packet-by-packet pitch retuning.
- Explicit bounded recovery when USB runs dry or its ring becomes full: fade the remaining packet to silence on starvation, mark for reprime, restart SRC/phase at the next audio render, and fade in the first 24 frames of fresh PCM. Never fill the packet indefinitely with the previous sample.
- Added safe ring-capacity checks against unsigned subtraction underflow; overrun never silently drops successive blocks while keeping stale read positions.
- SYSTEM → USB AUDIO now displays RECOV for stream re-prime events, retaining UND/OVER/STALL. Other USB diagnostic counters remain in the native source.
- New six-minute injected DMA/host-pause simulation, 10-minute nominal stereo/multitrack clock tests, and fade/free-space helper tests under undefined-behavior sanitizers; original SID, RETRO PULSE, FX, NOTE2 code and project format retained.
- Fully cleaned release output each iteration; no old FWSC/HTML duplicates inside the complete source ZIP.
- Caveat: physical FM1 USB-C capture, USB ISR load, recovery after real host stalls and OTA flashing are **not** verified.

## V1.4.31 — SID wah / default-detune regression correction

- Restored the V1.4.29 6581/DIRTY/BASS filter coefficient and resonance mapping. The V1.4.30 experimental curve overly boosted the default cutoff (96 → ~97 rather than ~37), potentially altering the audio character and accentuating phasey sweeps.
- For those three SID types, neutral legacy oscillator B settings use oscillator A's phase to avoid the new involuntary 12-cent detune beating. Independently tuned B, Sync, Ring and combined-wave features remain available.
- SID 8580 and SID NOISE DSP are unchanged compared with V1.4.30.
- Regression: four-second sustained notes check amplitude stability and custom-detune variation; filter mapping checked across original cutoff range. Physical FM1 testing and OTA recovery remain unverified.
- All previously implemented features, project format V25, sample storage, USB and boot-menu key isolation retained. Only the current release is shipped in the cleaned ZIP.

## V1.4.30 — SID authenticity/performance + NEW/LOAD keyboard isolation (experimental)

- SID A and B now use distinct oscillator phases and tunings, rather than using the same phase twice. Added optional A-master/B-slave Hard Sync, triangle Ring Mod and combined waveform DAC mode, mapped to previously unused SID SUB/NOISE/FM DEPTH parameters in the same stored patch schema.
- Added separate ROM-based 6581/8580 filter response curves interpolated once per voice per render chunk. Three per-sample 64-bit SID state-variable filter multiplications use proven bounded 32-bit products; no new voice allocation or large DSP buffers.
- Boot **NEW PROJECT**, **LOAD PROJECT**, and confirmation/loading dialogs drain queued FM1 note-on edges. Musical keys held while leaving these screens stay suppressed until released. No spurious note writing on entering tracker.
- Added dedicated host regression for chip curves, sync/combined/ring, and project-menu key fencing. Existing BITCRUSH/RADIO, NOTE2 and RETRO PULSE regressions remain passing.
- Unchanged project schema TRK1 V25, 13 global samples/NOTE2, six channels, USB multitrack and OTA asset lineage. New SID sound is an approximation, not chip-exact reSID. Physical FM1 performance, audio quality and OTA recovery are still unverified.

## V1.4.29 — RETRO PULSE sustain/DC correction (experimental; FWSC separately built)

- Removed 4-bit amplitude stepping from the ADSR that could sound like retriggers during sustain/decay/release. Preserved the DMG eight-step duty waveform.
- Centered the duty waveform by its exact high-time fraction, avoiding the old slow per-voice DC correction that could contribute a low-frequency residual.
- Reused existing per-voice DSP state, no additional audio buffers, unchanged project format or stored patches.
- Host regression `tests/track/test_v1429_retro_sustain.c` verifies no new note-on serial, stable sustain, DC < 5% of absolute amplitude, and full release.
- Native V1.4.29 `.fwsc` was subsequently built with unchanged protected loader/header; it remains unverified on physical FM1 hardware.

## V1.4.28 — Global User-Sample Streaming & Loop Optimization (experimental)

- BD4S interpolator caches the pair of 22.05 kHz source samples for each 44.1 kHz even/odd output pair, removing redundant per-frame two-window scans while retaining sample-exact output and bounded underrun recovery.
- Full 256-frame BD4S blocks are read in one 134-byte background flash transaction; short/tail blocks remain bounds checked with a two-stage fallback. No flash I/O or decoding is added to the audio interrupt.
- Global user-sample drum voices bypass the unrelated factory-ROM 64-bit interpolator; legacy drums still use their original resampling behavior.
- Added 64-loop x six-voice retrigger stress test with PCM-exact output, no render-time flash reads and zero host underruns, along with preexisting Storage V2 40,000-frame live-decode tests.
- Project TRK1 V25, global BD4S sample format, labels, NOTE2, Punch-In FX, Storage V2 project reserve and protected OTA assets retained. FM1 hardware stress/OTA verification still outstanding.
- **Release hygiene:** obsolete generated binaries, prior-version duplicate HTML/README files, build directories, bytecode and Git object history are excluded from the release ZIP; functional source, web tools, tests, licenses, references and documentation are retained.

### V1.4.27 — Punch-In BITCRUSH/RADIO DSP & CRY Renaming (experimental)

- Renamed Punch-In FX 09 from SCRATCH to **CRY**, including native UI, Punch-In event editor, simulator and README. FX ID 8 (zero-based) and project schema **V25** remain unchanged.
- BITCRUSH now combines 12–4-bit sample quantization with a continuous stateful sample-and-hold reducer (1–31 PCM frames at 44.1 kHz), preventing the formerly subtle bit-depth-only effect. Default K1 50 is approximately 8 bits at 2.76 kHz effective update rate.
- RADIO now applies two cascaded highpass stages (bass/low-cut) with subtle independent stereo hiss; upper frequencies remain present and the dry signal is never replaced by hiss. Short fades tame key-on transients. No additional PCM history allocation.
- Added native frequency-response, low-amplitude BITCRUSH and noise-level regression tests, with UndefinedBehaviorSanitizer; V1.4.26 NOTE2 and FX tests retained.
- JieLi native USB / Storage V2 build succeeded. Package preserves the protected header, original OTA loader and `FM-1_2123` identity. Hardware playback and OTA/recovery validation remain outstanding.

## V1.4.26 — Punch-In DSP Corrections, NOTE2 User Samples & Reduced Flash Traffic (experimental)

- Reworked SCRATCH's short-buffer forward/reverse transport, improved BITCRUSH resolution, corrected 4-stage PHASER, tuned RADIO band-pass/hiss and DJ ISOLATOR bass/mid/treble cuts. Fixed recovery of all 16 Punch-In parameter values.
- NOTE2 now supports all fourteen factory drums and thirteen user samples. A formerly unused bit in the drum KIT field extends NOTE2 IDs without adding bytes to TRK1 V25, changing NOTE1 or breaking legacy NOTE2 data. V1.4.25 will not load a project containing these new extended NOTE2 values; export before downgrading. Native runtime and WebSimulator use the same encoding.
- Improved Storage V2 global sample streaming: skips redundant flash reads, reuses one verified sector across voices and maintains per-voice generation validation. Six-voice native host-streaming tests and 27-NOTE2-value project codec tests pass.
- JieLi native build with UAC/Storage V2 passes, firmware container verified with protected header and OTA loader unchanged. **The physical FM1 DMA deadline, sound quality, USB recording and recovery have NOT been validated.** Back up project/sample data before upgrading.

## V1.4.25 storage reserve and sample capacity readout (experimental patch)

- Read-only sample-capacity query `0x76`/`0x7E` reports the verified active sample data size and available physically blank pages, counting both superblock generations and excluding dirty orphan sectors.
- Sample uploads now preserve eight complete erased sectors (32 KiB) for future project writes **in addition to** the existing four-sector recovery guard. Upload preflight fails closed without writing flash on insufficient space. This does not guarantee an arbitrary number of large future project writes.
- Project Manager FM1 SAMPLE SPACE bar displays reported capacity, the estimated remaining bytes and the reserved project and recovery space. Older firmware clearly reports NOT VERIFIED rather than fabricating a capacity percentage.
- Sample-storage failures display `FM1 STORAGE FULL` and do not claim a refused upload succeeded. Source and browser tests do not establish physical FM1 reliability.

## V1.4.25 — Project Manager sample reconnect / non-destructive sync hotfix (browser-only)

- Fixed the root cause of user samples appearing empty after reconnect: the Project Manager now sends the existing read-only `0x70` INFO request for all 13 U01–U13 FM1 slots and displays **FM1 STORED** status independently of the PC-side WAV bank.
- Uploading WAVs updates **only** occupied PC-bank entries requiring transfer; empty PC-bank entries never issue FM1 DELETE. Occupied device slots with different content are individually identified in a preflight plan and require explicit replacement confirmation.
- Added an explicit per-slot **DELETE ON FM1** action with acknowledgment and a fresh fingerprint check; deleting a WAV from the PC bank never deletes the corresponding FM1 sample.
- Enforced complete 13-slot scan before any writes, reject changed slot generation/checksum, and verify every untouched FM1 sample after an upload. Error/partial scans disable uploads.
- Added **READ FM1 SAMPLE SLOTS** and clear device-versus-PC status for every entry, including after reconnect.
- Original installed firmware V1.4.25 and TRK1 V25 project layout are unchanged. **No firmware flash required.** Existing native protocol exposes only sample occupancy/size/CRC/generation, not the original WAV or tracker label: users must retain their `.beetlebank` files.
- Tested in Chromium with mocked FM1 USB-MIDI replies; physical-device validation remains outstanding.

## V1.4.25 — Punch-In FX Page 2 and Global Sample Labels (experimental)

- Added eight new effects: SCRATCH, BITCRUSH, PUMP, GLITCH, WOW, PHASER, RADIO, and DJ ISOLATOR. Existing eight effects remain.
- Added automatic 1/2 or 2/2 Punch-In screen focus, with last-held-key priority on overlapping presses.
- Expanded FX key and recording masks to 16 bits; new TRK1 V25 stores 16 IDs and 13 adjustable Punch-In parameters. Old V24 projects remain readable.
- Kept 12 KiB shared Punch-In audio history; replaced 64-bit sample interpolation with signed 32-bit math.
- Added Project Manager three-character user sample labels stored in backwards-compatible .beetlebank V2 files and written in a metadata footer alongside compressed sample data. Existing sample format and 13 global slots retained.
- Native JieLi compilation is **not** physical hardware certification. Long-duration DMA, full-load DSP, USB multitrack, flashing and recovery tests remain necessary.

## BEETLE V1.4.24 — Project Manager Storage V2 conversion

- Added read-only USB-MIDI migration preflight and guarded partition initialization (Project Transfer V5, commands `0x5D`/`0x5E`).
- Added **CONVERT STORAGE TO V2** button, encrypted 12-slot backup ZIP, pre/post fingerprint verification, project restore, full slot rescan.
- Added **RESTORE V2 BACKUP ZIP** to resume project restoration without formatting again.
- Legacy V22/V23 project payloads are rewritten to V24 during restore, preserving original encrypted source backups.
- Scoped NOR erase to BEETLE-owned storage (`0x97000..0xDFFFF`), keeping boot/app/OTA regions outside the write range.
- Firmware package identity remains `FM-1_2123`, based on V1.4.5.
- Failure during migration can require recovery from downloaded PC backups; physical device test still needed.

## V1.4.23 — Legacy Project Conversion & Tracker REC UI

- Project Manager now scans stored project schema versions via CRC-checked native read-only transfer headers and visibly identifies old project versions in the twelve slots. User-controlled bulk conversion first exports an encrypted ZIP of every original, waits for backup confirmation, then rewrites only selected slots using the native V24 encoder and verifies each converted project by reading it back.
- Importing older `.beetle` files prompts for conversion and, on success, exports an upgraded encrypted backup. Legacy password-based containers remain supported. Conversion of project schema does not silently reformat the physical legacy storage partition.
- Removed `REC ON` / `REC OFF` labels from the tracker screen (native LCD and WebSimulator). Physical REC LED and Punch-In FX recording remain unchanged.
- Package identity `FM-1_2123` and protected V1.4.5 header/OTA loader preserved; global sample functions retained, still requiring a migrated V2 flash partition for on-device sample storage. Hardware tests still outstanding.

## V1.4.22 — Native BD4S Global Sample Playback / CH6 (experimental)

- Connected conditional V2 Storage global U01–U13 notes to the **existing six** native CH6 drumvoices. DPCM4 prefetch and storage verification are done on the main thread, not inside audio DMA.
- Project Manager uploads converted WAV samples to a V2 volume and verifies per-slot checksums; samples are project independent.
- Fixed double IRQ lock during sample start, flash/scratch ownership races in sample INFO, BEGIN and DELETE, and erroneous ROM-duration recalculation on streaming voice retune.
- Offloaded large live-stream runtime into project staging scratch to respect FM1 RAM linker limits. Project and sample-management operations pause native audio while borrowing scratch and reinitialize stream state afterwards.
- Tested six parallel native stream voices, repeated full sample PCM comparisons, 12-project/13-slot Storage V2 regression, and native JieLi builds. These are software tests; the physical USB/FM1/DMA timing path remains unmeasured.
- **Compatibility:** V2 bank requires an explicit migration of the legacy BEETLE flash volume using local authenticated backup and recovery-image tool. Legacy projects remain readable in the legacy partition layout. Installing V1.4.22 alone does not silently migrate flash.
- Maintained V1.4.5 `FM-1_2123` product identity and protected OTA-loader bytes. Firmware is experimental, not a hardware-certified final release.

## V1.4.21 — USB sample storage transfer (native, experimental)

- Global U01–U13 WAVs can be converted to original BEETLE BD4S/DPCM4 in the Project Manager and transferred through 24-byte acknowledged USB-MIDI packets to an **already migrated** Storage V2 volume. No samples are duplicated into projects. An incomplete upload never becomes the active index.
- Native writer reserves sectors in advance, checks full payload CRC and inner BD4S structure, writes and reads back data, and commits the index last. 12 project slots remain in an independent storage domain.
- Preserves legacy project .beetle data and the `FM-1_2123` product ID/verified V1.4.5 package basis. No automatic migration of a legacy device is attempted.
- Fixed a live-drum metadata alias: temporary flash work area cannot replace the bound one-shot pointers. Restores pointers before V2 decode. USB upload timeout returns the firmware to normal input handling.
- **NOT COMPLETED:** Firmware on a non-migrated FM1 still uses the old A/B storage; the physical migrator is not run in this firmware, and BD4S samples in the global V2 bank are **not yet rendered by the real audio mixer**. The native prefetch engine remains a separately tested module.
- Real FM1 USB transfer, sample playback, DMA deadlines and OTA recovery have not been physically validated. This is an experimental firmware candidate, not a production-safe user-sampling release.

## V1.4.20 — Native Storage V2 Project Route (experimental)

- Implemented native C copy-on-write Storage V2 writer in firmware: two CRC-checked metadata generations, readback verification before commit, independent 12-project/13-sample namespaces and separate flash-erase sector ownership. Native project SAVE, LOAD, DELETE and USB project transfer select V2 only on an owned, pre-migrated V2 volume; legacy TRKJ stays available on its original marker.
- Corrected a blocking migration incompatibility: V2 recovery images use `BTLSTV2\x01`, NOT the original legacy marker. Native V2 marker generation now matches the host image builder byte for byte; both the 32-byte marker and erased tail of its 4KiB sector are checked. V2 volumes reject all legacy-format destructive provisioning.
- Exercised all 12 project and 13 sample slots, storage-full refusal before writing, CRC-corrupted metadata rejection, and a cut after each of 71 NOR mutations in the new C writer; old or new committed data recover in the simulator.
- New experimental FM1 firmware and WebInstaller use the V1.4.5 `FM-1_2123` package identity and unchanged protected boot header/OTA loader. Native JieLi target builds pass. Actual device flashing/storage safety and DMA performance have not been demonstrated.
- **Not complete user samples:** native V2 sample writer and BD4S prefetch have host tests, but there is still no user-sample USB upload protocol or DPCM4 live mixing on the FM1. Do not present V1.4.20 as a fully operational user-sample release.

## V1.4.19 — Independent Global User Sample / Project Sector Allocation (source prototype)

- PROJECT and SAMPLE storage now use separate erase-sector ownership, with shared dynamic free space and a mandatory four-erased-sector reserve.
- Packed project records share project-sector pages; sample records can share sample-sector pages. No inter-domain page or sector overlap.
- Native read-only C audit verifies sector and page isolation; forged indexes are rejected.
- New `tools/storage_v2/report_coexistence.py` audits local V2 images read-only.
- Regression-tested 12 projects + 13 samples at a near-20-second data budget, cold restart, project/sample replacement and storage-full refusal.
- Existing V1.4.14 on-device firmware unchanged. Physical V2 writers, migration, transfer and sample playback are still pending. Package identity remains `FM-1_2123`.

## V1.4.17 — Read-only legacy A/B flash journal audit (SOURCE-ONLY)

- New offline `legacy_flash_audit.py` for independently obtained 1 MiB FM1 flash images. Verifies full BEETLE marker sector, all 24 legacy A/B banks, two layers of CRC, transaction commit, occupied tails, schema, slot identity and generation wraparound. Unknown or ambiguous data fail closed.
- Optionally exports only validated TRK1 projects from all 12 slots to a new private directory and builds an explicit migration manifest; the existing Storage V2 migration preflight then checks capacity and byte-for-byte readback after a simulated restart. No live device reads or flash writes.
- Added synthetic flash fault-injection tests with real legacy MOUNTAINKING/NUGGIE test projects, damaged-bank fallback and refusal of foreign, corrupt and conflicting records.
- Original V1.4.14 firmware and WebInstaller remain unchanged, identity `FM-1_2123` from V1.4.5; no new `.fwsc` and no claim of functioning on-device global user samples.
- See `docs/V1417-READONLY-FLASH-JOURNAL-AUDIT.md`.

## V1.4.16 — Offline verified migration preflight & clean source distribution

- New twelve-slot explicit backup audit with authenticated `.beetle` V2 exports and TRK1 CRC verification.
- Optional validated BD4S U01–U13 inputs; full in-memory Storage V2 write and byte-identical reboot readback.
- Reject missing/duplicate slots, tampered AES-GCM tags, broken CRC, path traversal and over-capacity migrations.
- Production flash driver, OTA, sound engine, package identity and installer remain exactly V1.4.14. No V1.4.16 `.fwsc` is represented as operational.
- Cleaned distribution of obsolete compiled files, duplicated browser copies and old release clutter; current source and test capabilities retained.
- See `docs/V1416-MIGRATION-AND-CLEANUP.md`.

## V1.4.15 — Global Sample Generation Leases & True Flash-Sector Reserve (OFFLINE)

- Corrected an unsafe storage-model reserve check: 4 **whole erased sectors**, not 4 empty pages, must remain after a transaction. Capacity refusal occurs before writing.
- Added immutable per-voice sample-generation leases, sector pinning and release in the original BEETLE Storage V2 model. Replace/delete/checkpoint while active voices play cannot silently switch their samples or reclaim in-use records.
- Native six-voice BD4S host prefetch gains a generation-aware callback, pinned prepare/poll and explicit disarm. The unpinned legacy API is retained and cannot service a pinned voice.
- Added real compiled C + Python cross-integration tests with concurrent old/new generations from one U-slot, same-output PCM, 12 independent projects and atomic storage-full rejection.
- **No new FM1 firmware**. V1.4.14 `.fwsc`, WebInstaller, package identity `FM-1_2123`, protected loader, existing FM1 audio and flash layout remain unchanged. No live 10–20-second sample bank yet. See `docs/V1415-GLOBAL-SAMPLE-GENERATION-LEASES.md`.

## V1.4.14 — Cross-firmware OTA compatibility lab + sector-backed sample streaming (21 October 2026)

- Kept the **exact V1.4.5 FM-1_2123** package identity, protected first 16 KiB and existing OTA loader. No bootloader replacement.
- Expanded BEETLE's outgoing OTA entrypoint to inspect Felucca-family app-only loaders by a bounded versioned family marker and RAM entrypoint. Validates their entire body CRC **before any flash erase**; continues to verify the complete incoming FM1 application/XIP layout and CRC before staging. Existing exact pinned community loader and exact official stock V15 exception remain. Unknown loader formats, corrupt payloads and unsafe app layouts are refused.
- Marker+CRC checks only package *format and accidental corruption*, **not authorship**. Third-party firmware/loader execution cannot be made risk-free. The Hub independently verifies catalogue SHA-256, but BEETLE has no cryptographic signature trust list for arbitrary other firmware.
- Added offline native OTA replay cases for the original loader, synthetic differently-built compatible family loader, unknown family and corrupt body. Simulated successful commits and verified zero erases on refusals. Not hardware-proven and not a universal guarantee.
- Added independent bounded `Storage.read_sample_range` with record CRC validation, paired with the actual six-voice BEETLE DPCM4 C prefetch engine in an integrated host test. Tests 13 banks, 12 original project slots, PCM exact output, replacement, removal, reboot, integrity fault injection, zero storage reads within audio render. **Never wired into FM1 flash/DMA.**
- Regenerated the WebInstaller with the exact V1.4.14 firmware binary and hash, retaining existing update/restore restrictions; does not turn the installer into an unrestricted flashing application.
- **Hardware reminder:** MOUNTAINKING DMA crackles, actual Firmware Hub cross-install, storage migration, RAM budget and flash ownership still require physical validation. No flash layout changes in this firmware.

## V1.4.13 — REC LED / Firmware Hub read-only diagnosis / sample note routing

- Native REC physical key #13 LED now follows actual note-entry state, and on FX/Punch screens follows the separate Punch-In record state. The boot ripple still owns LEDs while playing. Host tests cover all screen/status combinations.
- New `BEETLE-Hub-Diagnostics.html` queries identity (0x11) and the app-specific BEETLE HELLO (0x50/0x60), **read-only only**. It can distinguish a normal running BEETLE application from an ambiguous updater-ID warning after a live USB test. Not an alternative flash tool.
- Identified that BEETLE's OTA writer only accepts byte-pinned community loader and exact official stock V15 route; arbitrary third-party loader support is not safely implemented. Hub UI classification is outside the firmware; **switching via The Firmware Hub remains unverified and cannot be claimed fixed**.
- Own, host-tested routing for factory drums 00–13 and user sample notes 14–26 (U01–U13), no firmware DSP or sample flash writes added.
- All existing 12 project slots, mono SID note fix, Song Chain V24, LCD playhead, digital drum voices, V1.4.9 sample prefetch source, and package ID `FM-1_2123` preserved.
- OTA and audio hardware validation remains open.

## V1.4.12 — Song Keyboard LEDs, REC Workflow & Row Clipboard (hardware-unverified)

- Song Chains: physical keyboard LEDs follow the selected lane's sounding notes/chords (synth voices or triggered drum voices). Preview is computed in the foreground UI, with a bounded scan of existing voices; no new audio ISR computations. The WebSimulator highlights mapped white keys using project steps, not actual FM1 voice state.
- REC: outside Step FX/Punch-In menus, REC toggles Tracker note writing, default **ON** after boot/new project; OFF auditions notes without modifying steps. Within FX/Punch-In, REC toggles Punch-In recording only. The two modes have separate state.
- Song Chains: hold SEL + turn K1 to select row range; SEQ+OP1 copies, SEQ+OP2 pastes contiguous rows. Independent Song clipboard, atomic bounds check (maximum 64 rows), Undo support on native device. Existing Tracker shortcuts remain intact.
- FM1 battery ADC sampled in the foreground every four seconds, filtered/hysteretic; the centered BEETLE title is drawn red when low. **20% is a provisional raw-ADC estimate**, not calibrated on physical FM1; the `BEETLE_BATTERY_20_ADC_RAW` threshold must be measured and calibrated before a 20% guarantee.
- Native readonly and OTA/USB JieLi builds compile; Song row/battery unit tests, mono-SID tests, drum loop tests, and sample-prefetch host tests pass. This is *not* real-device LCD/LED/battery/USB or audio-stability validation.
- Package identity `FM-1_2123`, protected V1.3.8 boot header and OTA loader unchanged. Native global samples remain an **offline prototype**, not yet installed in hardware.

## V1.4.11 — Song STOP, Row Clone & Drum Loop Stability Laboratory (experimental)

- Song Chain `--` on CH1–CH5 releases sounding notes at the position boundary; on CH6 it chokes active drum tails over 48 output frames. An explicitly selected numbered empty pattern intentionally allows sustain.
- The first row auto-references P01; inserting a row copies its predecessor's six lanes, manual/automatic binding state and REP.
- TRK1 V24 persists the difference between auto-`--` and manually selected empty pattern; prior V23 projects read correctly, but older firmware cannot open new V24 saves.
- The Project Manager's legacy sample editor preserves V24 on export instead of accidentally downgrading it to V22/V23.
- Optimized factory drum ROM lookup in the sample loop; 128 dense single- and dual-hit pattern loops render **bit-identical** PCM to V1.4.10, with no change in voice count or sample quality.
- BD4S streaming lab now supports a 48-frame fade on Song `--`; existing prefetch tests pass. **Still host-only, not connected to physical FM1 flash or DMA.**
- Native JieLi read-only and experimental OTA/USB builds succeeded; exact physical AUDIO STOP / DMA success and OTA recovery remain unverified.
- Full methodology: `docs/V1411-SONG-STOP-DRUM-LOOP-VALIDATION.md`.

## V1.4.10 — Mono Audition and Song Lane Visibility (experimental)

- Instrument EDIT MONO no longer discards successive physical note-ons in a fixed 45 ms window. Eight-millisecond key-up stabilization, deterministic last-note priority, return to held keys, and generation-bound note-offs prevent lost/stale note events. Transition uses bounded release tails instead of clearing the entire synthesizer voice state for each key. No blocking I/O in audio callbacks.
- Song Chains: automatic source-pattern mapping is retained, but empty channels display `--` in first and added song rows. They appear automatically after an event is entered in their source pattern. Manual `--` remains silent. Saved song format unchanged.
- Preserves independent 13-slot user-sample prefetch lab (not wired into device), 12 project slots and firmware identity `FM-1_2123`. FM1 runtime behavior unverified.

## V1.4.9 — BD4S Sample Prefetch & Playback Laboratory (host only)

- Added original six-voice, 13-slot BD4S streaming laboratory (`beetle_bd4_prefetch.[ch]`). Decoder and storage callbacks run only in a bounded background poll; the 44.1-kHz audio render reads predecoded PCM windows and makes no storage requests.
- Uses two 64-frame decoded windows per voice and a fixed 2,816-byte host state; the FM1 RAM budget and ISR/main-loop handoff are not yet proven.
- Added six-voice PCM-exact host tests, a complete 20-second test sample, short-sample boundary tests, deliberate underrun/recovery and corrupted-header detection. Existing 10–20 second capacity/storage tests remain present.
- **No new FM1 firmware is released.** Physical flash ownership, transactional migration, realtime playback, sequencer lookahead and DMA stress tests remain open. The V1.4.8 `.fwsc` and WebInstaller are byte-identical to the original package. Identity `FM-1_2123` is unchanged.
- See [`docs/V149-PREFETCH-AUDIO-LAB.md`](docs/V149-PREFETCH-AUDIO-LAB.md).

## V1.4.8 — Original BD4S Codec & Storage Full Overlay (LAB)

- **Permanent compatibility identity:** retained exact `FM-1_2123` product ID from BEETLE V1.3.8, both native USB/SysEx and the `.fwsc`. The original V1.3.8 protected first 16 KiB and OTA loader are unchanged in the experimental package.
- Native FM1 project-save capacity refusal now gives a temporary topmost **STORAGE FULL / PROJECT NOT SAVED** popup on the existing error page. Handles full 12-slot NEW save and journal capacity/oversized projects. The write remains rejected, not retried or silently redirected. Project input guard during reading/writing is preserved.
- Project Manager capacity errors use the existing modal overlay, distinguishing its **PC-only 32 MiB `.beetlebank` cap** from experimental FM1 project storage. Rejected WAV replacements restore the old slot.
- Brand-new **BD4S**, independent original SikkenWorks adaptive signed-nibble 4-bit mono codec with 256-frame blocks, bounded RAM decoder, host WAV import/resampling, CRC-protected encoded file and preview WAV. Source: `tools/dpcm4/` and `firmware/fm1-native/beetle_dpcm4.[ch]`. NO other firmware sample code imported. Audio quality is lossy; no 100%-bitwise WAV fidelity claim for converted samples.
- End-to-end Python/C block decoding and integrity tests, plus actual 10s/20s BD4S payloads shared across U01–U13 in the offline transactional V2 flash simulator with twelve mixed-size projects. Capacity remains workload-dependent and not hardware proven.
- **No native sample playback, physical V2 storage, migration, USB sample transfer or hardware guarantee.** Existing FM1 legacy project path is unchanged. Verified builds do not prove absence of glitches or OTA safety.

## V1.4.5 — Playhead Ghost Fix (hardware-unverified)

## V1.4.7 — Global Samples: Project Storage V2 (offline prototype)

This is a **host-only engineering stage**, NOT new device firmware. Added an independently authored lossless C project packer with raw fallback, plus a NOR flash A/B metadata simulator with CRC-verified immutable records, 12 logical project slots, 13 sample slots, protected 4-sector reserve, power-cut fault injection and fail-closed capacity checking. Actual DPCM4 playback, real-device migration, USB transfer and safe flash ownership remain unimplemented. Existing V1.4.5 firmware and WebInstaller are byte-identical to the previous release. See `docs/GLOBAL-SAMPLES-STORAGE-V2-OFFLINE.md`.


- Fixed the V1.4.4 repaint synchronization regression reported in `IMG_2437.mp4`: the native main loop incorrectly recorded `transport.play_step` **after** a potentially long full LCD SPI transfer, not the step frame actually sent. When the audio ISR advanced meanwhile, old magenta step highlights could remain visible indefinitely.
- The full-frame UI now records `track_ui_step_snapshot` and `track_ui_song_pos_snapshot`, both latched before LCD rendering, and the next incremental update always repaints the real previously painted row.
- Song Chain highlight snapshots are also stable across both dirty-strip transfers; the grid FX source follows the song-position snapshot.
- Kept both 240x13 dirty-strip row updates and animated 22x23 play icon, the previous version's read/write button lock, audio/DMA fixes, and all existing project functionality. No synth DSP or project file format modifications.
- New tests reproduce an interrupt-driven step advance during full-frame LCD painting, then verify only the current step is highlighted for 64 loops, overlap, 16-step wrapping, and nonadjacent jumps; production source snapshot assignment is checked.
- Offline: native JieLi builds, unchanged PCM matrix and 120-second stress hash, original playhead geometry, and input-lock regressions. Firmware/installer SHA and protected loader checked. **No on-hardware LCD/DMA verification and no safe recovery guarantee.**

## V1.4.4 — Per-step Playhead LCD Fix (hardware-unverified)

- Fixed V1.4.3 regression: the 125–380 ms full-screen LCD throttle could skip three or more playhead steps at 156 BPM. The full-screen LCD refresh and the playhead movement no longer share the same throttle.
- Native Tracker now renders **two 240×13 pixel dirty bands** (previous and current step) plus a compact 22×23 pixel beetle animation on every observed step transition. Only the LCD transfer size and frequency change; no extra work is added to the audio ISR.
- Native Song Chains highlights update as two 240×15 bands while the position remains on the same 8-row page. A normal complete repaint is retained for page changes and source-pattern/content changes.
- The full UI raster path uses one latched step for both halves, preventing self-inconsistent two-half playhead frames if ALNK0 advances mid-SPI transfer.
- Partial rasterization skips non-intersecting tracker rows so each strip does not recalculate every value.
- The existing project save/load all-control input lock, DMA diagnostics, audio DSP, 12 on-device project slots, drum kit, samplebank prototype and USB implementation remain unchanged.
- Tests: JieLi readonly and USB/OTA builds, native 120-second 8-synth/6-drum stress regression, NUGGIE matrix, MOUNTAINKING 32 Song positions, new high-load playhead cadence/dirty-geometry host test, existing storage input-barrier test, FWSC CRC/header and installer integrity tests.
- **Software validation only. Physical FM1 LCD frame cadence, SPI contention and audio DMA under high load have not been measured. This does not prove the firmware is glitch-free or safe to flash without a verified recovery path.**

## V1.4.3 — MOUNTAINKING Stability Lab / Input Quarantine

- Added full-button, musical-key and encoder quarantine across project READ/VERIFY, WRITE/VERIFY and erase/provisioning operations. 320 ms of released/quiet controls required before returning to the editor.
- Added conditional audio-pressure LCD draw governor and null-FX DSP bypass without reducing synth polyphony or changing PCM matrix regression output.
- Added LAST LATE SONG POS to the RC AUDIO LOAD screen so the real hardware can identify positions associated with genuine DMA deadline misses.
- Host Song Mode playback analysis of MOUNTAINKING positions 1–32 (V20, 156 BPM), especially the 18→19 transition. No PCM clipping in the simplified host model; hardware crackling root cause remains to be confirmed.
- Native builds and FWSC package validated offline, firmware embedded/pinned in current WebInstaller. **Not hardware certified, no zero-underrun guarantee.**

## V1.4.3 — MOUNTAINKING Stability Lab / Input Quarantine

- Added full-button, musical-key and encoder quarantine across project READ/VERIFY, WRITE/VERIFY and erase/provisioning operations. 320 ms of released/quiet controls required before returning to the editor.
- Added conditional audio-pressure LCD draw governor and null-FX DSP bypass without reducing synth polyphony or changing PCM matrix regression output.
- Added LAST LATE SONG POS to the RC AUDIO LOAD screen so the real hardware can identify positions associated with genuine DMA deadline misses.
- Host Song Mode playback analysis of MOUNTAINKING positions 1–32 (V20, 156 BPM), especially the 18→19 transition. No PCM clipping in the simplified host model; hardware crackling root cause remains to be confirmed.
- Native builds and FWSC package validated offline, firmware embedded/pinned in current WebInstaller. **Not hardware certified, no zero-underrun guarantee.**

## V1.4.2 DMA Patch 2 — WebInstaller package fix

- Replaced the embedded **V1.3.3** firmware bytes (previously incorrectly distributed as current) with the verified *offline* V1.4.2 DMA Patch 2 `.fwsc`.
- Pin SHA-256 to `a82ddfd02c1f26ee1923fff59ecd864eef8b7cdd806a75f2aa0c403a9089b6aa`, check 610,086-byte length and exact `FM-1_2123` product identity. The protected 16 KiB header and OTA loader remain byte-identical to the original V1.4.1 reference.
- Both root `WebInstaller.html` and `web/WebInstaller.html` are byte-identical. Original USB-MIDI flow, checks, source-identity allowlist, restore/Firmware Freedom policies and all other app components retained. An on-device ID match is not a cryptographic post-flash verification because V1.4.1 shares the identity.
- Installer audit and negative tests verify rejected tampered packages, wrong SHA-256 and wrong product ID *before USB*. The host audio matrix and NUGGIE 64-loop tests remain part of the release.
- **Native firmware itself is unchanged from DMA Patch 2.** Host tests and successful JieLi compilation do **not** establish a durable fix for FM1 DMA DEADLINE; don't treat this candidate as a stable flash release.

# BEETLE V1.4.2 DMA Patch 2

Adds PCM-identical hot-path caching for lane vibrato selection, per-lane drive and gain; a 64-loop NUGGIE native host reproducer for 12 oscillators; both JieLi build variants and integrity-verified experimental FWSC. The global PC sample library is unchanged. Real FM1 DMA deadlines and recovery remain UNVERIFIED. Full change and limitations: `docs/DMA-V142-PATCH2-REPORT.md`.

---

# BEETLE V1.4.2 DMA LAB PATCH 1 (prototype, hardware unverified)

This candidate contains **native DSP performance corrections** tested against the NUGGIE V22 project; see `docs/DMA-V142-HOST-AUDIT.md` for exact test data, resource limits, precautions and remaining hardware acceptance requirements.

No physical FM1 deadline-free claim, no new global-bank device storage, and **no hardware-certified firmware release**. The desktop Global Sample Library is retained unchanged.

---

# BEETLE Changelog

## Global Sample Library — Stage 2 (PC tools / read-only native lab, not new FM1 firmware)

- Preserves the `.beetlebank` v1 binary format and original imported WAV bytes.
- Validates real RIFF/WAVE `fmt `/`data` chunks, chunk bounds and PCM/float audio metadata.
- Adds per-slot format/duration display, independent WAV export and cleaner preview lifecycle in the Project Manager.
- Adds standalone read-only C bank reader and cross-language compatibility/corruption tests.
- Keeps the V1.4.5 firmware, project storage, USB protocol and audio path unchanged. **No installable V1.4.6 firmware exists in this stage.**
- Deferred USB/DMA diagnostics recorded in `docs/DEFERRED-AUDIO-DIAGNOSTICS.md`.


## V1.4.1 — Stereo USB and RETRO PULSE tail (experimental)

- Clock drift correction for **stereo USB** moved out of 1ms packet assembly into the native DMA render boundary and filtered, minimizing packet-to-packet speed modulation. Six-stem USB algorithm unmodified.
- RETRO PULSE 4-bit DAC release amplitude crossfaded to continuous ADSR over 128 frames; held-note PCM remains bit-identical.
- Project Manager incorrect **0.37s** tooltip fixed to correct **0.28s** shared for all 13 one-shot keys (6,144 bytes). No unsupported sample storage extension.
- Confirmed safe firmware switching via compatible-package audit or independent UBOOT loader; no universal flash guarantees.
- Full and read-only JieLi builds, original protected boot and OTA assets, software USB clock model and sample-codec regression verified. No physical FM1 verification.


## V1.4.0 — Pre-reSID User One-Shots (EXPERIMENTAL)

- Based on V1.3.9, with original 14 drum mappings untouched. CH6 NOTE1 extends to 27 notes; keys 14..26 access thirteen user slots.
- Native fixed 6,144-byte project-owned companded PCM sample bank in preallocated RAM; no change to protected flash partitions; V23 optional sample suffix; V22 retained for sample-free projects.
- Project Manager WAV import, sample naming and key assignment, read/write and export of encrypted .beetle files, guarded USB project transfer, explicit sample-free V22 downgrade export.
- Fixed native project encoder's unsafe `memcmp` over unused packed-bitfield padding; deterministic semantic-step equality replaces it.
- Preserved stereo/USB six-stem clock path, oscillator volume calibration, full pre-reSID SID-lite, sustain display `1.00`, original credits and licenses.
- Package/host tests: see V1.4.0-VALIDATION.md. Physical FM1 tests NOT performed.

## V1.3.9 — USB multitrack improvements

- Bounded filtered USB clock correction and equivalent 32-bit six-stem interpolation; pre-reSID engine unchanged. Physical FM1 recording not validated.

## V1.3.8 — Pre-reSID baseline

- Last original firmware prior to native reSID and ymfm development; six tracker lanes, twelve oscillator types including SID-lite, fourteen factory drum sounds, and original native project storage.

For earlier release notes, consult the previous archived releases and Git history. The original authorship notices and licenses remain available in LICENSE, LICENSES and docs.

---

# Historical complete release notes (through V1.3.3)

# BEETLE Changelog

## V1.4.1 — Stereo USB and RETRO PULSE tail (experimental)

- Clock drift correction for **stereo USB** moved out of 1ms packet assembly into the native DMA render boundary and filtered, minimizing packet-to-packet speed modulation. Six-stem USB algorithm unmodified.
- RETRO PULSE 4-bit DAC release amplitude crossfaded to continuous ADSR over 128 frames; held-note PCM remains bit-identical.
- Project Manager incorrect **0.37s** tooltip fixed to correct **0.28s** shared for all 13 one-shot keys (6,144 bytes). No unsupported sample storage extension.
- Confirmed safe firmware switching via compatible-package audit or independent UBOOT loader; no universal flash guarantees.
- Full and read-only JieLi builds, original protected boot and OTA assets, software USB clock model and sample-codec regression verified. No physical FM1 verification.


## V1.4.0 — Pre-reSID User One-Shots (EXPERIMENTAL)

- Based on V1.3.9, with original 14 drum mappings untouched. CH6 NOTE1 extends to 27 notes; keys 14..26 access thirteen user slots.
- Native fixed 6,144-byte project-owned companded PCM sample bank in preallocated RAM; no change to protected flash partitions; V23 optional sample suffix; V22 retained for sample-free projects.
- Project Manager WAV import, sample naming and key assignment, read/write and export of encrypted .beetle files, guarded USB project transfer, explicit sample-free V22 downgrade export.
- Fixed native project encoder's unsafe `memcmp` over unused packed-bitfield padding; deterministic semantic-step equality replaces it.
- Preserved stereo/USB six-stem clock path, oscillator volume calibration, full pre-reSID SID-lite, sustain display `1.00`, original credits and licenses.
- Package/host tests: see V1.4.0-VALIDATION.md. Physical FM1 tests NOT performed.

## V1.3.9 — USB multitrack improvements

- Bounded filtered USB clock correction and equivalent 32-bit six-stem interpolation; pre-reSID engine unchanged. Physical FM1 recording not validated.

## V1.3.8 — Pre-reSID baseline

- Last original firmware prior to native reSID and ymfm development; six tracker lanes, twelve oscillator types including SID-lite, fourteen factory drum sounds, and original native project storage.

For earlier release notes, consult the previous archived releases and Git history. The original authorship notices and licenses remain available in LICENSE, LICENSES and docs.

## V1.4.18 — Native Storage V2 reader and migration-image preparation (source-only)

- Added original, bounded C reader for authentic V2 flash indexes, persistent twelve project references, and U01–U13 sample records. Compiles on JieLi target and passes host flash-image tests.
- Added strict offline full-flash backup → V2 image generation with source-project matching, sample integrity, CRC readback and protected-region byte equality.
- Added tests of corrupted sample records, multichunk samples, torn index recovery, and V2 migration from synthetic V1 project A/B banks.
- **No new `.fwsc`**; pre-existing firmware remains V1.4.14. Physical migration, write support and user-sample audio playback are still unimplemented.
