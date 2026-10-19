# BEETLE 1.01 RC1.0.7 · Inclusive tracker selection fix

- **SEL-down** now latches the current tracker row as the start anchor before K1 moves.
- **SEL + K1** expands the selection in either direction and always includes the original row and final row.
- **SEQ + OP1** copies all highlighted rows including the first; paste keeps the pre-existing behavior.
- Repeating SEL+K1 starts a new range at the newly selected cursor row.
- Simulator applies the same press-time anchor logic, and HELP describes the inclusive selection.
- Preserves V18 projects, independent drum NOTE1/NOTE2 FX, 12-slot storage, and HF2 OTA installer transport.
- Build identity `FM-1_2107` distinguishes RC1.0.7 from RC1.0.6 during post-update verification.

Tests: 256 start/end combinations of the C selection and lane-copy routines, V18 native regression, native builds and firmware package audits. Not validated on physical FM1 hardware. Firmware flashing and flash-backed storage remain experimental; use independent backups/recovery.
