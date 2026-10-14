# BEETLE V0.51 · Responsive keyboard + WebSimulator

Complete source for the M-VAVE FM1 experiment, based on V0.50.

- **Shared OCTAVE**: EDIT and TRACKER use the same keyboard octave (-4...+3). Press and release OCT- or OCT+ to move an octave. In EDIT both together still toggle MONO/POLY without shifting the octave. Playing notes from the physical keyboard uses the selected octave on both screens. Existing pattern notes are never transposed by changing the keyboard octave. K3 remains the note/instrument cell editor.
- **Correct note-offs**: live EDIT/TRACKER voices remember the actual MIDI note and release that exact note after changing octave or channel. Held EDIT notes are retriggered under the updated octave.
- **WebSimulator**: `BEETLE-V0.51-WebSimulator.html` is a browser-only simulator, updated to six lanes and 12 project slots. The same shared keyboard-octave control is simulated, and the original animated beetle boot art is included. It uses WebAudio rather than the exact native JieLi DSP, so audio parity with the FM1 is not claimed.
- **Source cleanup**: only current release firmware, latest webinstaller, simulator, source, fixture/reference files, production toolchain scripts, and retained regression tests. Old generated binaries, historical installers, duplicate art and build logs excluded.

## Experimental firmware safety

Hardware is not tested. Even when a C build, OTA container and simulated install pass, the storage firmware can still cause data loss or boot failures. Don't flash without a verified complete dump, working recovery route and knowledge of the risks.

## Build

Set `JIELI_TOOLCHAIN` to directory holding `pi32v2/` and `common/`.
Run `python source/firmware/fm1-native/build.py --hardware-test` for read-only.
The OTA pilot uses `--pilot`; the storage lab uses `--updater --beetle-storage-lab` and requires informed confirmation before destructive initial storage takeover.

Original reSID is not bundled; `RESID-LITE` is the compact native BEETLE SID-style instrument added in V0.50.
