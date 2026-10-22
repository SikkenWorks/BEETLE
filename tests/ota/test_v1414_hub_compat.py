#!/usr/bin/env python3
"""Offline OTA trust-boundary replay. No USB/flash hardware touched."""
import sys, subprocess, tempfile
from pathlib import Path
root=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(root/'tools'))
import fm1_fwsc as fw
base=(root/'BEETLE-V1.4.13-REC-LED-HUB-LAB-UNVERIFIED.fwsc').read_bytes()
parsed=fw.parse_ufw(base)
flash=next(e['data'] for e in parsed['entries'] if e['type']==0)
loader=next(e['data'] for e in parsed['entries'] if e['type']==100)

def change(loader_bytes, body_pos, xor_value, recalc_crc=True):
    l=bytearray(loader_bytes)
    l[32+body_pos]^=xor_value
    if recalc_crc:
        l[2:4]=fw.crc16(l[32:]).to_bytes(2,'little')
        l[0:2]=fw.crc16(l[2:32]).to_bytes(2,'little')
    return bytes(l)

with tempfile.TemporaryDirectory() as td:
    td=Path(td)
    runner=td/'ota_test'
    subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Wno-unused-function',
        '-I',str(root/'firmware/fm1-native'),str(root/'tests/ota/native_cross_firmware_replay.c'),
        str(root/'firmware/fm1-native/native_digest.c'),'-o',str(runner)],check=True)
    # Deliberately different loader SHA but intact family tag, entrypoint and valid CRC.
    # This is a SYNTHETIC TEST ONLY, not a production bootloader.
    valid_modified=change(loader,len(loader)-32-20,0x04)
    no_marker=change(loader,0x4c-32+3,0x01)
    broken_crc=change(loader,len(loader)-32-10,0x08,False)
    tests=[('original_beetle',loader,0,1),
           ('tagged_other_loader',valid_modified,0,1),
           ('not_felucca_loader',no_marker,-16,0),
           ('body_corrupted',broken_crc,-11,0)]
    for name,ldr,expect,commit in tests:
        raw=fw.build_fwsc(flash,ldr,'FM-1_2123')
        payload=fw.decode_fwsc(raw)[1]
        logical=td/(name+'.bin');logical.write_bytes(payload)
        proc=subprocess.run([str(runner),str(logical),str(expect),str(commit)],capture_output=True,text=True)
        print(name,proc.stdout.strip(), 'PASS' if proc.returncode==0 else 'FAIL')
        if proc.returncode:
            print(proc.stderr)
            raise AssertionError(f'{name} expected {expect}/{commit}')
    print('PASS: cross-firmware FM1 app/container checks and refuse unknown/corrupt loaders')
