#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SRC="$ROOT/firmware/fm1-native"
CFLAGS="-std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer"
BIN="${TMPDIR:-/tmp}/track-v03-native-test-$$"
trap 'rm -f "$BIN"' EXIT
clang $CFLAGS -I"$SRC" "$ROOT/tests/track/test_track_v02.c" "$SRC/track_core.c" "$SRC/track_name.c" -o "$BIN"
"$BIN"
clang $CFLAGS -I"$SRC" "$ROOT/tests/track/test_track_v03.c" "$SRC/track_mixer.c" -o "$BIN"
"$BIN"
clang -w -std=c11 -fsyntax-only -fno-builtin -I"$SRC/hal" \
 -DTRACK_FIRMWARE=1 -DETCH_FLASH_READ_ONLY=1 -DETCH_NATIVE_STORAGE=0 \
 -DETCH_NATIVE_OTA=0 -DETCH_NATIVE_UAC=0 -DETCH_NATIVE_UAC_48K=0 \
 "$SRC/bringup.c"
clang $CFLAGS -I"$SRC" "$ROOT/tests/track/test_track_v04.c" "$SRC/track_core.c" "$SRC/track_song.c" -o "$BIN"
"$BIN"
node "$ROOT/tests/track/test_web_v04.cjs"
clang $CFLAGS -I"$SRC" "$ROOT/tests/track/test_track_v05.c" "$SRC/track_chip.c" "$SRC/track_mixer.c" -o "$BIN"
"$BIN"
node "$ROOT/tests/track/test_web_v05.cjs"
node "$ROOT/tests/track/test_web_v07.cjs"
node "$ROOT/tests/track/test_web_v08.cjs"
echo "TRACK V0.8: native source and web code regressions PASS (run build script separately for cross-link audit)"
node "$ROOT/tests/track/test_web_v09.cjs"

clang $CFLAGS -I"$SRC" "$ROOT/tests/track/test_track_v011_drums.c" "$SRC/track_drum.c" "$SRC/beetle_drum_rom.c" "$SRC/track_mixer.c" -o "$BIN"
"$BIN"

# V0.16 fast hi-hat, dual NOTE and sample-variation reliability regression
clang $CFLAGS -I"$SRC" "$ROOT/tests/track/test_v016_drum_reliability.c" "$SRC/track_drum.c" "$SRC/beetle_drum_rom.c" "$SRC/track_mixer.c" -o "$BIN"
"$BIN"

# V0.29 hardware-key note auto-advance and chip/drum nonzero PCM.
clang $CFLAGS -I"$SRC" "$ROOT/tests/track/test_beetle_v029.c" \
 "$SRC/track_core.c" "$SRC/track_chip.c" "$SRC/track_mixer.c" "$SRC/track_drum.c" "$SRC/beetle_drum_rom.c" -o "$BIN"
"$BIN"
