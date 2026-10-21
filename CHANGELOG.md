## V1.4.5 — Playhead Ghost Fix (hardware-unverified)

- Fixed the V1.4.4 repaint synchronization regression reported in `IMG_2437.mp4`: the native main loop incorrectly recorded `transport.play_step` **after** a potentially long full LCD SPI transfer, not the step frame actually sent. When the audio ISR advanced meanwhile, old magenta step highlights could remain visible indefinitely.
- The full-frame UI now records `track_ui_step_snapshot` and `track_ui_song_pos_snapshot`, both latched before LCD rendering, and the next incremental update always repaints the real previously painted row.
- Song Chain highlight snapshots are also stable across both dirty-strip transfers; the grid FX source follows the song-position snapshot.
- Kept both 240x13 dirty-strip row updates and animated 22x23 play icon, the previous version's read/write button lock, audio/DMA fixes, and all existing project functionality. No synth DSP or project file format modifications.
- New tests reproduce an interrupt-driven step advance during full-frame LCD painting, then verify only the current step is highlighted for 64 loops, overlap, 16-step wrapping, and nonadjacent jumps; production source snapshot assignment is checked.
- Offline: native JieLi builds, unchanged PCM matrix and 120-second stress hash, original playhead geometry, and input-lock regressions. Firmware/installer SHA and protected loader checked. **No on-hardware LCD/DMA verification and no safe recovery guarantee.**

## V1.4.4 — Per-step Playhead LCD Fix (hardware-unverified)

- Fixed V1.4.3 regression: the 125–380 ms full-screen LCD throttle could skip three or more playhead steps at 156 BPM. The full-screen LCD refresh and the playhead movement no longer share the same throttle.
- Native Tracker now renders **two 240×13 pixel dirty bands** (previous and current step) plus a compact 22×23 pixel beetle animation on every observed step transition. Only the LCD transfer size and frequency change; no extra work is added to the audio ISR.
- Native Song Chains highlights update as two 240×15 bands while the position remains on the same 8-row page. A normal complete repaint is retained for page changes and source-pattern/content changes.
- The full UI raster path uses one latched step for both halves, preventing self-inconsistent two-half playhead frames if ALNK0 advances mid-SPI transfer.
- Partial rasterization skips non-intersecting tracker rows so each strip does not recalculate every value.
- The existing project save/load all-control input lock, DMA diagnostics, audio DSP, 12 on-device project slots, drum kit, samplebank prototype and USB implementation remain unchanged.
- Tests: JieLi readonly and USB/OTA builds, native 120-second 8-synth/6-drum stress regression, NUGGIE matrix, MOUNTAINKING 32 Song positions, new high-load playhead cadence/dirty-geometry host test, existing storage input-barrier test, FWSC CRC/header and installer integrity tests.
- **Software validation only. Physical FM1 LCD frame cadence, SPI contention and audio DMA under high load have not been measured. This does not prove the firmware is glitch-free or safe to flash without a verified recovery path.**

## V1.4.3 — MOUNTAINKING Stability Lab / Input Quarantine

- Added full-button, musical-key and encoder quarantine across project READ/VERIFY, WRITE/VERIFY and erase/provisioning operations. 320 ms of released/quiet controls required before returning to the editor.
- Added conditional audio-pressure LCD draw governor and null-FX DSP bypass without reducing synth polyphony or changing PCM matrix regression output.
- Added LAST LATE SONG POS to the RC AUDIO LOAD screen so the real hardware can identify positions associated with genuine DMA deadline misses.
- Host Song Mode playback analysis of MOUNTAINKING positions 1–32 (V20, 156 BPM), especially the 18→19 transition. No PCM clipping in the simplified host model; hardware crackling root cause remains to be confirmed.
- Native builds and FWSC package validated offline, firmware embedded/pinned in current WebInstaller. **Not hardware certified, no zero-underrun guarantee.**

## V1.4.3 — MOUNTAINKING Stability Lab / Input Quarantine

- Added full-button, musical-key and encoder quarantine across project READ/VERIFY, WRITE/VERIFY and erase/provisioning operations. 320 ms of released/quiet controls required before returning to the editor.
- Added conditional audio-pressure LCD draw governor and null-FX DSP bypass without reducing synth polyphony or changing PCM matrix regression output.
- Added LAST LATE SONG POS to the RC AUDIO LOAD screen so the real hardware can identify positions associated with genuine DMA deadline misses.
- Host Song Mode playback analysis of MOUNTAINKING positions 1–32 (V20, 156 BPM), especially the 18→19 transition. No PCM clipping in the simplified host model; hardware crackling root cause remains to be confirmed.
- Native builds and FWSC package validated offline, firmware embedded/pinned in current WebInstaller. **Not hardware certified, no zero-underrun guarantee.**

## V1.4.2 DMA Patch 2 — WebInstaller package fix

- Replaced the embedded **V1.3.3** firmware bytes (previously incorrectly distributed as current) with the verified *offline* V1.4.2 DMA Patch 2 `.fwsc`.
- Pin SHA-256 to `a82ddfd02c1f26ee1923fff59ecd864eef8b7cdd806a75f2aa0c403a9089b6aa`, check 610,086-byte length and exact `FM-1_2123` product identity. The protected 16 KiB header and OTA loader remain byte-identical to the original V1.4.1 reference.
- Both root `WebInstaller.html` and `web/WebInstaller.html` are byte-identical. Original USB-MIDI flow, checks, source-identity allowlist, restore/Firmware Freedom policies and all other app components retained. An on-device ID match is not a cryptographic post-flash verification because V1.4.1 shares the identity.
- Installer audit and negative tests verify rejected tampered packages, wrong SHA-256 and wrong product ID *before USB*. The host audio matrix and NUGGIE 64-loop tests remain part of the release.
- **Native firmware itself is unchanged from DMA Patch 2.** Host tests and successful JieLi compilation do **not** establish a durable fix for FM1 DMA DEADLINE; don't treat this candidate as a stable flash release.

# BEETLE V1.4.2 DMA Patch 2

Adds PCM-identical hot-path caching for lane vibrato selection, per-lane drive and gain; a 64-loop NUGGIE native host reproducer for 12 oscillators; both JieLi build variants and integrity-verified experimental FWSC. The global PC sample library is unchanged. Real FM1 DMA deadlines and recovery remain UNVERIFIED. Full change and limitations: `docs/DMA-V142-PATCH2-REPORT.md`.

---

# BEETLE V1.4.2 DMA LAB PATCH 1 (prototype, hardware unverified)

This candidate contains **native DSP performance corrections** tested against the NUGGIE V22 project; see `docs/DMA-V142-HOST-AUDIT.md` for exact test data, resource limits, precautions and remaining hardware acceptance requirements.

No physical FM1 deadline-free claim, no new global-bank device storage, and **no hardware-certified firmware release**. The desktop Global Sample Library is retained unchanged.

---

# BEETLE Changelog

## V1.4.1 — Stereo USB and RETRO PULSE tail (experimental)

- Clock drift correction for **stereo USB** moved out of 1ms packet assembly into the native DMA render boundary and filtered, minimizing packet-to-packet speed modulation. Six-stem USB algorithm unmodified.
- RETRO PULSE 4-bit DAC release amplitude crossfaded to continuous ADSR over 128 frames; held-note PCM remains bit-identical.
- Project Manager incorrect **0.37s** tooltip fixed to correct **0.28s** shared for all 13 one-shot keys (6,144 bytes). No unsupported sample storage extension.
- Confirmed safe firmware switching via compatible-package audit or independent UBOOT loader; no universal flash guarantees.
- Full and read-only JieLi builds, original protected boot and OTA assets, software USB clock model and sample-codec regression verified. No physical FM1 verification.


## V1.4.0 — Pre-reSID User One-Shots (EXPERIMENTAL)

- Based on V1.3.9, with original 14 drum mappings untouched. CH6 NOTE1 extends to 27 notes; keys 14..26 access thirteen user slots.
- Native fixed 6,144-byte project-owned companded PCM sample bank in preallocated RAM; no change to protected flash partitions; V23 optional sample suffix; V22 retained for sample-free projects.
- Project Manager WAV import, sample naming and key assignment, read/write and export of encrypted .beetle files, guarded USB project transfer, explicit sample-free V22 downgrade export.
- Fixed native project encoder's unsafe `memcmp` over unused packed-bitfield padding; deterministic semantic-step equality replaces it.
- Preserved stereo/USB six-stem clock path, oscillator volume calibration, full pre-reSID SID-lite, sustain display `1.00`, original credits and licenses.
- Package/host tests: see V1.4.0-VALIDATION.md. Physical FM1 tests NOT performed.

## V1.3.9 — USB multitrack improvements

- Bounded filtered USB clock correction and equivalent 32-bit six-stem interpolation; pre-reSID engine unchanged. Physical FM1 recording not validated.

## V1.3.8 — Pre-reSID baseline

- Last original firmware prior to native reSID and ymfm development; six tracker lanes, twelve oscillator types including SID-lite, fourteen factory drum sounds, and original native project storage.

For earlier release notes, consult the previous archived releases and Git history. The original authorship notices and licenses remain available in LICENSE, LICENSES and docs.

---

# Historical complete release notes (through V1.3.3)

# BEETLE Changelog

## V1.4.1 — Stereo USB and RETRO PULSE tail (experimental)

- Clock drift correction for **stereo USB** moved out of 1ms packet assembly into the native DMA render boundary and filtered, minimizing packet-to-packet speed modulation. Six-stem USB algorithm unmodified.
- RETRO PULSE 4-bit DAC release amplitude crossfaded to continuous ADSR over 128 frames; held-note PCM remains bit-identical.
- Project Manager incorrect **0.37s** tooltip fixed to correct **0.28s** shared for all 13 one-shot keys (6,144 bytes). No unsupported sample storage extension.
- Confirmed safe firmware switching via compatible-package audit or independent UBOOT loader; no universal flash guarantees.
- Full and read-only JieLi builds, original protected boot and OTA assets, software USB clock model and sample-codec regression verified. No physical FM1 verification.


## V1.4.0 — Pre-reSID User One-Shots (EXPERIMENTAL)

- Based on V1.3.9, with original 14 drum mappings untouched. CH6 NOTE1 extends to 27 notes; keys 14..26 access thirteen user slots.
- Native fixed 6,144-byte project-owned companded PCM sample bank in preallocated RAM; no change to protected flash partitions; V23 optional sample suffix; V22 retained for sample-free projects.
- Project Manager WAV import, sample naming and key assignment, read/write and export of encrypted .beetle files, guarded USB project transfer, explicit sample-free V22 downgrade export.
- Fixed native project encoder's unsafe `memcmp` over unused packed-bitfield padding; deterministic semantic-step equality replaces it.
- Preserved stereo/USB six-stem clock path, oscillator volume calibration, full pre-reSID SID-lite, sustain display `1.00`, original credits and licenses.
- Package/host tests: see V1.4.0-VALIDATION.md. Physical FM1 tests NOT performed.

## V1.3.9 — USB multitrack improvements

- Bounded filtered USB clock correction and equivalent 32-bit six-stem interpolation; pre-reSID engine unchanged. Physical FM1 recording not validated.

## V1.3.8 — Pre-reSID baseline

- Last original firmware prior to native reSID and ymfm development; six tracker lanes, twelve oscillator types including SID-lite, fourteen factory drum sounds, and original native project storage.

For earlier release notes, consult the previous archived releases and Git history. The original authorship notices and licenses remain available in LICENSE, LICENSES and docs.
