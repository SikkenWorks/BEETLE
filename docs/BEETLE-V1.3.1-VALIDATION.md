# BEETLE V1.3.1: instrument expansion and validation

## Design

- CH1–CH5: five independent **synth channels**, sharing a maximum of eight synth voices.
- **8 active synthesizer patch slots** (INST 1–8) available from the instrument column in every synth tracker channel, and editable in the instrument edit screen.
- Each patch can choose among the **12 oscillator/type modes** already in BEETLE, including both VA OSC and VA OSC2, and retains separate dual VA A/B oscillator settings as applicable.
- The **8 Instrument Library presets** are project-local stored patches and remain a separate feature from the 8 live patch slots.
- CH6 is **the digital drum kit**, not instrument 4. Its tracker display identifies KIT / ROM instead of a synth instrument number.

## Compatibility

- Native TRK1 storage schema V20: eight synth patches, 3-bit instrument values, 8 TYPE-banks; unchanged maximum 12,256-byte packed payload.
- V19 historical projects are readable. Existing patch slots 1–4 and nonblank note references remain unchanged. New patch slots 5–8 are initialized to independent factory sounds. Truly blank legacy CH5 rows are normalized to avoid wastefully consuming pattern storage.
- Existing 64-position Song Chains, 48 patterns, separate drums, Punch-In events, USB audio, MASTER FX, and V19 project safety checks retained.
- V1.3 and earlier firmware **cannot open new V20 files**. Export and verify backups before upgrading.
- Oversized projects are rejected before any storage write. Eight patch slots do not increase the finite on-device journal capacity.

## Software verification performed

1. Native JieLi pi32v2 updater/storage/USB-audio firmware build + ELF-to-app audit **PASS**. App payload ~465 KiB; protected header and OTA loader hashes match V1.3 exactly.
2. Eight-slot V20 save/load and MIDI step reference 0–7 **PASS**. Standalone pattern encode/decode retains references to slots 7 and 8.
3. Authentic historical V19 file generated using unmodified V1.3 codec -> V1.3.1 decoding -> V20 re-encoding -> new decoding **PASS**. All 64 Song Chains rows preserved.
4. WebSimulator native V19/V20 import, V20 export, and JSON 4->8 patch migration **PASS**.
5. V1.3 Punch-In recording, no ghost FX at playback start, single-note/chord loudness, and left/right synth/drum panning regressions **PASS**.
6. Existing 12-type oscillator RMS balance, 120-second USB sample-rate/queue simulation, HELP chapter tests, WebSimulator JavaScript syntax, embedded firmware bytes and SHA-256 checks **PASS**.

Run from the source directory: `bash tests/track/run_v131_regression.sh`.

## Risks and limits

**Software testing does not certify physical M-VAVE FM1 hardware.** New patch arrays increase RAM usage. The linker pool grows by 3 KiB, reducing the reserved stack area. Real-time audio stress, long sessions, UAC/USB capture, power interruption, storage flash ownership, OTA updates and independent rescue remain **unverified**. Do not assume a verified checksum makes flashing safe. Test only with a full verified backup and a recovery route that has been proven on the target unit.
