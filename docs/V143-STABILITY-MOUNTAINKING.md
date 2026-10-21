# BEETLE V1.4.3 STABILITY LAB — MOUNTAINKING / Audio & Storage Input

**Hardware status: NOT verified. DO NOT market this candidate as a guaranteed fix for DMA DEADLINE or stock OTA recovery.** This iteration is source- and host-validated, not device validated.

## Input projects
- MOUNTAINKING-SLOT-01.beetle: valid authenticated V2 .beetle, native TRK1 V20, CRC-valid, 32 Song positions, 156 BPM.
- Position 18 uses CH1 P18, CH2 P15, CH3 P15, CH4 P09, CH5 P08, drums P13.
- Position 19 uses CH1 P19, CH2 P14, CH3 P16, CH4 P08, CH5 P09, drums P11, including 4 dual drum triggers.
- NUGGIE-SLOT-07.beetle: prior DMA regression, native V22.
- Reference source: user-provided BEETLE V1.3.8 archive. Existing `track_song.c` and the DMA interrupt module were unchanged between V1.3.8 and DMA Patch 2; the baseline is not proof of identical load behavior on physical hardware.

## Fixes
1. `track_chip_render`: skip step FX dispatch when all three per-lane FX slots are zero; oscillator phase and envelope are still advanced identically, verified through the preexisting PCM matrix checksum `df02ac8b` and a 120s 8-synth/6-drum soak `1183fe60`.
2. `beetle_lcd_min_interval`: throttle 240x240 SPI-DMA redraw only after actual recent audio deadline pressure, reducing LCD contention in heavy Song Mode. The transport model and playhead step count remain driven by sample clock; visible frames may be skipped while pressure persists.
3. All controls quarantined BEFORE the project LOADING / SAVING / ERASE status screen is drawn. MIDI keyboard and physical button edges and encoder deltas are consumed during and after the synchronous operation. Inputs remain blocked until the controls are fully released and there have been 320 ms of quiet. Save/load/format/erase state is preserved. The existing SELECT anti-delete fence is retained.
4. Audio ISR records the last **actual** deadline/guard Song position in RC AUDIO LOAD diagnostics (`LAST LATE SONG POS`). Zero means outside Song Mode. The counter helps identify hardware failures; it is NOT a fix or a synthetic readout.

## Validation and limitations
- Both JieLi target configurations built successfully: read-only diagnostic and experimental storage+OTA+USB audio.
- All 32 MOUNTAINKING Song Mode positions host-rendered with UBSan. PCM peaks in the simplified host recreation stay below 16-bit clipping, including position 19. The simplified renderer is **not** the full IRQ/USB/LCD pipeline, so physical device clipping is not ruled out.
- Host NUGGIE 12 oscillator variants and normal 120s synthetic audio test run; existing PCM regression hashes unchanged.
- UI control barrier and LCD governor tests pass, including unsigned millisecond timer wrap.
- All on-device 12 project slots, project serialization schema, global PC `.beetlebank`, factory drums, 8 synthvoice budget and 6 USB stems preserved.
- The new FWSC retains the original protected flash header and OTA-loader bytes, verified offline.
- **No real FM1 DMA deadline measurements or reliable full-flash recovery path are established here.** An FM1 can still miss a deadline under CPU/USB/bus saturation. Do not replace existing safe firmware with an unverified image without backup and an independent proven recovery method.

## Hardware reproduction requested
1. Back up every `.beetle` and verify a *separate* stock recovery path before a candidate OTA. Keep V1.3.8 accessible.
2. Load MOUNTAINKING; read HOME > RC AUDIO LOAD counters (LATE AFTER COPY, BLOCKED BEFORE, WALL MAX, CLIP FRAMES), note their starting values.
3. Play Song Mode through positions 17-21, with the same USB multitrack mode, physical volume and effects that reproduce the crackle.
4. Inspect the same counters, including LAST LATE SONG POS. Rising LATE/GUARD implicates deadline missed. Rising CLIP FRAMES with no deadlines instead points to a gain issue. If neither rises, collect a USB/stereo recording to check transfer underruns and discontinuities.
5. Repeat with USB capture OFF/ON. A difference identifies the USB transfer path as part of the problem. Do not claim a clean run proves all heavy loads safe.

## Source commands
```
export JIELI_TOOLCHAIN=/path/to/jieli-linux-toolchains-20260730.1
python firmware/fm1-native/build.py --hardware-test
python firmware/fm1-native/build.py --beetle-storage-lab --updater --usb-audio
cc -O2 -std=c11 -Ifirmware/fm1-native tests/track/test_v143_io_and_display.c -o /tmp/io_test && /tmp/io_test
bash tools/diagnostics/run_dma_host_tests.sh
python tests/installer/test_v142_dma_installer.py
node tests/installer/test_v142_dma_installer.cjs
```
