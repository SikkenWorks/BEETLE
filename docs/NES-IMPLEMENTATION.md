# NES instruments: implementation and limits

Native engine: `source/firmware/fm1-native/beetle_nes_apu.h` and `track_chip.c`.
Types: 9 NES PULSE; 10 NES TRIANGLE; 11 NES NOISE. Three new types, not three new physical tracks.

- Pulse: the NES 8-step duty sequences (12.5/25/50/75%), fixed-point phase accumulator, existing BEETLE attack/decay/sustain/release envelope.
- Triangle: 32-step quantized NES waveform with 4-bit level steps.
- Noise: 15-bit LFSR using NTSC noise period table; 4-semitone groups select 16 pitches, channel duty selects alternate short feedback tap (via duty>=2).
- No DMC, extra APU register side effects, sweep hardware, precise analog mixing or expansion chips.
- Native renders at 44.1 kHz, shares up to eight BEETLE synth voices. Extra per-voice allocations are zero. WebAudio simulation uses a bandlimited waveform/noise buffer for convenience and is not bit-exact against native DSP.

This is an independent NES-APU-inspired implementation. Nes_Snd_Emu by Shay Green (blargg), fork maintained by James Athey, is credited and its upstream LGPL-2.1 included for attribution. No LGPL source is copied. The resulting firmware remains GPL-3.0-only.

Beta releases are tested on actual FM1 hardware by the maintainer before public GitHub release. Confirm these *new* instrument variants on physical hardware, including RAM/CPU/audio and project save/restore; this session's builds and tests do not assert that the maintainer's future hardware sign-off has happened.
