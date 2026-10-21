# Complete repository reconstruction: BEETLE V1.4.2 DMA Patch 2

This is a full root-layout BEETLE project, **not** a nested `baseline-source` export.

## Sources and merge rules

- Native firmware and source tree: V1.4.2 DMA Patch 2, whose native base is V1.4.1.
- Global sample library Project Manager: V1.4.2 prototype.
- Additional historical BEETLE root resources: user-supplied V1.3.3 complete project (missing files restored; never allowed to overwrite newer DSP/firmware).
- Included user's reproducible `NUGGIE-SLOT-07.beetle` original unchanged.

## Frontend version note

`simulator.html` remains the historical V1.3.3 browser simulator. `WebInstaller.html` was upgraded from the original V1.3.3 frontend to **embed and SHA-pin the actual DMA Patch 2 candidate**. See `docs/V142-INSTALLER-INTEGRITY.md`. Global-bank Project Manager is V1.4.2, but FM1 native bank transfer is not implemented.

## Hardware caveat

DMA Patch 2 is **not** proven to eliminate the physical `DMA DEADLINE` fault. Firmware is experimental. Restore/recovery on real FM1 hardware was not tested here; don't use it as an installable stable release.

## Build and tests

```bash
export JIELI_TOOLCHAIN=/path/to/jieli-linux-toolchains-20260730.1
python firmware/fm1-native/build.py
python firmware/fm1-native/build.py --beetle-storage-lab --updater --usb-audio
bash tools/diagnostics/run_dma_host_tests.sh
bash tools/diagnostics/run_dma_nuggie_64loops.sh tests/fixtures/NUGGIE-SLOT-07.beetle
node tests/test_global_bank.js
```

All proprietary toolchains are external. Restoring historical build scripts does not imply those scripts build Patch 2; for Patch 2 use the native build.py above.

## Merge counts

- Historical V1.3.3 root files: **570**
- V1.4.1 + Patch2 source files overlaid: **616**
- V1.4.2 global-bank test and module additionally retained.
