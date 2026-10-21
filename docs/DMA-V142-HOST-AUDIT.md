# BEETLE V1.4.2 DMA LAB PATCH 1 — native DSP analysis

**Software-validated prototype. Not a verified DMA DEADLINE cure or approved production release.**

## Reproduction / source data

- Input supplied privately by tester: `NUGGIE-SLOT-07.beetle` (portable AES-GCM V2 container).
- Container authentication: **passed**. Inner `TRK1` V22: **1724 bytes**, header CRC32 and payload CRC32: **passed**.
- Name `NUGGIE`, physical project slot **07** (zero-index slot 6), **120 BPM**.
- Pattern 01: CH1 starts a chord on INST1 (RETRO PULSE), CH2 plays notes using INST3 **SID BASS**, CH6 triggers paired factory drums at multiple steps. Song mode is disabled; other patterns have events but are not required for this incident.
- The test reproducer decodes pattern 01 and renders **two complete 16-step loops**; in software it reaches **8 active chip voices and 4 active drums**. Repeated note onsets also create bounded 176-sample release tails. This supports a credible high-load explanation for why adding CH2 triggers a late DMA frame. It does **not** prove which ALNK interrupt exceeded its deadline on the user's FM1.

## Confirmed source-level hotspots and exact-sound optimizations

1. **SID family (6581/8580/DIRTY/BASS/NOISE):** Previously recomputed 2 PWM thresholds, effective filter cutoff and resonance every output sample per voice. `beetle_sid_prepare` computes them once per stable render chunk; `beetle_sid_sample_prepared` retains all existing filter equations and state changes. Channel-step changes take effect at the next chunk as before.
2. **VA OSC / VA OSC2:** Pulse width used dynamic **64-bit division by 100 for every oscillator sample**; replace with a ROM lookup containing the mathematically identical 5–95% phase offsets. Wavetable antialiasing/mip tier selection is also hoisted from every VA sample to the beginning of each render chunk. The same harmonic-limited tables and interpolation remain in use.
3. **Digital drum engine:** Previously obtained each sample's ROM bounds, EQ sustain floor and attack scaling divisor inside the hot loop for up to six voices. Per-voice invariants are now resolved once per block. ROM PCM, interpolation, EQ, envelopes, choke, gain and note mapping are unchanged.
4. **Step modulation:** Five channel LFO rate and FX-type scans per output sample moved to once per stable audio chunk; phase increments retain exactly the same values.
5. **Master EQ / compressor / reverb:** Effect order and whether blocks are disabled / EQ neutral are now checked once per render chunk. Fixed compressor ratio and makeup terms are similarly hoisted. Dynamic envelope processing, reverb memory and active FX ordering are unchanged.
6. **Memory:** SID/drum scratch caches are in the existing **volatile `.beetle_song` RAM pool**, not the device's already-full low 96-KiB `.bss` or a new flash partition. This is critical: an intermediate build failed linking with a **156-byte RAM overflow** until the scratch was moved to the intended pool.

No sample-memory partitions, bootloader, OTA sectors, project layouts, `.beetlebank` format, mixer routing, DSP resolution, synth voice cap or drum voice cap were changed.

## Tests executed

- Exact PCM equivalence against the **unmodified V1.4.1 native code**: `df02ac8b` FNV hash both before and after patch, 12 oscillator types × 5 VA waveforms, eight simultaneous synthvoices and six retriggering drums with note release/retrigger. This tests sample identity, not audio deadline timing.
- NUGGIE: original CH2 SID BASS plus every other 0–11 synth type, two loops each. All 13 source/updated audio hashes matched exactly when compiled for the same host. Maximum observed active voices: 8 synth / 4 drums. PC execution-time numbers are **not** FM1 timings and are not a proof of speedup on JieLi hardware.
- **120 seconds of host audio at 44.1 kHz**, synthetic heavy usage of eight synthvoices, six drumvoices, all types, FM and VA shapes, five per-lane step FX, Punch-In, MASTER FX, with UndefinedBehaviorSanitizer: passed; deterministic hash `1183fe60`.
- Other native regression cases from the preserved V1.3.5 suite: independent multi-FX, V22 codec, block editing, 6-stem routing, 12-type waveform balance, Song Chains, UAC SRC 120-second simulated throughput: passed.
- JieLi native lab build with USB audio, experimental existing project journal and OTA: linker/app audit passed. Protected header/OTA loader and original FM-1_2123 package shape can be checked in the candidate FWSC manifest.

### Existing inherited test limitations

- Some historical runner scripts assert hard-coded **V19** project versions or a **V1.2** PCM digest. They already conflict with unchanged V1.4.1/V22 data and are not a valid regression verdict for this patch.
- The provided 1.4.2 prototype archive omits old `web/BEETLE-FirmwareSimulator-V1.3.5.html` assets; old web regression scripts terminate with a file-not-found error. The supplied Project Manager HTML is preserved unchanged.
- The prototype archive also omitted `reference/felucca-hal-sha256.json`. It has been reconstructed by **actual byte-for-byte matching against the supplied Felucca-main.zip** for 15 matched HAL files. Divergent HAL files were intentionally not listed as matching. This is a recovered, narrower audit, not a claim of complete Felucca equivalence.

## What is not proven

- The device-specific exact cause of the crash, which may be a CPU/DMA miss, nested TIMER5/USB interrupt contention, external flash XIP stalls or LCD/USB arbitration.
- Actual worst-case ISR elapsed TIMER4 ticks in all scenarios, and whether they fit the FM1's `128 * 544 = 69,632`-tick audio window, including the required `4,096`-tick buffer-publish reserve.
- USB 6-stem host capture stability, full device audio fidelity, flash update safety, and physical uninterrupted operation for hours.
- That **no conceivable heavy-load project** can ever trigger an audio DMA fault. Such a guarantee requires precise physical CPU/interrupt measurements and an independently tested rescue path; arbitrary user arrangements are not a fixed workload.

## FM1 acceptance test before any production release

1. Independently back up all 12 `.beetle` projects and full flash, and verify an **actually functioning** stock restore procedure. Merely obtaining `0xFF` sectors, a passing OTA simulation, or a byte-identical protected header does **not** establish safety.
2. On hardware with a recovery route, run `NUGGIE` CH1 only, CH1+DR, and CH1+CH2+DR, repeat Pattern 01 for **at least 30 minutes** each, note the *exact* firmware/UI/LATE/GUARD counters.
3. Repeat with CH2 successively set to all twelve oscillator types, then with dual VA pulse combinations, maximum six simultaneous drums, maximal eight voices, long tails, all three MASTER FX, per-step FX and Punch-In. Check both initial play and **second/third/fiftieth** loop boundaries.
4. Repeat the above while recording stereo USB, then six-channel USB multitrack; check channel alignment, undistorted frames and dropouts. Check LCD scrolling, USB-MIDI traffic and input scanning simultaneously.
5. Measure maximum *wall-time* per ALNK IRQ, `native_late`, `native_dma_blocked`, UI pressure, USB stalls, actual stack/POOL guards and long-term stability. Reject the release for any repeatable miss rather than silently sacrificing voices or suppressing a fault message.
6. Power-cycle and validate all 12 projects, global PC-only library, OTA rollback and stock firmware recovery with the same backup set. **Do not release as a proven fix** until physical measurements pass.

## Build and host reproduction

- Host tests: `bash tools/diagnostics/run_dma_host_tests.sh` (includes PCM parity, 120-second heavy render). To test NUGGIE, first decode locally using `python tools/diagnostics/decrypt_beetle_v2.py NUGGIE-SLOT-07.beetle nuggie.raw` (requires Python `cryptography`), then `bash tools/diagnostics/run_dma_host_tests.sh nuggie.raw`.
- Native read-only build: `JIELI_TOOLCHAIN=/path/to/toolchain python firmware/fm1-native/build.py`.
- Experimental storage/USB/OTA build: `JIELI_TOOLCHAIN=/path/to/toolchain python firmware/fm1-native/build.py --beetle-storage-lab --updater --usb-audio`.
- Candidate `.fwsc` is **OFFLINE PACKAGE VERIFIED ONLY**. It is not a verified stable install and must not be installed without the prerequisites above.

Source and attribution remain GPL-3.0-only; retain Felucca, ETCH and third-party credits.
