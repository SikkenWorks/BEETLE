# BEETLE V1.4.12 – Song Keys, REC & Battery Validation

## New firmware controls

| Context | REC behavior |
|---|---|
| Tracker / other ordinary pages | Toggle step recording ON/OFF (default ON). Audition still plays when OFF. |
| Step FX / Punch-In | Toggle Punch-In recording only. Leaving these pages ends Punch-In recording without changing tracker REC. |
| Song Chains | Hold SEL and turn K1 to select contiguous rows; SEQ + OP1 copy; SEQ + OP2 paste starting at cursor. |

Song keyboard LEDs show selected CH1–CH5 chord/note voices or CH6 drum triggers. During PLAY the current Song position supplies the source; the cursor selects the channel. UI updates read a bounded number of existing voice structures; no extra ISR DSP or storage access.

## Safety, scope, limitations

- Firmware product identity **FM-1_2123** unchanged from V1.3.8.
- **Battery 20% cannot yet be verified.** Raw FM1 battery ADC is uncalibrated; code uses provisional threshold 550 with low-pass filter and hysteresis. Tune from actual FM1 voltage and remaining-capacity measurements across temperature and discharge. This is not a fuel-gauge reading.
- New Song clipboard uses the pre-existing editor scratch region, not USB/audio ring buffers; preserves 64 row support and no new enlarged stack requirements.
- Saved project schema remains **TRK1 V24**, readable in current firmware; older releases may not read V24.
- New native global user sample storage is still **OFFLINE ONLY**; BD4S prefetch remains host-tested and not wired into physical flash/DMA.
- Firmware write/update, audio deadlines, playback smoothness, LEDs and battery percentage still require on-device testing, with proven independent backup and recovery before installation.

## Regression commands

```bash
bash tests/track/run_v1412.sh
bash tests/track/run_v1410.sh
bash tests/track/run_v13_audio_stability.sh
python tests/dpcm4/run_prefetch.py
JIELI_TOOLCHAIN=/path/to/jieli-toolchain python firmware/fm1-native/build.py --hardware-test
JIELI_TOOLCHAIN=/path/to/jieli-toolchain python firmware/fm1-native/build.py --beetle-storage-lab --updater --usb-audio
```

Native `.fwsc` package is experimental and not validated as safe to install on a physical FM1.
