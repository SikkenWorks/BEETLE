<p align="center">
  <img src="BEETLE.png" alt="BEETLE" width="320">
</p>

<h1 align="center">BEETLE</h1>
<p align="center">
  <strong>An open-source six-lane tracker and digital drum machine for the M-VAVE FM1.</strong><br>
  SikkenWorks · Born out of passion.
</p>

<p align="center">
  <a href="https://sikkenworks.github.io/BEETLE/WebInstaller.html"><img src="https://img.shields.io/badge/▲_INSTALL_BEETLE-000000?style=for-the-badge" alt="Install BEETLE"></a>
  <a href="https://sikkenworks.github.io/BEETLE/simulator.html"><img src="https://img.shields.io/badge/▶_WEB_SIMULATOR-000000?style=for-the-badge" alt="Open BEETLE Web Simulator"></a>
  <a href="https://sikkenworks.github.io/BEETLE/BEETLE-Project-Manager.html"><img src="https://img.shields.io/badge/▶_PROJECT_MANAGER-000000?style=for-the-badge" alt="Open BEETLE Project Manager"></a>
</p>

<p align="center">
  <sub>Development release: <strong>RC1.1</strong> · <a href="CHANGELOG.md">Release history</a></sub>
</p>

---

**RC1.1 — Chapter HELP + USB-C audio capture:** HELP now has twelve chapters, selected using **K2**, and each chapter scrolls independently with **K1**. BEETLE RC1.1's experimental firmware additionally exposes a class-compliant **stereo 16-bit / 44.1 kHz USB Audio Class 1 capture input** to your computer over USB-C, alongside analog audio output and USB MIDI. The supplied `.fwsc` is compiled with `--usb-audio`; previous firmware releases do **not** gain the feature by updating the WebInstaller alone. Neither USB audio performance nor OTA/recovery has yet been verified on a physical FM1. **Back up and verify a recovery route before installing.**

**RC1.0.11b — Song Chains audio-stability hotfix:** reduces per-voice mixer work and song-transition cost; the BEETLE synth voice budget now responds to repeated DMA deadline pressure instead of incorrectly limiting unused ETCH voices. Any audio-stop recovery screen now uses BEETLE branding. No changes to saved project format (V19), Song Chains, or tracker controls. Software-tested; physical FM1 endurance testing remains pending.

## What is BEETLE?

BEETLE turns the **M-VAVE FM1** into a standalone, hands-on music workstation. Compose patterns, play the built-in keyboard, design instruments, program drums, arrange songs and perform with live effects, all using the FM1's own controls.

| | |
| :-- | :-- |
| **6 channels** | CH1–CH5: synthesizers · CH6: digital drums with two hits per step |
| **48 patterns** | 1–16 steps per pattern, with independent channel content |
| **Song Chains** | 64 song positions with a separate pattern source for each channel |
| **Sound engine** | Up to eight shared synth voices, separate drum voices, 14 drum instruments |
| **Sound design** | VA, SID-inspired and NES-inspired instruments, patch editing and an eight-slot project instrument library |
| **Performance** | Mixer, mute/solo, Punch-In FX, snapshots and mute groups |
| **Projects** | 12 project slots, plus browser-based backup and project tools |

## Where to Start

You can learn the workflow in a few minutes using the [Web Simulator](https://sikkenworks.github.io/BEETLE/simulator.html), or follow the same controls on an FM1 running BEETLE.

1. **Open the tracker.** Press **SEQ**. This is your main editing screen.
2. **Choose a channel.** Turn **SELECT**: CH1–CH5 are synth lanes; CH6 is the drum lane.
3. **Pick a pattern.** Turn **ALGORITHM** to choose one of the 48 patterns while stopped.
4. **Move around the grid.** Turn **K1** for the step (row) and **K2** for the field (column). Turn **K3** to change the selected value.
5. **Enter a sound.** Press a musical key to write a note or drum hit. The tracker normally advances after the main note is entered. On CH6, selecting **NOTE2** with K2 lets you enter a second drum hit without overwriting NOTE1 or advancing the step.
6. **Correct mistakes.** **SEL** clears the selected step. **ENV** inserts a note stop (`[STP]`) in the focused note field; it is *not* the same as clearing the entire step. Use **SEQ + OP5** to undo an edit.
7. **Play it back.** Press **PLAY/STOP**. Change the tempo with **K4**. Stop playback before making structural edits.
8. **Build the sound and arrangement.** Press **EDIT** for instruments, **GLO** for the mixer, **LFO** for master effects, or **ARP** for Song Chains. Press **SEQ** to return to the tracker.
9. **Save your work.** Press **SAVE**, or open **HOME → SAVE / SAVE AS** to manage a project slot.

> **FM1 terminology:** **SEL** means the physical SELECT push-button; **SELECT** in this guide means the *rotary encoder*. **OP1–OP6** are the six mapped black musical keys used with **SEQ**. The function of some controls changes with the current screen.

## HELP chapters (RC1.1)

Press **HOME → HELP**. Turn **K2** to browse the twelve topics. Turn **K1** to scroll *inside the selected chapter*. The scroll position resets at each chapter change, and the bottom counter refers to the current chapter, not the whole manual. **HOME** returns to Projects and **SEQ** returns to Tracker. HELP remains read-only; turning the other encoders will not edit your music.

Chapters: Tracker Basics, Pattern Editing, Step Dynamics / Groove, Instrument / Keyboard, Projects, Live FX / Record, Song Chains, Live Mixer / Solo, Mixer / Performance, Master FX, Guide / Navigation and USB-C Audio.

## USB-C stereo audio capture (RC1.1, experimental)

Connect the FM1 with a **USB-C data cable**. On Windows, open **Settings → System → Sound → Input** or the audio input selector in a recording application and choose **BEETLE Audio** (device naming depends on the operating system). Set the recording format to **44,100 Hz, stereo, 16-bit** when available. Start playback on the FM1 and recording on the computer. The computer initiates the USB audio stream; there is no extra on-device enable switch. The stream taps BEETLE's final master stereo signal, including the master volume, and leaves the analog output and USB-MIDI path present.

USB-C audio is **audio from FM1 → computer**, not computer → FM1. It is not USB screen capture, audio-file transfer, USB mass storage, or multitrack individual stems. The WebSimulator produces local browser audio and **does not act as a physical USB audio device**.

The USB audio endpoint and the associated ISR were already present as an optional Felucca-derived implementation, but had been disabled in the provided RC1.0.11b firmware. RC1.1 enables this existing UAC1 capture path in the distributed native build with `--usb-audio` (44.1 kHz only). This is a **software-/linker-validated experimental feature**, not proof that Windows/macOS/Linux enumerates it on a real FM1 or that it runs glitch-free with all six lanes and MASTER FX. Check for a BEETLE Audio input after installing; if the device fails to enumerate, do not assume an OS-level driver change will fix the firmware. See [`docs/RC1.1-HELP-USB-AUDIO-VALIDATION.md`](docs/RC1.1-HELP-USB-AUDIO-VALIDATION.md).

Build the distributed USB-audio configuration with:

```bash
export JIELI_TOOLCHAIN=/path/to/jieli-linux-toolchain
python firmware/fm1-native/build.py --beetle-storage-lab --updater --usb-audio
```

The `.fwsc` includes experimental storage/OTA code as in the RC1.0.11b base; installing it remains risky. A separate recovery-tested stock firmware image has not been produced here.

## FM1 Controls

### Buttons

Every dedicated button is listed here with its role in BEETLE.

| FM1 button | What it does in BEETLE |
| :-- | :-- |
| **FX** | Opens **Per-Step FX** for the selected step. Press again to reach **Punch-In FX**. In the Performance screen, opens **Mute Groups**. |
| **SEL** | In the tracker, **clears the selected step** on release. Elsewhere it confirms a choice, toggles an option or applies the screen's highlighted action. Hold it and rotate K1 in the tracker to select a range. |
| **ENV** | **Stops the selected note** by writing `[STP]` in the focused NOTE or drum NOTE2 field. This is a note-stop command, not full step deletion. |
| **LFO** | Opens **MASTER FX**: EQ, compressor and reverb. |
| **EDIT** | Opens the **Instrument Editor**. Press again to reach the drum component editor where applicable. |
| **GLO** | Opens or closes the regular **Mixer**. |
| **HOME** | Opens **Projects** for New, Save, Load, Help and related tools. **Hold HOME** to open the Scale Guide. |
| **SAVE** | Opens the project save workflow or saves the current named project. |
| **ARP** | Opens **Song Chains**. When SONG is enabled, pressing ARP from Song Chains opens the **Live Mixer**; pressing again returns to Song Chains. |
| **SEQ** | Returns to the **Tracker** and acts as the modifier for editing shortcuts. |
| **PLAY/STOP** | Starts or stops playback. When SONG is enabled, playback follows the Song Chain. |
| **REC** | Arms or disarms **Punch-In recording**. During playback, held Punch-In keys can be captured as performance automation. |
| **OCT− / OCT+** | Changes the live keyboard octave (−4 to +3) without changing previously saved notes. On Song Chains, removes/adds a song position. Other screens may assign contextual actions. |

### Rotary encoders

| Encoder | Primary function | Other important uses |
| :-- | :-- | :-- |
| **SELECT** | Choose CH1–CH6 in the Tracker. Turning left from CH1 reaches the recorded Punch-In event lane. | Choose MASTER FX blocks or drum components on their respective screens. |
| **ALGORITHM** | Select patterns **01–48** in the Tracker. | **SEQ + ALGORITHM** shifts the selected lane's steps up/down while stopped. |
| **PRESET** | Cycle a focused NOTE through **note / `[---]` (empty) / `[STP]` (stop)**. | On CH6 NOTE2, cycles its second-hit value, empty and stop states. |
| **K1** | Move vertically through tracker steps. | Select rows/items; adjust an active Punch-In effect parameter **while its key is held**. |
| **K2** | Move horizontally between tracker fields. | Select columns, parameters and Song Chain sources. |
| **K3** | Change the focused tracker value. | Edit the selected parameter in most BEETLE editors. |
| **K4** | Change tempo (BPM). | **SEQ + K4** sets pattern length while stopped; contextually selects project slots or adjusts a MASTER FX parameter. |

**Punch-In parameter note:** On **1.01 RC1.0.11**, the native firmware uses **K1**, not ALGORITHM, to adjust an effect while its Punch-In key is held. ALGORITHM is the pattern encoder in the tracker.

### Musical keys

| Where | What the keyboard does |
| :-- | :-- |
| **Tracker / CH1–CH5** | Enter synth notes. Pressing a key writes to NOTE even if another tracker column has focus; existing instrument/chord/FX fields are preserved. |
| **Tracker / CH6** | Enter a drum in NOTE1, or a separate second hit when **NOTE2** is focused. |
| **Instrument Editor** | Audition the selected synth patch without adding tracker notes. |
| **Drum Editor** | Audition the selected drum component. |
| **Mixer / Live Mixer** | The six assigned **black keys mute CH1–CH6**; their six corresponding **white keys solo CH1–CH6**. LEDs reflect active mute/solo states. |
| **Punch-In FX / REC armed** | Eight assigned white keys, from left to right, hold the eight performance effects. Release the key to end the held effect. |

### Editing shortcuts

These combinations apply in the **Tracker** unless another screen is specified. Most destructive editing requires playback to be stopped.

| Hold / press | Result |
| :-- | :-- |
| **SEQ + OP1** | Copy the current lane, or the selected step range if a range is active. |
| **SEQ + OP2** | Paste the copied lane or range at the appropriate destination. |
| **SEQ + OP3** | Copy the complete six-channel pattern and recorded Punch-In events. |
| **SEQ + OP4** | Paste the complete pattern and Punch-In events. |
| **SEQ + OP5 / OP6** | Undo / Redo. The available undo history lives in RAM. |
| **Hold SEL + turn K1** | Select an inclusive range of tracker steps for range copying. |
| **SEQ + SEL** | Open **Block Editing** (Tracker), or **Performance Snapshots** (Mixer). |
| **SEQ + FX** | Open **Step Dynamics** (velocity and gate). |
| **SEQ + ENV** | Open **Step Groove** (microtiming, ratchets and probability). |
| **SEQ + OCT− / OCT+** | Transpose the saved synth note at the cursor down/up one octave, with preview. |
| **SEQ + K4** | Change the current pattern length (1–16 steps) while stopped. |
| **SEQ + ALGORITHM** | Rotate the selected channel's step data upward/downward while stopped. |
| **SEQ + EDIT** | Open the eight-slot **Instrument Library** from the Instrument Editor. |
| **SEQ + rightmost black key (F#5)** | Open **CLEAR CURRENT LANE**. K1 selects YES/NO, SEL confirms; HOME cancels. Erases only the current channel in the current pattern; supports Undo. |
| **OCT− + OCT+ in EDIT** | Toggle instrument **MONO/POLY** without changing keyboard octave. |

## The Main Screens

### Tracker

Each pattern holds up to 16 steps for each of six lanes. The highlighted cell is edited with K1/K2/K3; the keyboard enters notes. `[---]` means no new note, while `[STP]` ends a sounding note. **CH6** has independent NOTE1 and NOTE2 drum hits and their own step FX. Use **FX** for effects and **SEQ + FX / ENV** for step dynamics and groove.

### Instrument Editor and Library

Press **EDIT** to adjust synth types, oscillators, envelopes, filters and patch settings. BEETLE includes VA, SID-inspired and NES-inspired sound types. Instrument settings are independent across patch slots. The **Instrument Library** (SEQ + EDIT) keeps eight named patches *inside the current project*; its STORE, LOAD, COPY and RENAME actions use K1 for slot, K2 for action, K3 for active instrument, K4 for copy destination, and SEL to execute. Save the project to keep library changes.

### Mixer and Live Mixer

Press **GLO** for the regular six-lane mixer: volume (0–120%), pan, mute, solo and swing. K1 selects a lane, K2 selects its parameter and K3 edits it. **SEL** toggles quantized mutes. With quantization enabled, mute changes during playback take effect at the next pattern boundary; otherwise they are immediate. New projects start with synth lane volume 50%, drum lane volume 80% and regular-mixer quantized mutes OFF.

The **Live Mixer** is separate and only available when **SONG is ON**. Its mute/solo changes are temporary performance overrides, not edits to the persistent mixer. **OCT−** toggles live quantized mute and **OCT+** toggles live quantized solo independently; both default ON. Its mapped black keys mute lanes and the matching white keys solo them.

From the regular Mixer, **SEQ + SEL** opens four **Performance Snapshots** (K1 slot, K2 save, K3/SEL recall). Press **FX** there to enter **Mute Groups**: K1 chooses one of four groups, K2 chooses its member lane, SEL adds/removes membership and K3 toggles the whole group. Both features follow the mixer's quantization behavior during playback.

### MASTER FX

Press **LFO** for the master effects chain. Turn **SELECT** to focus **EQ**, **Compressor** or **Reverb**; turn **K1–K4** to adjust the block's four parameters; press **SEL** to enable/disable it. Hold **SEQ** and turn **K4** to change processing order. Master effects apply to the overall output.

### Punch-In FX

Press **FX** until **PUNCH-IN FX** is displayed, or arm **REC** to use the keyboard for performance effects. Hold one of the eight mapped white keys. For the first five effects, **turn K1 while holding the key** to edit its setting; the last three are direct actions without a K1 parameter.

| Effect | Behavior |
| :-- | :-- |
| **REPEAT** | Loops a short recent audio fragment. K1 adjusts its length. |
| **RING MOD** | Applies ring modulation to the current mix. K1 adjusts intensity (default 20). |
| **FILTER** | Applies a live filter. K1 adjusts its setting. |
| **TAPE STOP** | Slows the audio and drops its pitch. K1 adjusts its setting. |
| **STUTTER** | Rhythmically gates the mix. K1 chooses a tempo-related rate (default ×1). |
| **OCT UP** | Temporarily shifts the effect's pitch upward. |
| **OCT DOWN** | Temporarily shifts the effect's pitch downward. |
| **MUTE** | Temporarily mutes the output. |

The five adjustable Punch-In settings are saved with the project. With **REC** armed and playback running, BEETLE can record held effects and parameter changes for later playback.

### Song Chains

Press **ARP** to arrange patterns into a complete song. A song has up to **64 positions**. Each position can draw **each of its six channels from a different pattern**: `CH1 CH2 CH3 CH4 CH5 DR REP`.

| Control | In Song Chains |
| :-- | :-- |
| **K1** | Select song position (POS 01–64). |
| **K2** | Select source column (CH1–CH5, DR or REP). |
| **K3** | Choose source pattern 01–48, `--` for a silent channel, or repeats 1–16 in REP. |
| **OCT+** | Insert a new song position. Each channel defaults to the pattern matching that position (POS above 48 defaults to P48). |
| **OCT−** | Delete the selected song position. |
| **SEL** | Turn SONG ON or OFF. |
| **ARP** | With SONG ON, switch between Song Chains and Live Mixer. |

**Example:** POS 02 can play CH1–CH5 from **P02** while DR uses **P01**. The source patterns remain unchanged. Shorter source patterns loop within the song position. Recorded global Punch-In events use the CH1 source pattern; setting CH1 to `--` also silences those events for that position.

### Projects and Project Manager

Press **HOME** to access **NEW**, **SAVE**, **SAVE AS**, **LOAD**, **HELP** and other project tools. There are **12 project slots**. A project saves patterns, song arrangements, instruments and library, drums, mixer, effects and performance settings. The current project data schema is **V19**; supported older projects are migrated when loaded. V19 files cannot be opened in older V18 firmware.

The [BEETLE Project Manager](https://sikkenworks.github.io/BEETLE/BEETLE-Project-Manager.html) can import/export `.beetle` files. Its AES-GCM file encoding uses a shared key included in the open-source tools: this protects against casual inspection or corruption, **not determined decryption**. Direct device transfer requires the **experimental Project Transfer firmware** and has not been confirmed on physical hardware.

**Storage and firmware safety:** On-device flash capacity is finite, so an entirely filled 48-pattern project is not guaranteed to fit. The experimental storage backend may overwrite older user data in `0x97000–0xDFFFF` on first-use provisioning. Obtain a verified full-flash backup and a working independent recovery method before experimenting with destructive storage or firmware updates. The WebInstaller does not make a full device backup, and successful transfer verification does not guarantee a bootable firmware or working recovery path.

For setup and build instructions, see [`docs/START-HERE.md`](docs/START-HERE.md). For release-specific fixes and historical changes, see [`CHANGELOG.md`](CHANGELOG.md).

## Support

For a bug report, include your BEETLE firmware version, the screen and buttons involved, steps to reproduce, and relevant project details. Share diagnostic logs only when safe; do **not** upload a private full-flash dump publicly. Contributions should preserve real-time audio safety, backward compatibility and third-party attributions.

---

## Credits and Licences

BEETLE is developed by **SikkenWorks (Joppe Sikken)**. It builds on earlier open-source FM1 work and is **not a clean-room firmware**. The following credits and licence distinctions are part of the project and must be retained.

| Project / contributor | Contribution and licence relationship |
| :-- | :-- |
| **[Felucca](https://github.com/hugelton/Felucca)** · **Leo Kuroshita (@kurogedelic), Hügelton Instruments** | Original FM1 hardware abstraction, startup/linker foundations, LCD, input scanning, audio and boot/OTA infrastructure carried forward in the ETCH/BEETLE native tree. **GPL-3.0-only.** The SHA-256 audit in `reference/felucca-hal-sha256.json` verifies 19 listed critical files only, not identity of the entire firmware. |
| **[ETCH](https://github.com/SikkenWorks/ETCH)** · **SikkenWorks** | Earlier FM1 firmware supplying the working ETCH V1.37 firmware-container/bootloader reference and lineage for some native, flash/OTA, UI and hardware-support modules. The BEETLE tracker, digital drum instruments and project workflow are BEETLE-specific. The protected bootloader remains unchanged and may show inherited branding. |
| **JieLi Technology** | Manufacturer of the AC79-series target chip and associated toolchain/device SDK. Applicable JieLi and upstream notices continue to apply; third-party compiler toolchain binaries are not bundled. |
| **[reSID](https://github.com/libsidplayfp/resid)** · **Dag Lem** | Original creator of reSID, a MOS6581 SID emulator and technical inspiration for BEETLE's five SID-style instruments: `SID 6581`, `SID 8580`, `SID DIRTY`, `SID BASS`, `SID NOISE`. **Original reSID C++ source is not bundled, compiled, linked or executed.** BEETLE's lightweight implementation is SID-inspired, not cycle-accurate reSID emulation. |
| **[Nes_Snd_Emu](https://github.com/jamesathey/Nes_Snd_Emu)** · **Shay Green (blargg)**, fork maintained by **James Athey** | NES APU technical reference for BEETLE's `NES PULSE`, `NES TRIANGLE` and `NES NOISE` types. Upstream library: **LGPL-2.1**. BEETLE implements an independent, lightweight fixed-point C engine in `firmware/fm1-native/beetle_nes_apu.h`. **Original Nes_Snd_Emu C++ code is not copied, embedded, compiled or linked** and this is not cycle-accurate NES APU emulation. See `LICENSES/NES-SND-EMU-LGPL-2.1.txt`. |

**Additional upstream attributions.** The preserved [`LICENSES/FELUCCA-LICENSING.txt`](LICENSES/FELUCCA-LICENSING.txt) acknowledges **DaisySP** (Electrosmith / Emilie Gillet, MIT), **Rings** (Emilie Gillet, MIT), **msfa/Dexed** (Google / Pascal Gauthier, Apache-2.0), **CrispyZebra**, **Fukiai**, **Inter Tight**, **DotGothic16**, **Versilian Studios VSCO-2**, **klattsch** and **X0X**. These are attributions within the broader upstream project; BEETLE does **not** claim to include all of those engines, fonts or sound assets.

**Artwork.** The enlarged two-frame pink loading illustration derives from artwork supplied by the BEETLE project user and uses nearest-neighbour pixel scaling without an external icon-font dependency.

**Source licence and redistribution.** BEETLE's native firmware and derivatives are distributed under **GNU GPL v3 only (`GPL-3.0-only`)**, preserving the licence of the Felucca-derived source. The full text is provided in [`LICENSE`](LICENSE). When distributing modified firmware, provide the complete corresponding source and applicable copyright, SPDX and upstream licence notices. A binary, WebInstaller or bootloader alone does not satisfy source-offer obligations. For detailed file-level provenance and third-party notices, see [`docs/CREDITS-AND-LICENSES.md`](docs/CREDITS-AND-LICENSES.md), [`LICENSES/FELUCCA-LICENSING.txt`](LICENSES/FELUCCA-LICENSING.txt) and the source-file headers. Where lineage is uncertain, retain upstream notices rather than claiming exclusive authorship.

<p align="center"><strong>BEETLE · SikkenWorks · Born out of passion.</strong></p>
