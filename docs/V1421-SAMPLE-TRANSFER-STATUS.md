# BEETLE V1.4.21 — USB sample transfer and integration report

## Implemented in native firmware

- New USB SysEx operations 0x70–0x75 for INFO/BEGIN/DATA/COMMIT/ABORT/DELETE; 24-byte chunks; acknowledgement sequence and verified CRC.
- Independent Storage V2 sample sector namespace, with copy-on-write, read-back, and whole-bank publication only after structural BD4S validation.
- Host abort/disconnect timeout, no flash writes from audio or USB ISR.
- Stable legacy one-shot metadata outside the shared flash scratch prevents overwritten pointers under audio load.
- Fixed product/package identity `FM-1_2123` and protected V1.4.5 header/OTA loader.

## Implemented on computer

- WAV 16-bit PCM 22.05/44.1kHz mono/stereo converter to 22.05kHz mono BEETLE BD4S 4-bit, byte-matched to offline Python encoder in a reference fixture.
- Device feature negotiation on a verified V2 volume; safe 24-byte packet upload and per-slot verification via CRC. Import/replace/delete, bank export/import, and original WAV preview unchanged.

## NOT COMPLETE

- This firmware does **not** migrate existing legacy projects into Storage V2. The existing offline migration-image builder can prepare a full-flash replacement, but no completed physical migration procedure was tested here.
- The native DPCM4 prefetch engine is still not connected to the FM1 drum mixer. Stored global samples will **not yet sound** when triggered on device. The existing legacy short user one-shots are separate.
- No physical M-VAVE FM1 is available for testing USB-MIDI throughput, audio stability, or installer behavior.

## Tests

- Native JieLi OTA/V2/UAC build and safe read-only build.
- Existing Storage V2 suite, atomic 71-write power-interruption sweep, 24-byte packet chunking, malformed BD4S/CRC rejection, 12 projects + 13 samples in the simulated 288-KiB flash, DPCM4 exact-PCM prefetch host test, and Song Chain/SID/drum regressions.
- WebInstaller firmware SHA-256 and protected loader verified offline.

**Do not claim this build completes user-sample playback.**
