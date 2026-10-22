# BEETLE V1.4.14 — Hub Compatibility & User Sample Streaming Lab

**Candidate source + firmware, NOT hardware-validated. 21 October 2026.**

## Product identity and source basis

- The V1.4.5 firmware is used as the package reference. Product identity is **`FM-1_2123`** (also shared by the earlier V1.3.8 build).
- Its original protected 16 KiB and installed loader inside `.fwsc` are preserved exactly. This update changes the running BEETLE application's **outgoing updater**, not the loader shipped in the package.
- Source continues to derive from the original Felucca/ETCH BSP where credited; new sample/storage code is BEETLE-authored.

## Why other firmware was refused

The V1.4.13 running application accepted only the exact SHA-256 of an earlier ETCH-derived loader, plus one exact official V15 restore image. Valid FM1 packages with a different Felucca-family loader would fail before staging with error -16. The Firmware Hub's shared-ID detection is **not** the cause according to the Hub's public documentation: shared identity affects its *on-device badge*, not firmware installation.

## Native OTA change

- The original pinned loader still passes exact SHA verification.
- Additional **Felucca-family application-only loaders** can now pass with a bounded `FELUCCA-LOADER-1` first-literals marker and expected loader entry address. Full incoming loader CRC is checked **before** any staging erase. The original full app-area layout and CRC checks, read-back, commit-last and stock-V15-only path remain.
- Unsupported loader families (including unknown official/stock-like writers) continue to be refused. This is intentionally **not** a blanket bypass that could erase the boot area, OTA recovery, projects or flash. Firmware switching can still fail on a device that is already in another loader's update mode: that happens *before* running application OTA code participates.
- Marker+CRC is not cryptographic authentication. A malicious modified loader could imitate it; run only host-authenticated packages from trusted sources. The Firmware Hub SHA verification is performed by the browser, not inside BEETLE.

## Firmware Hub switching validation required

1. Backup all projects, bank files and full flash where independently supported. Establish a genuine independent recovery path before destructive OTA work.
2. Boot a tested candidate on an FM1, verify a read-only application identity probe responds with `FM-1_2123`.
3. Test outgoing Hub installation to an authentic current Felucca `.fwsc` and another compatible community firmware; independently verify both boot and identity. Avoid conflating the Hub's *loader-active* screen with a running BEETLE identity.
4. Test official V15 restore separately using the actual SHA-pinned original package.
5. Test interrupted update recovery and return from a different firmware. Do not assert "always possible" until all cases and loader families are independently reviewed.

The Hub documents that holding **OCT− at power-on** enters a USB rescue path if the installed app cannot launch normal update flow. If the device is genuinely in an incomplete loader, finish that loader's update/recovery before troubleshooting application code. See https://thefirmwarehub.com/kb/.

## User-sample progress

- `Storage.read_sample_range` reads only the needed portions of immutable current generation sample records and verifies per-chunk CRCs; no full-bank copy is required for a prefetch read.
- The new test `tests/dpcm4/test_v1414_storage_prefetch.py` loads the original 12 fixture projects, commits 13 original compressed BD4S sources into the offline NOR simulator, reboots the model, reads through the range-reader into the actual C prefetch implementation, and checks every output sample against the independent Python codec. Tests 6 parallel voices, zero render-context storage reads, sample replacement, checkpoint/reclaim and corruption refusal.
- **Not yet a device feature.** Important unresolved steps: ownership/migration of the existing 288 KiB project partition, ISR/main-thread concurrency, immutable voice generation leasing during bank replacement, sequencer lookahead, device USB upload/readback, memory-placement audit and worst-case DMA proof. Until then there is no new FM1 global-sample capacity.

## Reproduction

From repository root:

```sh
python3 tests/ota/test_v1414_hub_compat.py
python3 tests/dpcm4/test_v1414_storage_prefetch.py
python3 tests/dpcm4/run_prefetch.py
python3 tests/dpcm4/test_storage_integration.py
python3 tests/storage_v2/run_tests.py
bash tests/track/run_v1412.sh
python3 tests/installer/test_v1414_fwsc.py
```

Build with the original JieLi toolchain, **without connecting a device**:

```sh
export JIELI_TOOLCHAIN=/path/to/jieli-linux-toolchain
python3 firmware/fm1-native/build.py --hardware-test
python3 firmware/fm1-native/build.py --beetle-storage-lab --updater --usb-audio
python3 tools/build_v1414_fwsc.py BEETLE-V1.4.5-PLAYHEAD-GHOST-FIX-UNVERIFIED.fwsc build/track-fm1-native-beetle-storage-lab-updater-uac/app.bin BEETLE-V1.4.14-HUB-COMPAT-UNVERIFIED.fwsc
```

**Software success cannot establish hardware safety, guaranteed switching or glitch-free playback.**
