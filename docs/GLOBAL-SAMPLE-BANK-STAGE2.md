# BEETLE Global User Samples — Stage 2

**Development stage:** PC/library format and independent native read-only parser.
**FM1 firmware:** Unchanged V1.4.5. **No global-device sample transfer or realtime playback is claimed.**

## Fixed product behavior

- One shared 13-slot bank (U01–U13), independent of the twelve BEETLE projects.
- Drum-key indices 14–26 are reserved for these 13 user samples; 0–13 remain the original fixed drum mappings.
- Replacing Uxx will affect any project using Uxx *once a native shared bank exists*. Deleting Uxx should make that reference unavailable/silent without remapping an unrelated sample.
- `*.beetle` project files must continue to carry only musical content and slot references, never duplicated WAV payloads once migration is implemented.
- The PC `.beetlebank` format remains **v1**, so existing bank files remain readable. The file cap is 32 MiB **on the PC only**.

## Implemented in this stage

1. Strong RIFF/WAVE verification: required `fmt ` + `data`, bounded chunks, correct block alignment and supported PCM/IEEE-float formats. WAVE_FORMAT_EXTENSIBLE PCM/float is included. RIFF metadata remains byte-for-byte untouched. Unsupported compressed WAV formats fail clearly; other audio formats are not silently decoded/resampled.
2. Project Manager displays duration, channels, native WAV rate and bit depth, plus PC bank usage; adds **EXPORT WAV** for individual slots and stops preview playback before replacement/deletion/bank switching.
3. CRC-32 now uses a compatible lookup-table implementation for more efficient PC handling of large WAVs. The `.beetlebank` **binary layout is unchanged**.
4. A standalone C **read-only bank parser** (`firmware/fm1-native/beetle_global_bank_readonly.[ch]`) uses a 256-byte scratch chunk for data CRC, reads 13 slots through a storage-independent callback and never allocates or writes. It is **not linked into the FM1 build**. Test utility: `tests/global_bank/inspect_native.c`.
5. Cross-language tests assert `.beetlebank` v1 compatibility, exact round-trips, PCM/float variants, and corruption rejection by both JavaScript and C.

## Unresolved hardware barriers (DO NOT BYPASS)

- Available 1 MiB SPI NOR ownership has not been independently qualified. OTA staging, project A/B journals, autosave, voice-bank, VM backup and globals are protected; erased sectors do **not** imply free capacity.
- The present legacy 6,144-byte project-owned µ-law buffer cannot hold long one-shots. A proven new physical storage location **and** bounded non-blocking audio prefetch are required.
- The C parser validates a **host bank file** through a callback; it does not make the FM1 flash file system magically support 32 MiB.
- No USB upload, firmware flash writes or `.fwsc` replacement were added in this stage. All twelve projects and the current synth/drum audio path remain untouched.

## Next native milestone (gated)

1. Obtain physical read-only SPI NOR identification/mapping plus independent restore-tested bootloader and a verified full-device backup. Determine whether a **separate storage component** is necessary, since the existing 1 MiB layout is reserved.
2. Define a durable A/B transactional bank, CRC readback, power-loss rollback, and space reclamation without borrowing project/OTA sectors.
3. Prototype a nonblocking prefetch mechanism outside the audio ISR and measure worst-case 8 synthvoices + 6 drumvoices + 6-track USB + LCD refresh on **physical FM1 hardware**.
4. Add a new explicit USB-MIDI global-bank capability query; keep upload disabled unless the real device reports the protocol and verified storage size. Project transfer opcodes must never be used for arbitrary flash writes.
5. Add a versioned migration from old per-project V23 samples with safe V22 fallback; retain all twelve projects.
6. Release installable firmware only after hardware/audio/OTA/recovery validation.

## Reproducible offline tests

From the project root:

```bash
node tests/test_global_bank.js
cc -std=c11 -O2 -Wall -Wextra -Werror -I firmware/fm1-native \
 firmware/fm1-native/beetle_global_bank_readonly.c \
 tests/global_bank/inspect_native.c -o tests/global_bank/inspect_native
node tests/global_bank/test_cross_language.cjs
```

These tests prove file-format interoperability, **not** physical FM1 storage or DMA safety.
