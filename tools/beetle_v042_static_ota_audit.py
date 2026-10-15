#!/usr/bin/env python3
"""Static, no-flash audit: verifies both OTA gates and protected loader invariant."""
from pathlib import Path
import re
root=Path(__file__).resolve().parents[1]
ota=(root/'firmware/fm1-native/ota.c').read_text()
mat=(root/'firmware/fm1-native/native_ota_matrix.h').read_text()
boot=(root/'firmware/fm1-native/track_boot_splash.h').read_text()
assert 'len==ETCH_OTA_BODY_LEN' in ota
assert 'etch_ota_hash(ota_off+32,b,1)' in ota
assert 'official_stock_restore=1u' in ota
assert 'len==OTA_LDR_LEN' in ota
assert 'ota_rd16(b)==OTA_LDR_HCRC' in ota
assert 'ota_rd16(b+2)==OTA_LDR_DCRC' in ota
assert 'etch_ota_stock(fl_off,b)' in ota
assert 'if(!official_stock_restore)' in ota
assert 'etch_ota_available(b)' in ota
assert 'BEETLE-OTA-STAGE' in ota
assert 'ETCH-OTA-STAGE-1' in ota
assert 'const char etch[]="ETCH"' not in mat
assert 'beetle_play_icon' in mat and 'beetle_play_icon' in boot
assert '0xF81Au' in mat
assert '0xF81Eu' in boot  # exact RGB565 conversion of source #FF00F0
print('PASS V0.42 static OTA policy and boot visual checks (not a physical flash test)')
