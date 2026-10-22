# BEETLE V1.4.23 — Legacy project import and conversion

## User experience

1. Connect an FM1 running BEETLE with Project Transfer. The Project Manager scans all occupied slots and reads their project schema versions.
2. For V1–V23 projects on a V24 device, a LEGACY badge and conversion offer appear. Bulk conversion first downloads a ZIP with original encrypted backups of *all* selected slots, requiring confirmation before any write.
3. Each project is sent back to the same slot, decoded and re-encoded as V24 by the native FM1 codec; the result is read back, CRC checked and schema verified before the next slot is changed. Failed uploads stop the conversion. A currently active or busy slot is refused by the FM1, not overwritten.
4. Importing a `.beetle` file using an older TRK1 project schema or legacy password container likewise triggers conversion consent. After a successful commit it generates a new password-free `.beetle` backup from the newly stored V24 project, leaving the uploaded legacy file untouched.
5. The Tracker omits the `REC ON` / `REC OFF` text, but the REC button and LED still work. FX/Punch REC labels remain visible.

## Scope and limitations

Project file schema V24 is not the same as the physical Storage V2 partition. This feature **does not** silently transform a legacy flash layout into the V2 layout, and it does not enable global samples on a legacy partition. All existing global sample code remains included; the native DPCM4 playback and OTA still require FM1 hardware validation.

## Tests

- Full native JieLi FM1 build and firmware package integrity, product identity FM-1_2123 and protected header/OTA loader byte identity.
- Node JavaScript syntax checks for Project Manager and simulator.
- ZIP export roundtrip with two original encrypted-file fixtures.
- Existing V1.4.11 Song Chain/drum loop and Storage V2 coexistence regression tests.
- Browser UI navigation not end-to-end hardware tested, and Web MIDI conversion needs physical FM1 confirmation.
