# BEETLE V1.4.34: SONG selection, playback and UI validation

## User interaction

1. Enter SONG CHAINS (ARP), insert 6 distinct rows (OCT+), edit CH1..DR and REP with K2/K3.
2. Hold SEL on row 2; rotate K1 to row 4; release SEL. Rows 2..4 are all shaded magenta, with a 1px outlined active edit cell, matching tracker selection styling.
3. PLAY starts at row 2; plays row 2 with its own REP, row 3 and row 4 with their own REP; jumps to row 2 endlessly, even if the SONG LOOP setting reads OFF.
4. STOP restores row focus and leaves range visible. PLAY repeats same subset. Tap SEL to clear selection; press PLAY to resume ordinary song play beginning row 1 and respecting SONG LOOP OFF/ON.
5. Reverse selection (SEL pressed on row 5, K1 backwards to row 3); range is 3..5, not 5..3. Same for a single-row anchored selection. Select across pages for chains longer than 8 rows.
6. If SONG was OFF, starting PLAY on a selection enables SONG for selection playback. If selection is not active, normal SONG OFF behavior remains.
7. Verify SEQ+OP1/2 copy/paste still works. Insertion/deletion while stopped clears selection; selection/insert/delete/K3 changes during PLAY are not accepted.
8. A one-row selection must also reset Punch-In and note-off/song-boundary state every new cycle, even though the Song position number is unchanged.
9. Check other screens keep expected tracker step selection, CHD audition, SID, NOTE2, punch FX and USB stereo/multitrack behavior.

## Automated

- `cc -O1 -std=c11 -fsanitize=undefined -Ifirmware/fm1-native tests/track/test_v1434_song_selection_loop.c firmware/fm1-native/{track_core,track_song}.c -o /tmp/song1434 && /tmp/song1434`
- Test covers selected boundaries, reversed order, repeat counts, single row, out-of-bounds input clamping, full-song LOOP ON/OFF and unchanged song-row data.
- Native build: `JIELI_TOOLCHAIN=/path/to/toolchain python firmware/fm1-native/build.py --beetle-storage-lab --beetle-v2 --updater --usb-audio`.
- Verify `.fwsc` package, decompressed image, 16 KiB protected header and original OTA loader against V1.4.33. Verify embedded firmware hash and All-in-One manager payloads.
- `node --check` on HTML JavaScript sections.

## Baseline issue (not introduced by V1.4.34)
`tests/track/run_rc1011_song_chains.sh` fails its final V23 fixture decoding assertion on the identical V1.4.33 source, too. That older test is not counted as a pass; compare baseline if that migration path is to be changed. V25 data layout and encoder remain unchanged in V1.4.34.

## Important limitation
Physical FM1 tests, real-world OTA flashing/recovery and realtime audio stability are unverified; use a confirmed recovery path and sample/project backup.
