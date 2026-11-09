#!/usr/bin/env python3
"""Replays the packaged V1.8.3 using BEETLE's production C OTA stage,
then exercises the Felucca-lineage app-only loader with two real BEETLE images.
The simulated NOR and USB host do not substitute for physical FM1 testing.
"""
from pathlib import Path
import subprocess,tempfile,sys
r=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(r/'tools'))
import fm1_fwsc as fw
old=r/'BEETLE-V1.8.2-STOCK-V15-RESTORE-EXPERIMENTAL.fwsc'
new=r/'BEETLE-V1.8.3-DUAL-FLASH-EXPERIMENTAL.fwsc'
if not (old.is_file() and new.is_file()):raise SystemExit('Missing firmware files: first build/package release')
with tempfile.TemporaryDirectory() as tmp:
 d=Path(tmp)
 def compile_test(src,out,*extra):
  subprocess.run(['cc','-std=c11','-O2',str(src),*map(str,extra),'-o',str(out)],check=True)
 ota=d/'stage'
 compile_test(r/'tests/ota/native_cross_firmware_replay.c',ota,r/'firmware/fm1-native/native_digest.c')
 logical=d/'logical.bin';logical.write_bytes(fw.decode_fwsc(new.read_bytes())[1])
 subprocess.run([str(ota),str(logical),'0','1'],check=True)
 # Host loader replay checks no head writes, data retention and CRC fault refusal.
 loader=d/'ldr'
 compile_test(r/'tests/flash_id/ldr_test.c',loader)
 subprocess.run([str(loader),str(old),str(new)],check=True,stdout=subprocess.DEVNULL)
 # Same-format OTA package, but its payload CRC changes without correcting its
 # nested body/header digest: the device must reject it before any erase.
 parsed=fw.parse_ufw(new.read_bytes())
 files={x['name']:x['data'] for x in parsed['entries']}
 damaged=bytearray(files['ota.bin']);damaged[500]^=0x80
 broken=fw.build_fwsc(files['flash.bin'],bytes(damaged),parsed['product'])
 logical.write_bytes(fw.decode_fwsc(broken)[1])
 proc=subprocess.run([str(ota),str(logical),'-11','0'],capture_output=True,text=True)
 if proc.returncode!=0:
  print(proc.stdout,proc.stderr)
  raise AssertionError('Damaged loader was not rejected without NOR flash writes')
 print('PASS: V1.8.2-compatible OTA stage, new Felucca-derived loader, storage/head preserved, damaged loader blocked')
