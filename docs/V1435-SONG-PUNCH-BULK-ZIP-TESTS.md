# V1.4.35 — implementation and test notes

## On the FM1

- Open Punch-In FX. Check eight assigned white keys are illuminated, exactly matching visible FX page. Change page; first eight go dark, second eight light. Playing FX should still work including pressed keys and K1 parameters.
- In SONG CHAINS, short SEL acts as before. Hold SEL for 650+ ms: SONG LOOP ON/OFF toggles. Holding SEL and turning K1 sets the selected range without toggling LOOP. With selected range, PLAY loops only selection. Without a range, SONG LOOP dictates whole-song behavior.
- SAVE/LOAD of V25 projects and user samples are unchanged.

## Project Manager bulk ZIP

- With 1–12 occupied slots, select EXPORT PROJECTS ZIP. The result contains `PROJECTS-MANIFEST.json` and one individually encrypted `.beetle` per occupied slot. Empty slots excluded. Verify physical ZIP download before using it as a backup.
- Select IMPORT PROJECTS ZIP to preflight every archived project: supported uncompressed ZIP structure, central directory, CRC, manifest format, unique destination slots, encrypted project authentication, raw TRK1 CRC and exact original slot binding. The manager does not silently rewrite project slot numbers.
- Present two confirmation dialogs, with a backup acknowledgement before overwriting. Existing individual IMPORT/EXPORT are unchanged; unspecified destination slots are never deleted. The transfer and readback verification occur separately for each slot, so bulk import is not atomic and may stop partway through.
- ZIP contains only project data. Global user samples **U01–U13** are managed/backed up separately. An archive does not include firmware or samplebank.
- Corrupt ZIP, incompatible firmware schema, wrong project identity, interrupted readbacks and unexpected slot changes must stop transfer. Only this manager's uncompressed ZIP format is supported, intentionally; do not promise arbitrary ZIP imports.

## Native/host tests

- `bash tests/track/run_rc1011_song_chains.sh`: passes, including corrected actual V23 legacy-layout fixture. Test fixture was incorrect in baseline; runtime V23 project loader was not changed.
- `cc -O1 -std=c11 -fsanitize=undefined -Ifirmware/fm1-native tests/track/test_v1434_song_selection_loop.c firmware/fm1-native/{track_core,track_song}.c -o /tmp/v1435_song && /tmp/v1435_song`: passes.
- `node tests/project-manager/test_v1435_bulk_zip.cjs`: passes stored ZIP roundtrip, original slots, corruption/duplicate/path-traversal/unsupported compression rejects.
- All scripts in root standalone HTML files: syntax checks pass. Firmware app compiles with JieLi toolchain and USB/V2/updater switches.
- Verified `.fwsc` payload decrypts to built app; protected first 16 KiB and original OTA loader are identical to V1.4.34. Physical FM1 and actual OTA recovery not validated.

## Known baseline regression

The old PCM checksum test (`tests/track/run_rc1011b_audio_stability.sh`) fails on V1.4.34 **before any V1.4.35 changes**. The mismatch is measured `dd0f3681` vs hardcoded `43c0f1f6`. Its numerical baseline should be revalidated separately instead of silently updating the expectation. The V23 migration case, by contrast, passes after correcting the invalid legacy fixture.
