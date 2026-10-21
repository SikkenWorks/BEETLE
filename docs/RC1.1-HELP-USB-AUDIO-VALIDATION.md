# BEETLE 1.01 RC1.1 — HELP chapters & USB-C audio

## Base

The user-provided `BEETLE-1.01-RC1.0.11b-SONG-AUDIO-STOP-FIX-GITHUB-READY-COMPLETE(1).zip` was used as the base. The existing tracker engine, OTA flow, FM1 HAL, project schema V19 and stock protected loader were retained. The previous RC1.0.11b firmware container is kept as `reference/BEETLE-1.01-RC1.0.11b-FWSC.reference` for reproducibility.

## HELP

- Twelve chapters in `beetle_help.h`; no tracker state or project serialization changes.
- K2 selects chapter; K1 scrolls within it; headings no longer consume rows in a continuous list.
- Native display: topic name + chapter index, nine help lines, per-chapter progress indicator and scrollbar, HOME/SEQ navigation.
- `simulator.html` and `web/simulator.html` use the same 12 topics, labels and navigation.
- `tests/track/test_rc11_help_chapters.c` checks chapter indexing, isolation and scroll boundaries.

## USB-Audio

- Existing Felucca-derived `usb.c` UAC1 EP4 IN streaming path is enabled with `--usb-audio` in the shipped native build. Code has a fixed stereo 16-bit 44.1 kHz capture profile; 48 kHz is intentionally not enabled here.
- `audio_target.c` taps the final stereo master signal after gain, including the analog-output mute/fade path; `uac_service()` serves USB frames from TIMER5. USB MIDI and analog output remain.
- The USB product descriptor presents the capture-capable device as `BEETLE Audio`. This is a **USB input to the computer**, not a playback device for the FM1.
- RAM budget with UAC originally missed its 96 KiB link region by 36 bytes. RC monitoring counters are now placed in `.pool` (an existing, boot-zeroed SRAM section). No FLASH/bootloader/OTA memory mapping was extended.
- The package builder `tools/build_rc11_fwsc.py` checks the protected header, OTA loader, decoded app bytes and container structure against RC1.0.11b. `tools/embed_rc11_webinstaller.py` binds the new package to SHA-256 in `WebInstaller.html`.

## Physical tests required before claiming production readiness

1. USB enumeration on Windows/macOS/Linux after connecting a direct data cable, including disconnect/reconnect and sleep/wake; capture device should be `BEETLE Audio` (or OS-equivalent).
2. Record at 44.1 kHz/16-bit stereo while analog sound and USB MIDI are working; compare left/right channels, output master gain and clip behaviour.
3. Stress 6 lanes, 8 synthvoices, drum voices, Song Chains at position 50+, Punch-In/MASTER FX, and repeated USB connect/disconnect while monitoring UAC under/overruns and DMA deadline counters.
4. Test long duration (30+ minutes) and sample synchronization with a DAW recording. No claim of glitch-free USB capture is made without these tests.
5. Verified full flash backup, power-interruption test, known-good independent recovery, firmware update and stock restore before hardware installation is considered safe.

**Status**: Source built and package structurally verified; physical FM1 USB and OTA validation **not done**. Preserve prior project backups.
