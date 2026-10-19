# BEETLE 1.01 RC1.0.8 — Current Lane Clear

- Hold SEQ + rightmost black keyboard key (F#5, note-matrix bit 25), on TRACKER and while transport is stopped, to open **CLEAR CURRENT LANE**.
- The question is **ARE YOU SURE YOU WANT TO CLEAR THIS LANE?** Choose K1 YES/NO, SEL confirm, HOME cancel. The default answer is NO.
- Only the current pattern and active channel are changed. All 16 rows are cleared, even those hidden by a shorter pattern length. For CH6, NOTE2 and its V18 per-note FX are cleared together with the primary drum step.
- Undo/Redo uses existing pattern snapshot history. No flash writes occur until an explicit project SAVE.
- This is distinct from **PROJECTS → WIPE ALL STORAGE**, which remains unchanged.
- FM1 storage/updater boot header reference and OTA loader are preserved from the previous HF2 release. RC1.0.8 update remains *software-validated only*, not tested on a physical FM1.

## Validation

- Portable C test (`tests/track/test_clear_lane_rc108.c`) verifies all six lanes, all sixteen rows, all NOTE2 FX fields, no cross-lane modifications, and gating when not in tracker, during PLAY/REC or without SEQ.
- The three native JieLi variants (storage/updater, read-only hardware test, pilot OTA) are built using the supplied Linux pi32v2 toolchain; see build logs.
- The WebSimulator has the same YES/NO modal via **CLEAR LANE (SEQ+RIGHT BLACK)** in PATTERN EDIT.
- A valid firmware container and identical embedded package do not prove an OTA update is safe on a physical device.
