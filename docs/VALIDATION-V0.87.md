# BEETLE V0.87 validation

## Software verification

- All three JieLi pi32v2 build variants compile: read-only diagnostic, OTA pilot with USB audio, storage/update with USB audio.
- Storage/update binary: 439136 bytes, code headroom 142428 bytes (from linker build).
- Dedicated V0.87 native regression: black mute / adjacent white solo mapping, Stutter ×1, Ring Mod 20, V17 per-waveform patch roundtrip and V16 migration: PASS.
- Previous V0.84 UI static regression updated for requested SELECT/K1 reversal: PASS.
- Firmware Freedom V0.87: ETCH V1.36/V1.37 checked including simulated CHECK/FLASH/REBOOT and unsupported-image refusal; BEETLE container exact hash verification: PASS.
- Native read-only audit and native package check: PASS. Bootloader and first 16 KiB retain ETCH V1.37 reference bytes; full-flash protection remains unchanged.
- JS syntax check for WebSimulator: PASS. Full interactive browser test NOT completed this run.
- Long full regression run stopped due to an extended browser test / execution timeout. It reached the V0.72 browser export checks; earlier audio, project storage, undo/redo, library, and older migration tests shown as PASS. Do not claim a complete new full-suite pass.

## Fixes found in source review

- MASTER FX selection: SELECT; parameter knobs: K1–K4; SEL enable. Removed stale caption.
- Punch-In K1 controls held FX parameters, pattern ALGORITHM no longer does; Ring Mod default 20 and Stutter BPM ×1 default.
- Black mute keys and white solo keys, including LEDs, in normal and Live Mixer.
- Per TYPE ADSR, oscillator, duty, cutoff and gain now stored independently in four patch slots; V17 format preserves V16 import. Dense saved projects still must fit fixed journal capacity.
- EDIT duty / cutoff previously wrote the selected lane rather than the focused patch index, and EDIT patch volume could change the mixer fader. Both fixed.
- Native and browser HELP end-of-list counter reflects the last visible entry.
- This change has not been physically tested on FM1 in this session. Public beta hardware testing remains a maintainer step.

## Important storage caveat

Physical BEETLE project storage is experimental and initialization can erase previous ETCH/Felucca data at 0x97000–0xDFFFF; never initialize without backup/recovery. OTA image digest and container integrity do not imply recovery safety.
