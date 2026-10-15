#!/usr/bin/env python3
"""Read-only FM1 header guard. Refuses to approve a .fwsc with modified boot bytes.

Usage:
 python verify_protected_header.py --reference ETCH-V1.37.fwsc --candidate BEETLE.fwsc --app v037-storage-app.bin
Returns nonzero on any protected-header or OTA mismatch or app-byte difference.
"""
import argparse,hashlib,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
import fm1_fwsc as fw

def sha(b):return hashlib.sha256(b).hexdigest()

def payloads(path):
 report,errors=fw.verify_fwsc(path)
 if errors:raise ValueError(f'{path}: firmware structure/checksum failed: {errors}')
 items={e['name']:e['data'] for e in fw.parse_ufw(path.read_bytes())['entries']}
 return report,items

def main():
 ap=argparse.ArgumentParser()
 ap.add_argument('--reference',type=Path,required=True)
 ap.add_argument('--candidate',type=Path,required=True)
 ap.add_argument('--app',type=Path,required=True)
 ap.add_argument('--expected-product',type=str,required=True,help="Pin the exact intended BEETLE product ID")
 a=ap.parse_args()
 ref,reference=payloads(a.reference)
 can,candidate=payloads(a.candidate)
 if ref['product']!='FM-1_1037':raise ValueError('Reference must be exact ETCH V1.37 identity')
 if sha(a.reference.read_bytes())!='0ca1d0f16bc6658cbf13b0010a5341b8d20d0a56904c0d0118a393081b0f47d9':raise ValueError('Unexpected reference bytes')
 if not a.expected_product.startswith('FM-1_') or can['product']!=a.expected_product:raise ValueError('Unexpected firmware product identity')
 if candidate['flash.bin'][:0x4000]!=reference['flash.bin'][:0x4000]:
  diffs=[hex(i) for i,(b,c) in enumerate(zip(candidate['flash.bin'][:0x4000],reference['flash.bin'][:0x4000])) if b!=c]
  raise ValueError('BOOT HEADER MODIFIED: '+', '.join(diffs[:40]))
 if candidate['ota.bin']!=reference['ota.bin']:raise ValueError('OTA loader modified')
 assets=fw.extract_stock_assets(a.reference)
 data=bytearray(candidate['flash.bin'][fw.APP_AREA_FLASH:]);fw.sfc_xor(data,0,len(data),0,assets['key'])
 app=a.app.read_bytes()
 if bytes(data[0x120:0x120+len(app)]) != app: raise ValueError('Compiled app not equal to firmware app')
 print('PASS: ETCH V1.37 protected boot bytes unchanged, OTA unchanged, compiled BEETLE app exact')
 print('Firmware SHA-256:', sha(a.candidate.read_bytes()))
 return 0
if __name__=='__main__':
 try:sys.exit(main())
 except Exception as e:print('REJECT:',e,file=sys.stderr);sys.exit(1)
