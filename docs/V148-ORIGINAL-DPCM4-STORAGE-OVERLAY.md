# BEETLE V1.4.8 — Original BD4S and full-storage overlay

**Development stage.** Own BEETLE sample codec offline; the existing FM1 project flash path only gets a visible failure popup. **Global samples remain unavailable on the real device.**

## V1.3.8 identity lock

Firmware identity `FM-1_2123` preserved byte-for-byte. Native `bringup.c`, `build.py`, USB SysEx and OTA package all use that same product ID. The candidate `.fwsc` has protected header and OTA loader byte-identical to the original V1.3.8 reference. Matching identity after firmware restart is **not** proof of new firmware installation or successful recovery.

## On-device storage-full UX

When a save returns `TRACK_JOURNAL_CAPACITY`, BEETLE shows **STORAGE FULL / PROJECT NOT SAVED** as a topmost LCD overlay for 3.2 seconds. If all twelve project slots were occupied, it says **ALL 12 PROJECT SLOTS USED**; for a too-large project or journal capacity limit it says **NO SPACE FOR PROJECT**. It cannot truthfully distinguish total free sectors from a per-project 12 KiB cap in legacy V23: neither is represented as spare global-sample space. Nothing is deleted, retried or reformatted by this popup. The existing blocking-input guard during `READING + VERIFYING`/`WRITING + VERIFYING` remains.

## PC library overlay

The Project Manager displays its own modal `STORAGE FULL` if the **PC** `.beetlebank` exceeds the 32 MiB host file cap. The old WAV assignment is reinstated before showing it. When the physical project-transfer endpoint returns error 12 (`Project exceeds FM1 storage capacity`) it uses the same alert type but a separate FM1 error message. It never asserts 32 MiB of FM1 storage.

## BEETLE BD4S original lossy codec

Codec written from scratch for BEETLE, not imported from other firmware. Mono 22,050 Hz signed PCM input, signed nibble (`-8..7`) adaptive difference steps, bounded step `[8..8192]`, 256-frame independently decodable chunks. Each chunk: 6-byte predictor/step/frame count, and 4 bits for every following frame. Complete `BD4S` file contains a 16-byte header with `CRC32` of all chunk bytes; decoding rejects bad CRC, lengths, versions, formats, or out-of-bounds blocks.

Encoder is offline Python; the C decoder is isolated and works on a **prefetched RAM block** (no malloc, no flash reads and no DMA ISR work). Neither the decoder nor the offline V2 flash allocator is yet wired into the FM1 audio engine. Bandwidth: about **11,600 bytes per second**, including block headers. Target total 10 seconds ~116 KiB, 20 seconds ~233 KiB, before on-flash allocator and index overhead. Compression is **lossy**, unlike the original `.beetlebank` WAV archive which stays byte-accurate on PC.

`python tools/dpcm4/beetle_dpcm4.py original.wav sample.bd4s --preview preview.wav` accepts 16-bit PCM mono/stereo WAV at 22.05 or 44.1 kHz; other rates intentionally rejected until a verified high-quality resampler is added. The optional preview is mono 22.05 kHz. Converted samples are *not* transferred to FM1.

## Offline acceptance tests

Run:

```
python tests/dpcm4/test_codec.py
python tests/dpcm4/test_storage_integration.py
python tests/dpcm4/test_ui_identity.py
python tests/storage_v2/run_tests.py
```

Python/C decoders agree exactly on blocks from sine, drums, HF noise, random and saturating fixtures. CRC and truncated block errors are rejected. The 10s/20s sector capacity test uses actual original BD4S compressed audio split among U01–U13 and a MOUNTAINKING/NUGGIE-sized twelve-project population, not filler bytes. **This is a simulated favorable workload only**: twelve full-size incompressible projects can leave too little space, and fragmented flash may need a safe compactor. The simulator's `authorized=True` flag does not grant actual NOR ownership.

## Hardware release blockers

Independent full-flash backup/recovery, physical NOR ownership, nondestructive migration from V23, robust power-failure recovery at *sub-page* timing, real-time safe DMA prefetch, USB bank transfer/readback, LCD/playhead and 8-synth/6-drum/USB audio stress, and physical FM1 validation all remain incomplete. **Never use this experimental `.fwsc` as proof of a stable, safe sample firmware.**
