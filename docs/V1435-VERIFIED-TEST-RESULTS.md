# BEETLE V1.4.35 — validation evidence (host, 2026-10-25)

| Test | Result |
|---|---|
| JieLi native build `--beetle-storage-lab --beetle-v2 --updater --usb-audio` | PASS; 512,684 B app; app-slot headroom 68,880 B |
| Firmware `.fwsc` verify and decrypted-app payload compared to native app.bin | PASS |
| Protected first 16 KiB and OTA loader byte-for-byte versus V1.4.34 | PASS |
| V23 project regression `tests/track/run_rc1011_song_chains.sh` | PASS (correct, actually V23-structured test fixture) |
| V1.4.34 song selection and looping host test | PASS |
| V1.4.35 bulk ZIP local archive roundtrip and corruption/duplicate/path/central directory tests | PASS |
| Punch FX `tests/track/run_v1427_fx.sh` | PASS |
| USB 6-stem realtime host model `tests/usb/run_multitrack_realtime.sh` | PASS; idealized model, not real FM1 |
| WebSimulator, Project Manager, WebInstaller, All-in-One Manager JS parse | PASS |
| WebInstaller embedded `.fwsc` and All-in-One embedded 3 HTML source equality | PASS |
| Physical FM1 LEDs, knob long-press, USB transfers, real audio, OTA flashing and rescue | NOT TESTED |

## Historical test failures, not claimed as passes

- `tests/track/run_rc1011b_audio_stability.sh`: pre-existing PCM checksum mismatch, reproduced *also in unmodified V1.4.34*: measured `dd0f3681`, hardcoded `43c0f1f6`.
- `tests/track/run_v1413.sh` chains an old `run_v1411.sh` test that still asserts serialized schema `V24` even though current BEETLE saves `V25`. The test aborts. The newer V23/V25 migrations and relevant song-loop regression are tested separately; the old hardcoded V24 assertion was not silently changed.

The V23 failing fixture was legitimately repaired without changing production decoder and remains supported as regression coverage.

## Safety

Firmware remains experimental and unverified on physical FM1. Bulk import is not an atomic transaction: if interrupted, earlier slots may have been committed while later slots remain unchanged. Each imported project is read back after write, with no automatic restore. Import does not include shared user-sample content. Back up project data and shared samplebank separately, and verify a recovery path before OTA installation.
