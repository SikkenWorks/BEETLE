#!/usr/bin/env python3
"""BEETLE V0.37 offline-only protected-header repair.
Requires exact archived ETCH V1.37 .fwsc and known broken V0.37 .fwsc.
Preserves compiled BEETLE app/OTA exactly. No FM1 device interaction.
"""
import argparse,hashlib,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
import fm1_fwsc as fw

def sha(b):return hashlib.sha256(b).hexdigest()
def entries(raw):
 p=fw.parse_ufw(raw)
 return p['product'],{e['name']:e['data'] for e in p['entries']}
def main():
 ap=argparse.ArgumentParser()
 for key in ['reference','broken','output']:ap.add_argument('--'+key,required=True,type=Path)
 a=ap.parse_args()
 rr=a.reference.read_bytes();bb=a.broken.read_bytes()
 if sha(rr)!='0ca1d0f16bc6658cbf13b0010a5341b8d20d0a56904c0d0118a393081b0f47d9':ap.error('Not exact ETCH V1.37')
 rid,r=entries(rr);bid,b=entries(bb)
 if rid!='FM-1_1037' or bid!='FM-1_2037':ap.error('Wrong firmware IDs')
 if b['ota.bin']!=r['ota.bin']:ap.error('OTA loader mismatch')
 flash=bytearray(b['flash.bin']);reference=r['flash.bin']
 changes=[i for i in range(0x4000) if flash[i]!=reference[i]]
 if changes!=[0x20,0x21,0x22,0x23]:ap.error('Different header modifications; refusing: '+str(changes[:32]))
 flash[0x20:0x24]=reference[0x20:0x24]
 if flash[:0x4000]!=reference[:0x4000]:ap.error('Protected boot still differs')
 result=fw.build_fwsc(bytes(flash),b['ota.bin'],bid)
 a.output.write_bytes(result)
 rep,errors=fw.verify_fwsc(a.output)
 if errors:
  a.output.unlink(missing_ok=True);ap.error('Firmware verification failed: '+str(errors))
 print('PASS, corrected .fwsc:',a.output,'SHA-256',sha(result))
if __name__=='__main__':main()
