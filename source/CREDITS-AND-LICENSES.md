# BEETLE · Credits, licenses, and instrument attribution

BEETLE is a custom 6-channel tracker for the M-VAVE FM1, maintained by SikkenWorks. Distribution of this source is GPL-3.0-only, with dependencies and their respective notices retained in `LICENSES`.

## SID-inspired instruments

Original **reSID**: written by **Dag Lem**, a reverse-engineered MOS6581 SID emulator, licensed under **GPL-2.0-or-later**. See https://github.com/libsidplayfp/resid and its `COPYING` and `AUTHORS` files for precise upstream attribution. The reSID upstream code is *not* copied, compiled, linked or embedded in BEETLE V0.53.

BEETLE's `beetle_sid_lite.h` and the five **SID 6581, SID 8580, SID DIRTY, SID BASS, SID NOISE** instrument flavors are original, approximate fixed-point SID-style synth implementations. These emulate aspects of the sound but **do not** implement cycle-exact reSID, chip-specific measured oscillator combinations, or authentic analog SID filters. They should not be described publicly as original reSID or exact MOS6581/8580 emulation.

## Other sources and upstream dependencies

This FM1 firmware's boot/HAL lineage follows Felucca / ETCH. See `source/LICENSES/FELUCCA-LICENSING.txt` and `source/firmware/fm1-native/LICENSES/*` for notices that must accompany distribution. Firmware build variants and OTA are experimental and **not physically certified**.
