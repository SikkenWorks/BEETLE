# BEETLE V1.4.2 DMA Patch 2 — installer / hardware safety

## Corrected versioning

The previous full-project ZIP shipped with `WebInstaller.html` still embedding the **actual V1.3.3** firmware (product `FM-1_2118`, pinned SHA-256 `16544049...`). Changing a title would not fix that. The root and `web/` installer are now updated, with exact embedded bits from `BEETLE-V1.4.2-DMA-PATCH2-UNVERIFIED.fwsc`.

| Field | New value |
|---|---|
| Displayed version | V1.4.2 DMA PATCH 2 |
| Firmware ID | `FM-1_2123` (same as native V1.4.1) |
| Length | 610,086 bytes |
| SHA-256 | `a82ddfd02c1f26ee1923fff59ecd864eef8b7cdd806a75f2aa0c403a9089b6aa` |
| Packaging | Flash/app and OTA sections pass original CRC checks |
| Protected boot header | Identical to V1.4.1 reference, first 0x4000 bytes |
| OTA loader | Identical to V1.4.1 reference |
| Physical OTA installation | **Not tested** |
| Physical DMA deadline fix | **Not proven** |

The old B1 WebMIDI staging and source-identity checks are preserved. Offline verification checks the FWSC checksum, CRC, preserved loader and identity; there is still no cryptographic proof that a flashed FM1 boots the updated application, because the product identity is reused. The UI now explicitly explains this limitation.

## Required tests

```bash
python tests/installer/test_v142_dma_installer.py
node tests/installer/test_v142_dma_installer.cjs
bash tools/diagnostics/run_dma_host_tests.sh
bash tools/diagnostics/run_dma_nuggie_64loops.sh tests/fixtures/NUGGIE-SLOT-07.beetle
```

## DMA stability remains an open hardware task

The host-side DSP loop comparisons prove that optimized samples are PCM-identical for tested waveforms and loads. The JieLi builds compile. None of those tests exercises the real FM1 ISR, flash bus arbitration, USB transfer timing, physical ADC/DAC, LCD interrupt load or actual DMA deadline. The current firmware can still reduce voice count on measured missed blocks and enter AUDIO STOP after continuing failures. Do **not** promise that the patch eliminates deadlines or is production-safe.

Required for stable release: back up all twelve project slots; verify recovery with an independent tool; instrument 128-frame ALNK0 wall-time and worst-case 95% margin, guard and late counters; 30+ minute NUGGIE loops under CH1+CH2+DR and across all 12 oscillators, VA waveforms, all effects, USB six-track active and screen redraws; power interruption and stock-recovery tests. If the actual worst-case audio time exceeds the 128-frame deadline (about 2.90ms at 44.1 kHz), the DSP/IRQ bottleneck must be fixed and remeasured, not hidden by muting voices or disabling safety checks.

## Frontend coverage

- `WebInstaller.html`, `web/WebInstaller.html`: V1.4.2 DMA Patch 2, actual firmware embedded.
- `BEETLE-Project-Manager.html`, `web/BEETLE-Project-Manager.html`: V1.4.2 global PC-only WAV bank.
- `simulator.html`, `web/simulator.html`: historical V1.3.3 simulator (not a native V1.4.2 audio emulator).
