# RC1.0.11 SONG CHAINS validation

- C runner: `bash tests/track/run_rc1011_song_chains.sh` checks lane isolation, NOTE1/NOTE2 FX, looped short patterns, `--` mute, insert defaults, repeats, V19 encode/decode and V18 binary migration.
- Storage workspace: `bash tests/track/run_rc1010_storage_workspace.sh` confirms all 12 A/B slots remain serializable within 12,256 bytes. Oversized projects are rejected before erasing.
- Chromium web: `python tests/web/test_rc1011_song_chains.py` tests SONG navigation, inserting and independent lane editing; `python tests/web/test_rc1011_native_v19.py` tests V19 binary export/import.
- Project Manager uses firmware-advertised schema and refuses writing V19 to an older V18 firmware.
- The experimental `.fwsc` reuses the protected RC1.0.10 header and OTA loader byte-for-byte; the app source and full container CRCs were software-validated.

**NOT VERIFIED:** physical FM1, audio timing while mixed songs are playing, native USB-MIDI Project Manager roundtrip, external OTA loader transition, and long-duration stress tests. Do not assume that the firmware is safe to flash merely because software builds and CRCs pass.

- OTA replay (native C): `bash tests/ota/run_rc1010_cross_firmware_audit.sh` replays the retained RC1.0.10 baseline, ETCH V1.36/V1.37, invalid variants, and current RC1.0.11 `.fwsc`. Tests confirm no erases for rejected corrupt applications or untrusted loader. This is a software-only simulation.
