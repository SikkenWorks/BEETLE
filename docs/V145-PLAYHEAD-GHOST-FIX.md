# BEETLE V1.4.5 — Video-confirmed playhead ghost / LCD snapshot fix

**Status: compiled / host-validated prototype, not a physically verified release.**

## Evidence

The user-provided `IMG_2437.mp4` demonstrates magenta horizontal playhead marks remaining on older steps while the playhead continues to move. The previous V1.4.4 partial renderer was broadly functional, but a race existed specifically in the full redraw path.

## Root cause in production source

In `track_fm1_draw()` the UI saves `track_ui_step_snapshot = track_state.transport.play_step` **before** starting two 240×120 LCD transfers. The oscillator/audio ISR can advance the song/step while those transfers take place. After `screen()` returned, V1.4.4 erroneously assigned `painted_step = track_state.transport.play_step`, not `track_ui_step_snapshot` (the frame actually drawn). Therefore an already stale row appeared to have been cleared, and later delta updates started erasing the wrong old row. This produces persistent magenta ghosting. Song Mode had the analogous `painted_song_pos` mismatch.

## Changes

- The post-`screen()` bookkeeping now stores **the frozen LCD step and song position**, never the live transport state after rendering.
- `track_fm1_draw()` latches Song Mode position at the same time as its step snapshot.
- Song Mode's focused row, live highlight and the grid's FX-bank source use the same snapshot during rendering.
- The Song Mode two-strip painter uses a consistent snapped position across both transfers.
- Partial grid painting still transfers only two 240×13 bands and the small animated icon. No additional audio ISR work, full-frame redraws per step, new flash writes, sample buffers, voices or compression.

## Tests

`tests/track/test_v145_playhead_ghost.c`: 240×240 software framebuffer, simulation of an ISR stepping *during a full LCD frame*, compare next display to a complete expected repaint, 64×16 continuous steps, 16-to-0 wrap, overlapping highlights, and nonadjacent jumps. All must leave a single magenta row. This models paint logic; it cannot simulate LCD bus errors, panel timing or camera artifacts.

`tests/track/test_v145_playhead_source.py`: inspects the native `if(do_full)` branch and fails if the post-SPI ISR read is used again instead of the committed snapshots.

Preserved: V1.4.4 playhead geometry test, V1.4.3 project read/write input quarantine, 120-second eight-synth/six-drum PCM soak and the baseline oscillator matrix. Two JieLi configurations compiled. The firmware `.fwsc` is repackaged with original protected first 16 KiB and unchanged OTA loader. The WebInstaller pins the exact new `.fwsc` by SHA-256, and does not prove flash safety.

## Hardware verification (not performed here)

At 156 BPM, repeat 16+ loops of the same pattern. Observe that no earlier rows retain playhead pink; verify Pattern/Song transitions and go back from Song Mode to Tracker. Repeat with USB multitrack active and heavy audio. Check `RC AUDIO LOAD` counters and whether `LAST LATE SONG POS` increases. This branch does **not** assert a zero-underrun engine or a verified independent recovery route. Avoid deploying without backups and a proven independent rescue method.
