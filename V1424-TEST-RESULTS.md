# BEETLE V1.4.24 — Software validation

- JieLi target experimental native V2+OTA+UAC build: PASS, app 505,256 bytes; 76,308-byte application headroom.
- JieLi read-only diagnostic build: PASS, linked forbidden-symbol audit PASS.
- New B2SB bootstrap byte-for-byte equal to established Storage V2 simulator format: PASS.
- Project Manager Chromium simulation: initial state; backup-before-erase; 2 occupied + 10 empty; V2 conversion, restore and verification: PASS.
- Changed project digest before erase: refused (0x5E never sent): PASS.
- Restore from migration ZIP without reformatting: PASS.
- Existing V22 -> V24 import regression: PASS.
- Existing AES-256-GCM project encryption / SysEx wire tests: PASS.
- Storage V2 offline regression suite, including 20-second bank scenario, project preservation and flash power cuts: PASS.
- WebInstaller embedded firmware matches loose `.fwsc`: PASS.
- Protected loader, boot/header and firmware identity `FM-1_2123`: PASS offline.
- Physical FM1 USB / conversion / DPCM4 heavy-load playback: **NOT TESTED**.
- Firmware candidate SHA-256: `f18f04100ae1e924af5f977166533ccfad24d8a499eb4397cb73364e807137ad`.

**Storage conversion is not atomic.** Its old project partition is erased after the host backup is confirmed. A failure may require restoring from the local encrypted ZIP or using the established independent recovery route.
