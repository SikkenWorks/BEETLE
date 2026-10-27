# BEETLE V1.4.28 — Streaming Optimization and Tests

The 13-slot global BD4S library still uses one shared sample bank and six drum voices; no storage format or note mappings change.

## Exact changes

1. `beetle_bd4_prefetch.c`: One background read of an entire 134-byte encoded 256-frame block, instead of header+payload. Tail blocks are bounded. CRC/generation checks remain in Storage V2 read path.
2. `beetle_bd4_prefetch.c`: Cache the even-frame source sample pair for the odd interpolated frame; preserve fallback lookups on underruns to recover promptly from a newly refilled window.
3. `track_drum.c`: Skip the factory ROM 64-bit interpolation on global samples already rendered as 44.1 kHz PCM.
4. `tests/dpcm4/test_prefetch_stream.c`: Repeat 64 loops x six voices, verify every PCM frame and ensure the renderer never reads storage. Keep existing long and bounded-underrun tests.

## Reproduce host tests

```sh
python tests/dpcm4/run_prefetch.py
python tests/dpcm4/run_v1422_live.py
bash tests/track/run_v1427_fx.sh
```

Build the JieLi target (external toolchain required):

```sh
JIELI_TOOLCHAIN=/path/to/jieli-linux-toolchain python firmware/fm1-native/build.py --beetle-storage-lab --updater --usb-audio --beetle-v2
```

## Hardware acceptance test still required

Play a 16-step pattern with six simultaneous global sample triggers, NOTE1+NOTE2, 120–180 BPM, several repeats, then enable CPU-heavy synth chords, MASTER FX and USB six-stem recording. Confirm stable tempo, no sound dropouts, no DMA STOP, and test repeated restarts and project reloads. Capture and report hardware audio and logs. A software test cannot demonstrate these properties on FM1 hardware.

Before flashing experimental firmware, back up all projects and `.beetlebank` content and independently confirm a recovery method. OTA protected loader identity does not prove OTA/recovery reliability on a physical device.
