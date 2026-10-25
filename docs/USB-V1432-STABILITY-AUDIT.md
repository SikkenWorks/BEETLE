# BEETLE V1.4.32 — USB audio capture recovery

## Source findings (V1.4.31)
- `usb.c` used a 512-frame ring but primed only 104 frames: a delayed 128-frame audio DMA may exhaust that reserve during long playback.
- On underrun, the USB packet filled the rest with `uac.last` / `ua_stem_last[]`, creating audible held-waveform/DC segments; it kept its old clock phase and simply continued.
- On overrun, producer skipped an entire PCM block, with no resynchronization of read/write positions.
- Stereo used a faster SRC adaptation formula than six-stem recording. At frame-level, large correction transients can sound like pitch drift.
- These are deterministic source findings, not a diagnosis of whether a particular physical FM1 has a DMA, USB host or clock issue.

## Changes
- Same 512-frame rings and PCM formats. PRIME: 44.1 kHz 104 → 224; optional 48 kHz 113 → 244. Startup latency +120/44100 s ≈ +2.72 ms. Memory usage for PCM rings unchanged.
- Both 44.1 kHz modes use `beetle_uac_mt_filter_fill` + `beetle_uac_mt_next_step` at native audio-render boundaries. All six stems share a single Q16 phase.
- Outgoing packet is faded to zero only on actual starvation, sets `go=0`; next native render primes silence, zeros last sample, resets fractional position, and fades in fresh audio for 24 frames.
- Producer overflow also sets `go=0` and stops writing that render until the next render primes. A bounded capacity helper guards corrupted/stale ring distances.
- `uac.recoveries` counter is shown as `RECOV` in SYSTEM USB AUDIO. `uac.missed` still exists in code but is not displayed on this compact page; UND/OVER/STALL remain.

## Validation and limits
- Host simulations: 600 s ideal drift at codec 44,117.6Hz; 360 s injected DMA scheduling delays + 22ms host stalls; validate no accumulated invalid buffer positions and recovery events.
- Native build uses `--beetle-storage-lab --updater --usb-audio --beetle-v2`; app ELF audit and firmware protected asset equality are mandatory.
- **Host model isn't the physical 2kHz Timer5/USB SOF/DMA ISR.** Long USB captures with actual patterns + USB multitrack + samples require device tests. No USB audio gap can be prevented if the DSP misses too many deadlines, and resynchronization still creates a bounded dropout.
- Firmware is experimental. Back up projects and user samples and confirm a recovery method before testing.
