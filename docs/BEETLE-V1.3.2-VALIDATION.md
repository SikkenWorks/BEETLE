# BEETLE V1.3.2 — Source-level validation

**Status:** experimental software, not tested on an actual M-VAVE FM1.

## Diagnosed bugs

1. **EDIT patch redirection.** `track_edit_prepare_patch()` auto-copied any patch referenced by a saved note to the next available slot and silently changed `track_input_instrument[channel]`. The tracker references were not changed, so later audition and sequencer playback could use different patches. The matching WebSimulator had the same allocator. This is removed. INST 1–8 now have stable identities, and EDIT focuses the instrument explicitly assigned to a selected saved note. All eight patches can be edited directly, even when every patch is used.
2. **Live audition mismatch.** Playback read per-instrument duty/cutoff, but parts of keyboard/EDIT audition could use the last per-lane DSP values. All note-on paths now apply duty/cutoff for the requested instrument. Per-instrument VOLUME is applied to velocity at note-on, with the previous factory default 65 as unity to avoid changing factory loudness.
3. **RETRO WAVE grain/stepping.** The original 32-position wavetable was read as abrupt 4-bit steps. Linear phase interpolation between adjacent wavetable entries reduces high-frequency stepping artifacts. The source waveform and 12 available oscillator types are unchanged. A different native PCM checksum is expected.
4. **OCT edit limitation.** Old direct OCT keys changed the live keyboard octave, while stored notes required SEQ+OCT and only one step. V1.3.2 uses one bounded note-transposition helper for a single cell or an inclusive SEL+K1 selection. Each valid root note shifts by one octave; empty/STP cells, other lanes, chord settings and drum notes do not change. Group edit uses one undo transaction. The SEQ+OCT combination is retained.

## Factory defaults

| INST | Default TYPE |
|---|---|
| 1 | RETRO PULSE |
| 2 | RETRO WAVE |
| 3 | VA OSC |
| 4 | VA OSC2 |
| 5 | VA BASS (VA OSC with sub-octave B and short envelope) |
| 6 | VA PAD (VA OSC2 with softened detuned oscillators, slow ADSR) |
| 7 | SID 6581 |
| 8 | NES PULSE |

INST 5 and 6 have their own factory oscillator mixes, envelope settings and filter cutoffs. These are initial editable patches, not the twelve oscillator types and not the eight project-local Instrument Library preset slots. CH6 remains the independent drum engine.

## Verification

- Native JieLi pi32v2 build and app-region audit, remaining headroom, no linker error.
- V20 encoder/decoder; historical V19 file migration; 64 Song Chains positions.
- Song Mode Punch-In and fresh-start ghost-effects checks.
- Stereo panning and polyphony RMS regression checks.
- V1.2 USB audio SRC ring drift and 12 wave-type RMS regression tests.
- Native C octave tests: inclusive reversed ranges, +12 and -12, clamp at 0/127, empty/STP, preserve independent instrument IDs and nonselected lanes.
- Native HELP twelve chapters compared line-for-line with both simulator files; simulator JavaScript syntax checked.
- `.fwsc` app image, original first 16 KiB and OTA loader bitwise preserved; embedded installer product/hash/package validation.

### Limits

No USB capture sample from a physical FM1, no hardware-observed USB audio host enumeration, no real-world Retro Wave audition, no crash/recovery test and no measured physical CPU load. Sound improvements and reliable flashing are therefore not hardware-certified. Experimental storage can overwrite data; verified full-flash backup and an independent recovery route are needed before installation.

The project schema remains **V20**. Older V19 projects may be loaded, but V20 files cannot be loaded by old V19 firmware.
