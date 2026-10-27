# BEETLE V1.4.27 Punch-In FX testing

This release changes only CRY labeling and the native and simulator BITCRUSH and RADIO DSP. Project format remains TRK1 V25; FX IDs and all 13 global sample labels/storage behavior stay unchanged.

## Manual physical FM1 acceptance test

1. Back up `.beetle` projects and the `.beetlebank`. Confirm a genuinely working recovery route **before** experimental flashing.
2. On FX page 2, hold the former SCRATCH key: it must now display **CRY**, with identical bidirectional sound and adjustable K1 setting. Recorded FX must show CRY in the event editor.
3. Test BITCRUSH on a bassline, a quiet sustained high note, and drums. Hold FX 10. K1 50 should give an unmistakably granular/downsampled tone; turning up should audibly increase degradation. Releasing should restore dry audio without clicks.
4. Test RADIO on a bass-heavy pattern. Hold FX 15: the bottom end should be cut while melody and upper percussion are retained with light hiss. K1 should increase lowcut and hiss. Releasing must restore bass.
5. Test 16 FX switching and last-held-key page focus, NOTE2 U01–U13, six-note polyphony, long song loops, full MASTER FX and USB multitrack for clipping, DMA misses, stuck sound and glitches.
6. Test successful OTA update and return using a separately verified recovery path. Passing software package hashes is not hardware OTA validation.

## Native regression

```bash
bash tests/track/run_v1427_fx.sh
```

Expected: audible PCM changes at low input amplitude, RADIO 200 Hz attenuation stronger than 3 kHz, stable low-level noise and V1.4.26 NOTE2 legacy codec pass.

## Constraints

- Shared 12 KiB Punch-In history unchanged; no new persistent PCM buffers.
- Bitcrush state: two held PCM values, one counter, one crossfade value.
- Radio reuses existing per-channel highpass state with one crossfade byte.
- Storage V2 layout, project serialization V25 and loader unchanged.
- No physical FM1 DMA, acoustic or real USB tests have been performed here.
