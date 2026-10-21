#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
W="$(mktemp -d)"; trap 'rm -rf "$W"' EXIT
C='-O1 -g -std=c11 -fsanitize=undefined -fno-omit-frame-pointer -Ifirmware/fm1-native'
FULL='firmware/fm1-native/track_core.c firmware/fm1-native/track_song.c firmware/fm1-native/track_chip.c firmware/fm1-native/track_drum.c firmware/fm1-native/beetle_drum_rom.c firmware/fm1-native/track_mixer.c firmware/fm1-native/track_project_store.c'
for x in v135_independent_fx v135_fx_pool_atomic v134_block_multi; do
 cc $C "tests/track/test_${x}.c" $FULL -lm -o "$W/$x"
 UBSAN_OPTIONS=halt_on_error=1 "$W/$x"
done
for x in v133_range v133_stems v13_punch_song v13_poly_pan rc1011b_audio_stability v12_usb_src v12_wave_balance; do
 case "$x" in
 v133_range) SRC='firmware/fm1-native/track_core.c';;
 v133_stems|v13_poly_pan) SRC='firmware/fm1-native/track_chip.c firmware/fm1-native/track_drum.c firmware/fm1-native/beetle_drum_rom.c firmware/fm1-native/track_mixer.c';;
 v13_punch_song|rc1011b_audio_stability) SRC='firmware/fm1-native/track_core.c firmware/fm1-native/track_song.c firmware/fm1-native/track_chip.c firmware/fm1-native/track_drum.c firmware/fm1-native/beetle_drum_rom.c firmware/fm1-native/track_mixer.c';;
 v12_wave_balance) SRC='firmware/fm1-native/track_chip.c firmware/fm1-native/track_mixer.c';;
 *) SRC='';;
 esac
 cc $C "tests/track/test_${x}.c" $SRC -lm -o "$W/$x"
 UBSAN_OPTIONS=halt_on_error=1 "$W/$x"
done
cc $C tests/track/test_rc11_help_chapters.c -o "$W/help" && "$W/help"
cc $C tests/track/test_v135_chromatic_key.c firmware/fm1-native/track_core.c -o "$W/chromatic" && "$W/chromatic"
node tests/track/test_v137_web_fx.cjs
python tests/web/test_v135_black_keys.py
python tests/web/test_v135_fx3_ui.py
python tests/web/test_v136_settings_navigation.py
python tests/web/test_v138_all_in_one.py
python tests/track/test_v137_settings_static.py
cc -O2 -std=c11 -Ifirmware/fm1-native tests/track/test_v137_settings_physical.c -o "$W/settings" && "$W/settings"
python tests/ota/test_v138_identity_manager.py
printf 'PASS V1.3.8 native regressions, browser UI, FWSC and Manager parity\n'
