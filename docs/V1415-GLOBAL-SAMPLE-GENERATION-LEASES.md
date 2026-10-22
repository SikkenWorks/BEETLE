# BEETLE V1.4.15 — Generation-leased global sample storage (offline lab)

**Development source only, not an installable V1.4.15 firmware.**
The FM1 `.fwsc` and WebInstaller are the **unchanged V1.4.14 experimental** build.
Package identity stays exactly `FM-1_2123`, as requested from V1.4.5.
No flash ownership or safe migration route has been proven on physical FM1 hardware.

## Confirmed findings and corrections

1. **Incorrect recovery reserve check.** The previous storage model counted four *empty 256-byte pages*, not the promised four completely erased *4096-byte sectors*. An update could otherwise succeed while leaving less than the required erase/compaction headroom. `_plan()` now counts only wholly erased, unallocated NOR sectors after accounting for every pending page. An unsafe update fails **before any NOR mutation**.
2. **Mutable-slot read hazard.** The old host prefetch callback took only U-slot and offset. If the sample was replaced or deleted during playback, later prefetches could read the new generation midway through the old voice, or lose a sector after garbage collection. The model now has `acquire_sample()` / `read_sample_lease()` / `release_sample()`. A pinned immutable generation protects its record sectors across updates, checkpoints and deletion until the last lease is released.
3. **Per-voice native callback.** The standalone C prefetch lab adds `beetle_bd4_stream_prepare_pinned()` and `beetle_bd4_prefetch_poll_pinned()`. The read callback receives the **voice, U-slot and immutable generation** and must verify all three. A pinned voice explicitly rejects the older mutable-slot polling API (return code -2). `beetle_bd4_stream_disarm()` is called in scheduler context, after audio stops accessing that voice, **before** its storage lease is released. The legacy host-only API remains for existing tests and compatibility.
4. **No DSP changes.** The real-time render loop, existing eight synthvoice and six drumvoice engines, USB audio, Song Chains, LCD and OTA/loader have not changed. Decoding and CRC storage reads remain background-only in this lab. This does not establish physical FM1 DMA safety.

## Tests

Run from project root:

```sh
python3 tests/dpcm4/test_v1415_pinned_storage.py
python3 tests/dpcm4/run_prefetch.py
python3 tests/dpcm4/test_v1414_storage_prefetch.py
python3 tests/dpcm4/test_storage_integration.py
python3 tests/storage_v2/run_tests.py
python3 tests/ota/test_v1414_hub_compat.py
python3 tests/installer/test_v1414_fwsc.py
```

Validated in this iteration:
- Two simultaneously playing voices on *the same* U-slot, each using a distinct sample generation, give bit-identical 44.1 kHz PCM while a replacement and full metadata checkpoint occur between playback blocks.
- Replacement, deletion and garbage collection cannot reclaim sectors still leased to active voices. Expired/double-released leases are refused.
- Native legacy callback is rejected for generation-pinned voices. No storage callbacks run in the audio renderer. No underruns in the scheduled integration scenario.
- With only five fully empty sectors available, an attempted two-sector upload is refused without mutating NOR because it would violate the four-sector reserve.
- Existing 12 projects, U01–U13, sample codec and prefetch, OTA replay and FM1-package integrity tests pass.
- JieLi target builds of V1.4.14 were rerun. Generated app binaries have SHA-256 identical to the V1.4.14 source-release reference. The new lab module is **not** linked into either app.

## What is still missing before real user samples on FM1

- Verify the exact hardware flash partition ownership and execute a non-destructive migration design for the existing 12 V23/V24 project slots. An empty simulated 288 KiB volume is not an installed FM1 volume.
- Implement a device-side flash/NOR driver transaction manager that supports background, bounded reads and per-voice immutable leases, with real scheduler/interrupt synchronization, pinning and release after the DMA reader is finished.
- Implement a versioned USB capability negotiation, upload, readback, CRC check and commit that never targets foreign/boot/OTA/restore sectors.
- Calibrate background prefetch scheduling, RAM placement and bus contention while running eight synthvoices, six drumvoices, USB audio and the step-accurate LCD. Demonstrate zero recurring underruns and timing margin on physical hardware, not only a desktop run.
- Independently verify OTA updates *from* BEETLE through The Firmware Hub to other supported firmware and confirm a genuine recovery route. Existing expanded compatibility is not universal or hardware-proven.

**Do not install V1.4.15 sample code to flash. This stage is strictly offline.**
