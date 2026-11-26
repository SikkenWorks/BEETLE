#!/usr/bin/env python3
"""Real BEETLE native OTA C source host replay with clean and foreign staging.
Does not simulate real FM1 hardware or USB re-enumeration.
"""
from pathlib import Path
import sys, subprocess, tempfile, hashlib
root=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(root/'tools'))
import fm1_fwsc as fw
assert 'ota-FM-1_900' in (root/'firmware/loader/loader.c').read_text()
assert 'ota_selected_stock_loader?"ota-FM-1_015":"ota-FM-1_900"' in (root/'firmware/fm1-native/ota.c').read_text()
with tempfile.TemporaryDirectory(prefix='beetle-v19-') as td:
 d=Path(td)
 exe=d/'native'
 subprocess.run(['cc','-std=c11','-O2','-Wno-unused-function',str(root/'tests/ota/native_cross_firmware_replay.c'),str(root/'firmware/fm1-native/native_digest.c'),'-o',str(exe)],check=True)
 base=fw.parse_ufw((root/'BEETLE-V1.9-FIRMWARE-SWITCHING-EXPERIMENTAL.fwsc').read_bytes())
 assert base['header_crc_ok']
 flash=next(x['data'] for x in base['entries'] if x['name']=='flash.bin')
 loader=next(x['data'] for x in base['entries'] if x['name']=='ota.bin')
 app_lower=next(x['data'] for x in fw.parse_ufw((root/'BEETLE-V1.8.3-DUAL-FLASH-EXPERIMENTAL.fwsc').read_bytes())['entries'] if x['name']=='flash.bin')
 def run(payload, expect, expected_ident=None, foreign=False):
  inp=d/'incoming.bin';inp.write_bytes(fw.decode_fwsc(payload)[1])
  proc=subprocess.run([str(exe),str(inp),str(expect),str(int(expect==0)),*(['foreign'] if foreign else [])],capture_output=True,text=True)
  print(('foreign ' if foreign else 'clean   ') + proc.stdout.strip())
  if proc.returncode:raise AssertionError('OTA host replay: '+proc.stdout+'\n'+proc.stderr)
  if expected_ident and 'loader_identity='+expected_ident not in proc.stdout:raise AssertionError('wrong OTA handoff record identity')
  return proc.stdout
 good=(root/'BEETLE-V1.9-FIRMWARE-SWITCHING-EXPERIMENTAL.fwsc').read_bytes()
 run(good,0,'ota-FM-1_900')
 run(good,0,'ota-FM-1_900',True)
 old=(root/'BEETLE-V1.8.3-DUAL-FLASH-EXPERIMENTAL.fwsc').read_bytes()
 run(old,0,'ota-FM-1_900',True)
 # Corrupt the loader while keeping UFW's own checksums internally valid:
 corrupt=bytearray(loader);corrupt[500]^=0x40
 run(fw.build_fwsc(flash,bytes(corrupt),'FM-1_2123'),-11,foreign=True)
 # Corrupt app, keep UFW valid: target app CRC check must fail before erase.
 corruptflash=bytearray(flash);corruptflash[0x85000]^=0x40
 run(fw.build_fwsc(bytes(corruptflash),loader,'FM-1_2123'),-24,foreign=True)
 # Refuse V16-size firmware, which exceeds app-only writer boundary.
 extended=flash + b'\xff'*(0xA9000-len(flash))
 run(fw.build_fwsc(extended,loader,'FM-1_016'),-26,foreign=True)
 # Tampering with an inner loader fails its structural check before erase.
 unknown=bytearray(loader);unknown[0x50:0x60]=b'UNKNOWN-LOADER!'
 run(fw.build_fwsc(flash,bytes(unknown),'FM-1_2123'),-7,foreign=True)
 print('PASS: clean+foreign staging; original community OTA; handoff identity; bad loader/app and V16-sized image refused before erase')
