#!/usr/bin/env python3
"""Build BEETLE V1.8.3 open-source Felucca-lineage, app-only OTA loader.
The stock V15 loader is NOT substituted for stock restore.
"""
import os,subprocess,sys,struct
from pathlib import Path
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'tools'))
from fm1_fwsc import validate_ota
# Use upstream Felucca LZ4 block implementation (attribution preserved).
sys.path.insert(0,str(root/'tools/loader_support'))
import lz4blk

def crc16(d,c=0):
 for b in d:
  c ^= b << 8
  for _ in range(8):c = ((c<<1)^0x1021 if c&0x8000 else c<<1)&0xffff
 return c

def head(dcrc,a,b,attr,name=b'usb_hid_ota.bin'):
 body=struct.pack('<HIIBBH16s',dcrc,a,b,attr,0,0,name)
 return struct.pack('<H',crc16(body))+body

def wrap(raw):
 blocks=bytearray()
 for i in range(0,len(raw),4096):
  frag=raw[i:i+4096];comp=lz4blk.compress(frag)
  if len(comp)>4096 or lz4blk.decompress(comp)!=frag:raise RuntimeError('compressed RAM loader block invalid')
  blocks+=struct.pack('<II',len(comp),len(frag))+comp
 inner=head(crc16(raw),len(raw),0x01c0a800,0)
 payload=inner+blocks
 return head(crc16(payload),0x20,len(payload),0x41)+payload

def run():
 tc=Path(os.environ.get('JIELI_TOOLCHAIN',''))
 if not (tc/'pi32v2/bin/clang').exists():raise SystemExit('Set JIELI_TOOLCHAIN to the unpacked JieLi toolchain')
 out=root/'build/v183-loader';out.mkdir(parents=True,exist_ok=True)
 cc=str(tc/'pi32v2/bin/clang');ld=str(tc/'pi32v2/bin/ld');objcopy=str(tc/'common/bin/objcopy')
 src=root/'firmware/loader'
 subprocess.run([cc,'-target','pi32v2','-c',str(src/'crt0_ldr.S'),'-o',str(out/'crt0_ldr.o')],check=True)
 subprocess.run([cc,'-target','pi32v2','-Os','-ffunction-sections','-fno-builtin','-Wall','-Wno-unused-function', '-I'+str(root/'firmware/hal'),'-I'+str(root/'firmware/src'), '-c',str(src/'loader.c'),'-o',str(out/'loader.o')],check=True)
 elf=out/'loader.elf'
 subprocess.run([ld,'--gc-sections','-e','_start','-T',str(src/'loader.ld'),str(out/'crt0_ldr.o'),str(out/'loader.o'),'-o',str(elf)],check=True)
 subprocess.run([objcopy,'-O','binary','-j','.text',str(elf),str(out/'loader.bin')],check=True)
 raw=(out/'loader.bin').read_bytes()
 if not 1024<=len(raw)<=0x14000:raise RuntimeError('loader invalid RAM size')
 if b'FELUCCA-LOADER-1' not in raw[:96]:raise RuntimeError('Missing upstream loader identity tag')
 image=wrap(raw)
 info=validate_ota(image)
 if not (info['outer_header_ok'] and info['inner_header_ok'] and info['outer_data_crc_ok'] and info['inner_data_crc_ok'] and info['inner_load_ok']):raise RuntimeError('loader wrapper failed validation:'+str(info))
 if len(image)>0x5000-0x200:raise RuntimeError('OTA staging loader exceeds bounds')
 (out/'ota.bin').write_bytes(image)
 print(f'BEETLE V1.8.3 loader: {len(raw)} B raw, {len(image)} B wrapped, validates {info["encoding"]}')
if __name__=='__main__':run()
