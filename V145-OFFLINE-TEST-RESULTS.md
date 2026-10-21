# BEETLE V1.4.5 GHOST FIX — Offline validation

- Video: `IMG_2437.mp4` shows persistent previous magenta playhead positions. Confirmed software race: V1.4.4 recorded the LIVE ISR step after a long LCD transfer rather than the frozen frame's displayed step.
- Native fix: `painted_step=track_ui_step_snapshot; painted_song_pos=track_ui_song_pos_snapshot;` after full transfer; consistent Song Mode render snapshot, unchanged narrow incremental row updates.
- Native JieLi read-only build: PASS.
- Native JieLi OTA+USB+experimental project storage build: PASS.
- V1.4.5 240x240 model: ISR changes step mid-full frame, 64×16 loops, highlights, wrap and jumps: PASS.
- Production code latch assertions: PASS.
- Original V1.4.4 playhead geometry: PASS.
- Project read/write input lock regression: PASS.
- Audio host 120s / 44.1kHz / 8 synth + 6 drum / all 12 types and 5 VA waveforms: PASS, output checksum 1183fe60 unchanged.
- PCM matrix: PASS, checksum df02ac8b unchanged.
- Desktop global sample library: PASS.
- FWSC exact product FM-1_2123, protected first 16KiB, unchanged OTA loader, package integrity: PASS (offline).
- WebInstaller embedded equals standalone .fwsc, pinned SHA-256, old/tampered files refused by JS test: PASS.
- Complete repository: all V1.4.4 archive entries retained.
- Physical FM1 LCD, DMA deadlines, audio crackle and recovery path: NOT TESTED / NOT PROVEN.

The original FM1 cannot be assumed to be safe to flash merely because these checks pass. The previous MOUNTAINKING Song Mode position 19 crackle is not proven fixed. This test report is for development only.
