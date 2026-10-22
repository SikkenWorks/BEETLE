#!/usr/bin/env python3
"""Check actual native Storage V2 marker generation matches migration tool."""
from pathlib import Path
import ctypes
import subprocess
import tempfile
import sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/storage_v2'))
from create_v2_recovery_image import marker
with tempfile.TemporaryDirectory() as d:
    so=Path(d)/'libmarker.so'
    subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-fPIC','-shared',
        str(ROOT/'firmware/fm1-native/beetle_storage_v2_reader.c'),
        str(ROOT/'firmware/fm1-native/beetle_project_pack_v2.c'),'-o',str(so)],check=True)
    lib=ctypes.CDLL(str(so))
    buf=(ctypes.c_uint8*32)()
    lib.beetle_v2_make_marker.argtypes=[ctypes.POINTER(ctypes.c_uint8)]
    lib.beetle_v2_make_marker(buf)
    native=bytes(buf)
    expected=marker()
    assert native==expected,(native.hex(),expected.hex())
    assert len(native)==32 and native[:8]==b'BTLSTV2\x01'
    assert native[24:]==b'\xff'*8
    print('PASS: native V2 marker EXACTLY matches full-flash migration image marker')
