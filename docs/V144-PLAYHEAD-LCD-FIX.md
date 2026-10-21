# BEETLE V1.4.4 — Playhead LCD regression repair

**Candidate only: no on-device FM1 run or OTA recovery proof.**

## Root cause

V1.4.3 introduced `beetle_lcd_min_interval()`: 125/210/380 ms minimum intervals for **whole-screen** repaint under increasing audio pressure. The native 156 BPM playhead advances on a 16th note every approximately 96.15 ms. Since V1.4.3 tied actual playhead updates to full-screen repaints, multiple steps were silently skipped by the LCD even though the audio engine and transport kept running.

## Source changes

1. `firmware/fm1-native/track_runtime.h`: delta-based playhead painting. For each observed tracker step, rerender only the former and new 13-pixel row bands with the original grid painter, plus one compact 22x23 icon blit. For Song Mode positions within one 8-position view, highlight rows are updated as 15-pixel bands. Song page crossings and content changes continue to request a complete refresh.
2. `firmware/fm1-native/beetle_playhead_update.h`: pure geometry and step-change predicates used in native code and tests.
3. `tests/track/test_v144_playhead.c`: boundary, tempo, heavy-pressure and geometry regression tests.
4. Full-screen paint now takes one main-thread `play_step` snapshot across both LCD halves. Interrupts are not blocked for rendering. The main input poll remains 18ms, but physical LCD latency depends on SPI/CPU scheduling.

Tracker normal full screen = 2x240x120 RGB565 = 115200 payload bytes. Two strips plus icon = 2x240x13x2 + 22x23x2 = 13492 bytes; about **8.5 times less** SPI pixel payload per step. This is an estimate based on transferred pixel bytes, not measured wall time on the FM1.

## Deliberate constraints

- No SPI writes or long interrupt masking from ALNK0 (audio ISR).
- No change to DSP outputs, oscillator resolution, instruments, voices, sample RAM, USB endpoints, project file encoding or fixed FM1 flash map.
- All-control quarantine remains active for READ/WRITE/VERIFY project screens.
- Existing 125–380ms limiter remains for truly large LCD redraws. It no longer suppresses incremental tracker step updates.
- At high load there is still a physical limit: an LCD frame or DMA deadline can be missed if interrupt/task scheduling cannot keep up. A verified zero-dropout claim requires actual FM1 hardware measurement.

## Software verification

- JieLi read-only and storage/OTA/UAC builds: passed.
- `cc -O2 -std=c11 -Ifirmware/fm1-native tests/track/test_v144_playhead.c -o /tmp/playhead && /tmp/playhead`: passed.
- Existing storage input-lock host regression: passed.
- PCM matrix: `df02ac8b`; 120-second audio soak: `1183fe60`, unchanged.
- MOUNTAINKING all 32 Song positions host-rendered. The original position-19 crackling remains **unproven fixed** on physical FM1.
- Firmware packaging retains existing protected first 16 KiB, identical OTA loader and verified CRC/container structure.

## Real-hardware check

Use only with a verified complete backup **and independent recovery path**. At 156 BPM play through MOUNTAINKING positions 17–21 in Tracker and Song Chains, both with USB multitrack OFF and ON. Record playhead motion at high frame rate; every active step should highlight in order. Check `HOME > RC AUDIO LOAD` for LATE, GUARD and LAST LATE SONG POS. If those increase, a deadline problem remains. Do not infer real-time safety from these host tests.
