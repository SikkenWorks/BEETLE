<div align="center">
  <a href="https://sikkenworks.github.io/BEETLE/WebInstaller.html">
    <img
      src="https://img.shields.io/badge/▲_INSTALL-BEETLE_(CHROME%2FEDGE)-000000?style=for-the-badge&labelColor=000000&logoColor=white"
      alt="Install BEETLE (Chrome/Edge)"
      width="450"
    />
  </a>

  <a href="https://sikkenworks.github.io/BEETLE/simulator.html">
    <img
      src="https://img.shields.io/badge/▶_LAUNCH-BEETLE_WEB_SIMULATOR-000000?style=for-the-badge&labelColor=000000&logoColor=white"
      alt="Launch BEETLE Web Simulator"
      width="450"
    />
  </a>
</div>

<p align="center">
  <img src="BEETLE.png" alt="BEETLE" width="400">
</p>

**An open-source, six-lane tracker for the M-VAVE FM1.**  
**SikkenWorks — Born out of passion.**

**Current documented release: V0.63** · [Release history and changes](CHANGELOG.md)

BEETLE turns the FM1 into a small, hands-on music workstation. Its native firmware combines **five synth tracks sharing eight synthesizer voices**, **one two-note-per-step drum track**, a 16-step tracker with **48 patterns**, **12 project slots**, real-time Punch-In effects, and song sequencing. It is intended to be played and programmed on the device, using the physical keyboard, rotary encoders, and dedicated buttons.

> **Release status: V0.63 experimental, not hardware-certified.** Builds, simulation tests and firmware-header checks are **not** proof of correct operation on an actual FM1. The storage-enabled firmware can erase existing user data, and recovery from a failed update is not guaranteed.

**V0.63:** STEP GROOVE adds sample-clock microtiming **0–7/64 step late**, 1–4 ratchets, and 100/75/50/25% step probability. Open with **SEQ+ENV**, use K1 to pick a step, K2 a parameter, K3 to adjust, and SEL to reset. **Drum NOTE2** now supports physical PRESET switching between the selected drum sample, `[---]` and `[STP]`. The new V12 project schema reads older V11 and pre-V11 projects without altering their original timing. Native firmware and WebSimulator are included. See [CHANGELOG.md](CHANGELOG.md) for full details and limitations.

**V0.62:** Fixed a cursor bug: K1 now stops at the final active row of a shortened pattern (e.g. an 8-step pattern only allows steps 00–07). DYNAMICS and Block Editor obey the same limit, and focus is corrected after switching patterns, STOP, Undo/Redo, and LOAD. Hidden step data are preserved when shortening and later extending a pattern. V0.61 previously fixed SEQ+K4 and made HELP K1-only.

**V0.60:** Sequencer 2.0 adds 1–16 step lengths (SEQ+K4 when stopped), quantized manual pattern changes (ALGORITHM while playing), K4 chapter navigation in HELP (removed in V0.61) and protection against stale SEL presses after project LOAD. V0.59 added per-step velocity and gate with a new STEP DYNAMICS screen (SEQ+FX). Project format V11 introduced these values; V12 additionally stores groove settings; V10 and earlier supported projects remain readable. The V0.58 moon-and-tree loading animation is retained. See [CHANGELOG.md](CHANGELOG.md).

## Current workflow

- **Sequencer 2.0 (V0.62):** K1 cannot move past the current pattern length. Stop transport, hold SEQ and turn K4 to adjust current pattern length (1–16), or queue a pattern while playing with ALGORITHM. A queued change starts on the next pattern boundary, not immediately. Song Mode continues to control song sequencing. In the WebSimulator use PAT LEN −/+, hold SEQ while turning K4, or Shift+click a K4 button.
- **Project LOAD safety:** SEL pressed during project reading will not clear a step upon return to the tracker. Wait until the load finishes and release the key before editing.

- **Step Dynamics (V0.59):** `SEQ + FX` from the tracker opens velocity/gate editing. K1 selects step, K2 selects velocity or gate, K3 edits, SEL resets; SEQ/HOME returns to tracker. Default gate remains legato for older projects.

- **WebSimulator:** six-channel MIXER and 21 240×240 interface screens, with ARP for SONG MODE, GLO for MIXER, SEQ for TRACKER.
- **PROJECTS → HELP:** scrollable on-device controls; K1 scrolls by one entry; K4 does nothing in HELP, HOME returns to PROJECTS and SEQ to the tracker. Includes the V0.57 editing shortcuts.
- **MASTER FX:** Compressor and Reverb show four circular pixel knobs controlled by K1–K4, with the existing DSP preserved.
- **Editing:** V0.57 adds full-pattern copy/paste, six-level RAM-only Undo/Redo and a one-lane block editor; see below and [CHANGELOG.md](CHANGELOG.md).

## What you can make

- Write independent 1–16-step patterns for **CH1–CH5 (synth)** and **CH6 (digital drums)**; chain them in Song Mode.
- Play up to **eight shared synth voices**. The drum synthesizer has its own voice pool; this is not a promise that every voice combination can run without glitches on physical hardware.
- Use the **14-component, sample-ROM digital drum kit**: kick, snare, closed/open hats, three toms, crash, ride, clap, three percussion sounds and rim. Voices have individual editable pitch (−36 to +36 semitones), attack, decay, sustain, tail/release, three-band tonal controls and level.
- Use shared synth presets, VA oscillators, ADSR, detune, oscillator blending, drive, sub oscillator, noise and FM depth. EDIT offers nine instrument TYPE options, including five SID-inspired flavors (`SID 6581`, `SID 8580`, `SID DIRTY`, `SID BASS`, `SID NOISE`). Since V0.53 the 6581 and 8580 variants have separate filter and saturation character. These are not the original reSID engine.
- Adjust **volume, pan, mute, solo and swing (0–100)** for each lane. The drum channel defaults to mixer volume 80, with increased internal drum source gain.
- Add tracker step effects, including **volume and drum-pitch automation**, plus **velocity/gate per step (V0.59)**; use **[STP]** to stop sustained notes.
- Apply eight live Punch-In commands: **REPEAT, REVERSE, FILTER, TAPE STOP, STUTTER, OCTAVE UP, OCTAVE DOWN and MUTE**. For effects 1–5, hold the relevant key and turn ALGORITHM to edit that effect's parameter.
- Open a separate MASTER screen with a four-band graphical EQ, compressor, reverb and reorderable processing blocks. V0.52 adds compressor Attack and reverb Width, with four pixel-art knobs controlled independently by K1–K4. Hold SEQ and turn K4 to reorder the processing chain. These are FM1-constrained implementations, not bit-identical copies of desktop effects.

## Professional editing (V0.57)

In the tracker while transport is stopped, hold **SEQ** and press the physical black OP keys: **OP1** copies a lane, **OP2** pastes a lane, **OP3** copies the entire six-lane pattern plus recorded Punch-In FX, **OP4** pastes it, **OP5** Undo, **OP6** Redo. **SEQ + SEL** opens Block Editing (K4 start, K1 end, K2 action, K3 amount, SEL apply; HOME/SEQ exit). Native Undo/Redo stores six edit actions in RAM, not on flash. A new edit after Undo discards the redo branch. The WebSimulator provides explicit buttons for the same actions. The Block Editor uses the same black/white/magenta 1-bit BEETLE styling as the tracker.

## Controls and workflow

| Where | Control | Function |
| --- | --- | --- |
| Tracker | **SELECT rotary** | Select CH1–CH6. Scrolling left from CH1 opens the recorded Punch-In-event lane; scrolling right past CH6 stops. |
| Tracker | **ALGORITHM rotary** | Choose from the 48 patterns when no Punch-In key is held. |
| Tracker | **K1 / K2 / K3** | Select step, select cell/column, change selected value respectively. |
| Tracker | **SEL button** | Clear the currently selected step. |
| Tracker | **PRESET rotary** | Cycle the selected note cell through its note, **[---]** (empty), and **[STP]**. |
| Tracker | **ENV** | Insert **[STP]** directly. |
| Tracker | **OCT− / OCT+** | With the cursor on an instrument cell, decrease/increase the instrument number (0–3). Otherwise use the normal synth octave controls. |
| Tracker | **SEQ + OP1** | Copy the current pattern's entire selected lane. |
| Tracker | **SEQ + OP2** | Paste that lane, including notes, instruments and step FX, into the current lane/pattern. The clipboard survives changing lanes/patterns. |
| Mixer | **GLO** | Open/close the mixer. Six black OP keys can mute CH1–CH6; their LEDs indicate mute. |
| Instrument Edit | **EDIT** | Open instrument parameters; on the next EDIT page access the drum component editor. |
| Song | **ARP** | Open Song Mode (not GLO double-click). |
| Master | **LFO** | Open the master EQ/compressor/reverb chain. |
| Punch-In | **REC / Punch-In screen** | Hold a Punch-In key for an effect; ALGORITHM adjusts that effect's stored setting. |
| Projects | **SAVE / LOAD menus** | Work with 12 slots. A project stores patterns, instruments, mixer, drums, MASTER settings, song data and five Punch-In ALGORITHM parameters. |

**Keyboard octave:** TRACKER and EDIT share the same octave range (−4 to +3). OCT−/OCT+ shift the live keyboard octave without transposing existing pattern notes. In EDIT, pressing both octave buttons together toggles MONO/POLY without shifting octave.

**TYPE selection in EDIT:** the encoders step through all nine instrument types with wraparound.

Some controls are context-sensitive. Screen labels and FM1 button names take precedence over this overview.

### Punch-In FX

- **REPEAT:** freezes and loops a recent audio snapshot. ALGORITHM changes its length.
- **REVERSE:** freezes a snapshot and actually reads the recorded PCM backward. The 12 KiB capture ring uses half-rate stereo storage with interpolation, giving up to approximately **128 ms** of history. Very long reversed musical phrases are not possible within this buffer.
- **FILTER:** opens at setting 0 by default; the ALGORITHM setting is retained.
- **TAPE STOP:** progressively slows sample read speed and lowers pitch.
- **STUTTER:** gates the current audio alternately on and off, with ALGORITHM-controlled speed.

Each effect retains its own ALGORITHM value when released. SAVE/LOAD stores all five adjustable Punch-In settings; supported older projects open with default values where fields are missing.

### Standard digital drum kit

BEETLE boots with a **preconfigured digital kit**, not fourteen identical generic parameter sets. Kick emphasizes low body; snare uses a brighter wire texture; closed hats have shorter tails than open hats; toms have separate tuning; crash and ride have different envelopes; clap and rim are individually shaped. You can change each component in Drum Edit and save those changes with a project. The sounds are generated in software and must still be judged through the real FM1 output.

## Projects, compatibility and storage safety

- **12 project slots**, with journalled A/B storage where the experimental storage backend is enabled.
- **Current project schema: V11.** Newly saved projects include per-step velocity and gate as well as the earlier MASTER parameters (compressor Attack and reverb Width). Supported older projects, including V10, V9 and earlier formats, remain readable; missing settings are initialized to defaults and old project data are not silently rewritten. V8 previously added the five Punch-In values; V6/V7/V8 compatibility was included in earlier regression tests.
- The physical project record has a **finite capacity**. A fully populated project may not fit. An oversized project is rejected *before* any flash erase; do not assume all 48 six-lane patterns can be filled to capacity simultaneously.
- **WARNING:** the storage-enabled build requires explicit first-use setup. The experimental storage region can overwrite legacy user data at addresses **`0x97000–0xDFFFF`**. Never initialize it without a verified full-flash backup and an independently working recovery path. Project writes are *disabled* in the pilot build.
- The WebInstaller does not make a complete backup. Even a verified OTA transfer does not guarantee that the new firmware will boot or that USB recovery will remain available.

## Build and test

The target is the M-VAVE FM1 JieLi **pi32v2** environment. The JieLi cross-toolchain is a separate prerequisite; **the toolchain is not bundled**.

```bash
export JIELI_TOOLCHAIN=/path/to/jieli-linux-toolchain
cd source
python firmware/fm1-native/build.py --hardware-test
python firmware/fm1-native/build.py --pilot
# Destructive storage-capable variant, experimental only:
python firmware/fm1-native/build.py --beetle-storage-lab --updater
bash tests/track/run_v041.sh
```

Use a known-good version of Python, a C compiler with ASan/UBSan support for host tests, and the JieLi toolchain for the target. Source releases include an audited baseline of the hardware layer and package-integrity test scripts. **None of these tests substitute for physical device validation.**

The documented release contains a standalone WebInstaller and `.fwsc` package; use a desktop browser with Web MIDI, a direct data cable, and the correct FM1 device identity. **Never bypass the installer identity or hash checks.**

## Open-source origins, credits and licences

BEETLE is **not a clean-room firmware**. Parts of its low-level architecture and update pathway came from earlier open-source projects. These contributions are important and must remain credited:

1. **Felucca**, by **Leo Kuroshita (@kurogedelic), Hügelton Instruments** — original M-VAVE FM1 hardware abstraction, startup and linker foundations, LCD, input scanning, audio and boot/OTA infrastructure used or carried forward in the ETCH/BEETLE native tree. [Felucca](https://github.com/hugelton/Felucca) is distributed under **GPL-3.0-only**. BEETLE includes a recorded SHA-256 audit for 19 preserved critical Felucca/HAL/bootstrap files in `source/reference/felucca-hal-sha256.json`; this proves equivalence only for the enumerated files, **not** that the entire application is unchanged upstream code.
2. **ETCH**, by **SikkenWorks** — the prior SikkenWorks FM1 firmware and working **ETCH V1.37 firmware container/bootloader reference** on which BEETLE's device startup and `.fwsc` packaging workflow were based. Some `native_*`, flash/OTA, UI and shared hardware support modules retain ETCH lineage; the tracker, new digital drum instruments and project workflow are BEETLE-specific additions. The protected bootloader is intentionally kept unchanged for compatibility; the loader may still display older branding. [ETCH](https://github.com/SikkenWorks/ETCH).
3. **JieLi Technology** — manufacturer of the AC79-series chip, target architecture, and tooling used to compile and package firmware. The released container inherits necessary device/SDK-format material from the verified reference package. Refer to original JieLi licensing terms and the upstream Felucca licence notices; third-party toolchain binaries are not part of this repository.
4. **reSID**, originally developed by **Dag Lem** — acknowledged for its historical and technical contribution to SID chip emulation and as an inspiration for BEETLE's SID-style instruments. **Not included as a compiled or linked dependency.** [reSID repository](https://github.com/libsidplayfp/resid). See `source/CREDITS-AND-LICENSES.md` for further detail.

Felucca's own attribution document, preserved at **`source/LICENSES/FELUCCA-LICENSING.txt`**, identifies additional third-party projects in the broader upstream repository: **DaisySP** (Electrosmith / Emilie Gillet, MIT), **Rings** (Emilie Gillet, MIT), **msfa/Dexed** (Google / Pascal Gauthier, Apache-2.0), **CrispyZebra**, **Fukiai**, **Inter Tight**, **DotGothic16**, **Versilian Studios VSCO-2**, **klattsch**, and **X0X**. Those are upstream acknowledgments: **this BEETLE target build does not claim to embed all of those engines, fonts, or audio assets**. See Felucca's corresponding source and notices for the actual upstream file-level relationships.

The enlarged two-frame pink loading artwork is derived from an image **provided by the BEETLE project user**. The firmware draws it in nearest-neighbour pixel scaling without an external icon-font dependency.

### reSID credits and acknowledgment

**reSID — original author: Dag Lem.** reSID is a reverse-engineered emulation of the MOS6581 SID (Sound Interface Device), developed for Commodore 64 audio. See the original project: **[libsidplayfp/reSID](https://github.com/libsidplayfp/resid)** (a mirror of VICE's reSID), including its `AUTHORS` and licensing notices.

BEETLE acknowledges **Dag Lem and the reSID project** as an influence on its SID-inspired instrument design. BEETLE includes five SID-style instruments (`SID 6581`, `SID 8580`, `SID DIRTY`, `SID BASS`, and `SID NOISE`), implemented in BEETLE's separate lightweight synth engine. **BEETLE does not bundle, compile, link, or run the original reSID C++ source code.** It is not cycle-accurate reSID emulation.

The full attribution and distinction between incorporated code and inspirations are documented in [`source/CREDITS-AND-LICENSES.md`](source/CREDITS-AND-LICENSES.md).

### Source licence and distribution obligations

The included native firmware and derivatives are distributed under **GNU GPL v3 only (`GPL-3.0-only`)**, preserving the licence of Felucca-derived source. The full GPL text is at `source/LICENSE`. If you distribute a modified BEETLE firmware, provide its complete corresponding source and all applicable copyright notices, including relevant Felucca notices. The presence of a working binary, WebInstaller or bootloader does **not** by itself satisfy source-offer requirements.

See `source/LICENSES/FELUCCA-LICENSING.txt` and copyright/SPDX headers in individual source files. For code with unclear lineage, retain the upstream notices rather than claiming exclusive SikkenWorks authorship.

## Known limitations

- Native reverse is limited to a short buffer (~128 ms); this is not long-form offline sample reversal.
- PCM interpolation, the small target CPU, memory footprint and analogue FM1 output affect perceived quality.
- The upstream/protected **OTA loader** has not been changed to force the pink BEETLE Matrix screen; it may still show ETCH branding during installation.
- A full-screen redraw still requires two 120-row SPI transfers. Previous splash optimizations reduced unnecessary transfers and idle repaints, but smooth navigation and animation still require hardware measurement.
- Flash ownership/recovery and sustained 8-voice + drum + MASTER FX load **remain experimental**.

## Support

If you find a bug, include the firmware version, what you pressed, your exact project state, and (when safe) a diagnostic log. Do not share a flash image containing private material publicly. Contributions that preserve realtime audio safety, backward compatibility and upstream copyright attribution are welcome.

**BEETLE — SikkenWorks. Born out of passion.**
