from pathlib import Path
import re
root=Path(__file__).resolve().parents[2]
fw=root/'firmware/fm1-native'
bring=(fw/'bringup.c').read_text(); build=(fw/'build.py').read_text()
assert '#define FELUCCA_ID "FM-1_2051"' in bring
assert "'product':'FM-1_2051'" in build
assert "'version':'BEETLE-0.51'" in build
assert 'beetle_boot_active=1u' in bring
assert 'beetle_boot_advance();' in bring
assert 'if(!safe && !native_updating && !native_tripped && !native_screen_gate.ready)' in bring
assert 'else if(native_screen_gate.ready)beetle_boot_active=0u' in bring
assert 'for(uint8_t f=0u;f<2u;f++)' not in bring
sid=(fw/'track_chip.c').read_text()
assert 'case TRACK_CHIP_RESID_LITE:' in sid
assert 'beetle_sid_sample(' in sid
assert '"RESID-LITE"' in (fw/'track_runtime.h').read_text()
app=(root/'build/track-fm1-native-beetle-storage-lab-updater/app.bin').read_bytes()
assert b'FM-1_2051' in app and b'FM-1_2047' not in app
print('PASS V0.51 on-device ID, repeating boot frame service, 5th SID oscillator compiled')
