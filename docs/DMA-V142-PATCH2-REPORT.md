# BEETLE V1.4.2 — DMA Patch 2 (software-verified only)

## Scope

This is a continuation of the source-level DSP optimization from DMA Patch 1.
The native firmware remains based on BEETLE **V1.4.1**; **V1.4.2** provides a
PC-only global WAV library. **This build is experimental and is NOT a demonstrated
fix for DMA DEADLINE on real FM1 hardware.** No device was accessed or flashed.

## What changed since Patch 1

- `track_chip.c`: scan three vibrato FX slots once per render chunk/channel,
  instead of every voice at every sample. The already-computed lane vibrato
  is passed to the oscillator render function. FX priority remains unchanged.
- `track_chip.c`: cache the channel's drive multiplier and combined mixer
  volume × step gain once per chunk, rather than multiplying/retrieving them
  for each active voice at every sample. Fixed-point integer operation ordering
  and output PCM remain unchanged.
- `tests/diagnostics/benchmark_nuggie.c`: add optional 2..128 contiguous loops.
  The supplied private project was tested for **64 consecutive loops** with CH2
  using its original SID BASS and with *each* of the 12 oscillator types.
  All 13 FNV PCM digests match the original Patch 1 renderer, including across
  release tails and loop wrap; max **8 synthvoices and 4 drumvoices**.
- Existing host PCM matrix: all 12 types, five VA shapes: **df02ac8b** unchanged.
- Existing 120-second 44.1 kHz synthetic stress test: **1183fe60** unchanged,
  8 synthvoices + up to 6 drums + FX, UndefinedBehaviorSanitizer pass.
- Cross build: native read-only and existing experimental OTA/USB 6-stem project
  journal variants compiled with the JieLi pi32v2 toolchain, linker/entry audit
  passed. Experimental UAC app size **484,008 bytes**, app SHA-256 in manifest.
- New FM-1_2123 candidate passes offline FWSC verification; protected 16 KiB
  header and OTA loader match the original reference byte-for-byte.

## What this DOES NOT establish

- A PCM-exact host render only checks deterministic DSP output. It does NOT
  measure FM1 audio-IRQ wall time, flash/XIP stalls, interrupt nesting, or USB
  endpoints. Under physical heavy load, **DMA DEADLINE can still occur**.
- PC benchmark `maxblock_us` includes scheduler interruptions; it is NOT a
  meaningful approximation of the FM1's 128-frame DMA deadline. No quantitative
  speedup or permanent freedom from underruns is claimed.
- The inherited deadline strategy may still shed synthvoices under overload,
  and can stop audio after repeated misses. This patch does not hide faults or
  remove safety guards. The user's request for zero DMA glitches cannot be
  honestly certified without physical instrumentation and stress tests.
- Desktop global U01–U13 library cannot be sent to native flash; RAM and
  protected flash ownership are unchanged. The `.beetle` project schema and
  12 on-device project slots are unchanged.

## Test/validation reproduction

From `baseline-source/` run:

```sh
bash tools/diagnostics/run_dma_host_tests.sh
bash tools/diagnostics/run_dma_nuggie_64loops.sh /path/to/NUGGIE-SLOT-07.beetle
```

With independently installed JieLi toolchain, run:

```sh
export JIELI_TOOLCHAIN=/path/to/jieli-linux-toolchains-20260730.1
python firmware/fm1-native/build.py
python firmware/fm1-native/build.py --beetle-storage-lab --updater --usb-audio
```

Repackage with the baseline reference, compiler-produced `app.bin`, and
`tools/build_v141_fwsc.py` (which validates the app audit and protected bytes).
The archive contains the full sources, browser Project Manager, credits and
licenses, test suites, build outputs, FWSC and integrity report. The proprietary
JieLi toolchain and private NUGGIE project file are **not** redistributed.

## Required hardware acceptance (not yet done)

1. Back up all 12 projects and verify an independent FULL restore path first.
2. On a recovery-capable test FM1, capture ISR wall-time high-water marks and
   LATE/GUARD for CH1; CH1+DR; CH1+CH2+DR, 30+ minutes each and across wraps.
3. Cover all 12 oscillators, every VA shape, high polyphony/ADSR release tails,
   MASTER FX, Punch-In and six stem USB recording while the LCD is active.
4. Do not publish as stable until all deadline/CPU/IRQ margins are measured,
   repeated hardware tests pass, and stock downgrade/recovery is verified.
