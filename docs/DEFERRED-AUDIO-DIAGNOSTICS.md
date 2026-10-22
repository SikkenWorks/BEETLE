# Parked for later: BEETLE Audio Diagnostics & Stability

The following investigation was explicitly postponed on 2026-10-21 to prioritize user samples. **Not implemented in this stage.**

- Capture missed DMA deadlines, worst-case audio block time and active voices (including Song Mode position and step).
- Detect USB stereo/multitrack ring underruns, overruns and frame discontinuities.
- Detect internal audio discontinuities, voice stealing and abnormal envelope transitions.
- Export diagnostics via an explicit **read-only, negotiated** USB-MIDI protocol in the Project Manager.
- No heavy logging, flash writes or USB transfers from the audio DMA ISR. Validate low overhead on FM1 hardware.
- Use Kokiri USB stereo capture and MOUNTAINKING Song position 19 as regression scenarios.

This is a recorded future work item, not a claim that missing DMA/audio issues have already been fixed.
