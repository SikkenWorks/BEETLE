# BEETLE 1.01 RC1.0.10 — SAVE/LOAD Storage Fix Validation

## Root cause (native firmware)

`bringup.c` allocates `beetle_codec_buffer[TRACK_STORE_V4_MAX]` = 12,256 bytes. In RC1.0.9, both `track_journal_save` and `track_journal_load` rejected every buffer smaller than legacy `TRACK_STORE_MAX` = 24,576 bytes with `TRACK_JOURNAL_INVALID` (-2). The SAVE screen collapsed this into the generic `STORAGE ERROR` message. No normal save could advance to the journal write. This is an unconditional source-code failure for the actual storage-enabled build.

The encoder was also passed a false 24,576-byte capacity even though the allocated buffer was only 12,256 bytes. That mismatch could become a RAM overwrite if the first check were naively removed.

## Corrections

- Validate the actual native 12,256-byte workspace for both SAVE and LOAD.
- Pass the supplied real capacity into `track_store_encode`. Reject oversized projects before any physical erase.
- Refuse a READ that exceeds the supplied capacity before copying the payload.
- Distinguish `PROJECT DATA ERROR` and `FLASH I/O ERROR` in the firmware UI.
- Preserve the V18 format, A/B flash locations, project ownership/foreign data gating, cross-firmware OTA CHECK changes, reference 16 KiB boot header, OTA loader and project-transfer commands.

## Tests run

- PASS `bash tests/track/run_rc1010_storage_workspace.sh`: 12 slot saves and loads using the 12,256-byte buffer; same-project no-erase check; torn journal write and recovery; undersized-buffer rejection; oversized-project rejection before erase; guard bytes intact.
- PASS standalone V0.75 atomic fault injection: 223 error/power-loss cut points and retained latest-good bank.
- PASS all three native JieLi builds: storage+OTA, read-only, OTA pilot.
- PASS `bash tests/ota/run_rc1010_cross_firmware_audit.sh`: native OTA replay of BEETLE and original ETCH 1.36/1.37, incorrect XIP, damaged app and untrusted loader rejection, and JS verifier parity.
- PASS JavaScript syntax checking for the standalone WebInstaller, WebSimulator and Project Manager.
- Full historical V0.71 regression chain is NOT green: the V0.69 library test fails at an older decode fixture/assertion; this failure is separate from the physical workspace fix and was not silently modified.
- Firmware container CRC, program identity and embedded package SHA-256 checked. Real FM1 hardware NOT tested.

## Important remaining boundaries

A corrupted ownership marker, foreign sectors or a genuine SPI-NOR I/O failure may still reject saving; those are deliberately fail-closed. This bug fix neither formats flash nor bypasses the ownership checks. Physical device confirmation of the SAVE/LOAD path and OTA update remains necessary. Existing firmware on the FM1 does not receive the correction until a compatible, verified update succeeds.
