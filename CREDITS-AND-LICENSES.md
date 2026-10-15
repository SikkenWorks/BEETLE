# BEETLE · Credits, licenses, and instrument attribution

**BEETLE / SikkenWorks** was developed by **Joppe Sikken**. It is a custom 6-channel tracker for the M-VAVE FM1. Distribution of this source is GPL-3.0-only, with dependencies and their respective notices retained in `LICENSES`.

## FM1 hardware foundations

**Felucca** — **Leo Kuroshita (Hügelton Instruments)**, upstream FM1 startup,
HAL, display/input, audio and OTA foundations, licensed GPL-3.0-only.
See https://github.com/hugelton/Felucca and `LICENSES/FELUCCA-LICENSING.txt`.
**ETCH** — SikkenWorks, earlier FM1 firmware and protected OTA package reference.

## SID-inspired instruments

Original **reSID**: written by **Dag Lem**, a reverse-engineered MOS6581 SID emulator, licensed under **GPL-2.0-or-later**. See https://github.com/libsidplayfp/resid and its `COPYING` and `AUTHORS` files for precise upstream attribution. The reSID upstream code is *not* copied, compiled, linked or embedded in BEETLE V0.53.

BEETLE's `beetle_sid_lite.h` and the five **SID 6581, SID 8580, SID DIRTY, SID BASS, SID NOISE** instrument flavors are original, approximate fixed-point SID-style synth implementations. These emulate aspects of the sound but **do not** implement cycle-exact reSID, chip-specific measured oscillator combinations, or authentic analog SID filters. They should not be described publicly as original reSID or exact MOS6581/8580 emulation.

## NES-style instruments and Nes_Snd_Emu

**Nes_Snd_Emu** — originally written by **Shay Green (blargg)**, later maintained
in the linked fork by **James Athey**.
Project: https://github.com/jamesathey/Nes_Snd_Emu
Licence: **LGPL-2.1** (`LICENSES/NES-SND-EMU-LGPL-2.1.txt`).

The three BEETLE V0.78 oscillator types **NES PULSE**, **NES TRIANGLE**
and **NES NOISE** are original C/fixed-point code in `beetle_nes_apu.h`,
informed by NES APU architecture and the Nes_Snd_Emu technical reference.
**No Nes_Snd_Emu C++ source is copied, linked or compiled into BEETLE.** The on-device CREDITS page attributes the Nes_Snd_Emu authors and describes the library as a technical reference.
Consequently, BEETLE must not be described as shipping the accurate upstream
Nes_Snd_Emu emulator, and it does not claim cycle or nonlinear-mixer accuracy.
The upstream LGPL licence is reproduced for completeness and credit, not as a
claim that the new native source changes the project's GPL-3.0-only licence.

## Other sources and upstream dependencies

This FM1 firmware's boot/HAL lineage follows Felucca / ETCH. See `source/LICENSES/FELUCCA-LICENSING.txt` and `source/firmware/fm1-native/LICENSES/*` for notices that must accompany distribution. BEETLE beta releases are tested on real FM1 hardware by the project maintainer before GitHub publication. New NES voices and firmware-switch paths require their own device checks before release sign-off.
