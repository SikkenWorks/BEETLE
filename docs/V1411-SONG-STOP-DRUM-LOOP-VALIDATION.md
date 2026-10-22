# BEETLE V1.4.11 — Song STOP + Row Clone + Drum Loop Laboratory

**Experimental firmware source. Not hardware validated.** Product identity is `FM-1_2123` from V1.3.8; protected boot header and OTA loader are unchanged in the packaged image. Actual safe OTA recovery and flash ownership are not independently proven.

## 1. Song Chain behavior

- An explicit `--` in a synth lane performs the **same note release as `[STP]`** at the start of that song position. On CH6 it chokes existing drum tails with a brief 48-frame fade. The action happens at the song-position boundary, not every step.
- An automatically empty lane also behaves as `--`, and becomes active automatically when its source pattern receives musical content.
- A **deliberately selected numbered empty pattern** (P01–P48) is different: it performs **no forced release**, so an earlier sustain is allowed to continue until note-off, another note or the instrument's own envelope release.
- New Song Chain rows copy the entire preceding row, including all six source lanes, the `--` state, and repeat count (REP). The initial row is automatically bound to P01, with empty lanes displayed as `--`.
- Song sources are represented internally as 0..47 for explicit Pnn, 128..175 for auto-bound Pnn and 255 for explicit `--`.

## 2. Project schema and migration

- New saves use **TRK1 V24** so an automatic hidden lane is distinguishable from an explicitly selected empty pattern after reboot.
- Old V23 projects remain loadable. V23 historical trailer import is tested using a CRC-correct converted fixture.
- Firmware V1.4.10, V1.3.8 and older cannot open newly saved V24 projects. Back up all `.beetle` files before testing/downgrading.
- Project Manager's optional legacy one-shot editor now **preserves V24** on save/export, instead of accidentally rewriting its version header as V22/V23. This protects new song auto/manual flags.

## 3. Double-hit drum pattern AUDIO STOP report

The physical report is a Pattern Mode playback stop after one loop when multiple patterns exist, with two simultaneous hits (e.g. kick/crash) per step, but not in Song Mode. The source contains heavy ROM sample indexing inside the per-frame drum path. This version reduces repeated ROM lookup work while preserving the exact original six-voice renderer and mixer output. Host regression renders 128 consecutive 16-step loops with both single and dual drum strikes and checks a bit-identical PCM checksum against the baseline.

**The physical DMA/AUDIO STOP root cause is not proven.** Offline rendering cannot show exact JieLi ISR deadline, LCD/USB priority contention, or NOR flash timing. This optimization could help but does NOT guarantee that a max-load physical FM1 will be glitch-free. Real device timing/cycle measurements and a verified recovery path remain mandatory.

## 4. Global user samples: Stage 5 preparation

- Preserve all 13 global U01–U13 sample slots, the original BD4S 4-bit codec and the six-voice background-prefetch laboratory.
- The host-only decoder now has explicit 48-frame *Song `--` stop* functions for single sample voices or all six voices. This matches CH6 stop behavior and is tested without flash reads in the render loop.
- The **host-only stream state is now 2,840 bytes** instead of 2,816 bytes, due to per-voice fade counters. Target linker RAM feasibility is not established.
- Sample length goal remains 10–20 seconds across 13 shared slots, dependent on free project storage. No physical global sample write/playback is enabled yet; the current `.fwsc` does not enable Storage V2.

## 5. Reproduce offline tests

Run at repository root:

```bash
bash tests/track/run_v1411.sh
bash tests/track/run_v1410.sh
bash tests/track/run_v13_audio_stability.sh
bash tests/track/run_v140_journal_oneshots.sh
python tests/dpcm4/run_prefetch.py
python tests/dpcm4/test_codec.py
python tests/dpcm4/test_storage_integration.py
```

Build with JieLi toolchain set in `JIELI_TOOLCHAIN`:

```bash
python firmware/fm1-native/build.py
python firmware/fm1-native/build.py --beetle-storage-lab --updater --usb-audio
python tools/build_v1411_fwsc.py reference/BEETLE-V1.3.8-ORIGINAL-PACKAGE.fwsc build/track-fm1-native-beetle-storage-lab-updater-uac/app.bin BEETLE-V1.4.11-SONG-STP-DRUM-OPT-UNVERIFIED.fwsc
```

Hardware verification must include: heavy overlapping drum tails across two or more patterns, Pattern Mode repeated 16-step loops, Song Mode transitions, eight synthvoices plus six drumvoices, all active USB audio channels, playhead/LCD activity and sample-rate drift. Stop and correct confirmed problems before release.

## 6. Acceptance limitations

Host tests, clean compilation and firmware hashes are **not equivalent to a real FM1 DMA benchmark**. The native SFC flash map and the irreversible OTA recovery pathway remain unverified. Do not install this candidate without a verified full device backup and an independent rescue/recovery route.
