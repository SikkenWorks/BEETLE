# BEETLE RC1.0.11b — Song Chains audio-stop hotfix

**Source:** BEETLE 1.01 RC1.0.11 user-supplied complete project ZIP.
**Affected hardware report:** audio engine stops/restarts repeatedly near Song Chain position 50, and incorrectly shows the ETCH PLAY/AUDIO STOP screen.

## Actual source findings

1. `audio_target.c` previously shed `native_audio.voice_limit`, which belongs to ETCH DSP and does not limit the active BEETLE `track_chip_render()` synth pool. After repeated missed DMA deadlines, `native_budget_late()` therefore failed to lower actual tracker compute cost and could trigger automatic audio stops. Corrected to bound BEETLE `track_chip.realtime_voice_budget` (8 down to 4; gradual recovery).
2. `track_chip_render()` previously recomputed whether any of six channels was soloed for every voice, every sample. Replaced with equivalent cached per-render mix gating/levels. Reference and patched stereo PCM checksums match exactly without overload.
3. `track_song_compose()` previously performed per-step modulo and assignment even for the common 16-step case. It now uses contiguous copies on full-length patterns while preserving 1–15 step repetitions, silent lanes, and independent CH6 NOTE2 FX.
4. `screen_render()` previously fell through to the ETCH UI when `native_tripped`. It now shows an explicit BEETLE AUDIO STOP recovery page. Automatic/manual restarts reset tracker voice/drum DSP without touching stored song/project data.

## User project inspection (not redistributed)

The AES-GCM-authenticated user `.beetle` backup was decrypted for **local diagnosis only** with the publicly embedded V2 compatibility key. Its CRC-valid payload has format V19, 64 song rows, BPM 122, and 12,207 bytes. Native C decoding and all 64 composite song-row transitions succeed. Song position 50 has 48 authored note events; position 52 has 50 (counted over each full 16-step lane). All references are within 48 patterns or the valid `--` silent sentinel. The backup was **not** modified or included in any release ZIP. Dense note activity is consistent with increased DSP load, but physical per-block time was not measured.

## Tests

- Native JieLi storage+OTA firmware: **PASS** linker and app image audit.
- Native JieLi read-only firmware: **PASS** linker, app audit, read-only symbol audit.
- Native JieLi OTA pilot: **PASS** linker and app image audit.
- `tests/track/run_rc1011b_audio_stability.sh`: **PASS** 64-row synthetic SONG CHAINS, 4–8 adaptive voice limit, 32 consecutive render chunks, PCM bit equality versus unmodified RC1.0.11 (FNV1A32 `2ccc77fc`), V19 migration and NOTE2 FX isolation.
- FM1 package structural CRCs, protected 16 KiB header, original trusted OTA loader, decoded APP bytes: **PASS**. Installer must embed the exact released package with pinned SHA-256.

## Not yet proven

- Physical FM1: actual CPU saturation reduction, continued playback without audible crackles, behavior throughout full ~64-row user song, and repeat OTA recovery have **not** been demonstrated.
- The emergency limiter may temporarily steal lower-priority synth voices under sustained overload, rather than stopping the whole audio subsystem. Its sonic acceptability requires testing on the device.
- A different cause of on-device DSP overload could remain. If the problem persists, save the hardware read-only audio guard/late diagnostic counters, firmware identity and approximate song position.

**Flash advice:** physical recovery and verified backup first; `.fwsc` validity is not physical boot proof. No FM1 Recorder or experimental three-FX firmware changes in this hotfix.
