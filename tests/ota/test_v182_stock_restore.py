#!/usr/bin/env python3
"""Native host replay of V1.8.2's exact-stock branch using a synthetic V15-shaped
package. Fixture SHA pins are changed ONLY in a temporary copy of ota.c,
never in the release firmware. For exact M-VAVE V15 verification, supply the
unmodified official .fwsc with --official-v15; it is not redistributed.
"""
from pathlib import Path
import hashlib,sys,subprocess,tempfile,struct, argparse
root=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(root/'tools'))
import fm1_fwsc as fw
parser=argparse.ArgumentParser()
parser.add_argument('--official-v15',type=Path)
opts=parser.parse_args()
source=(root/'firmware/fm1-native/ota.c').read_text()
assert 'beetle_stock_v15_loader_sha256' in source
src_harness=(root/'tests/ota/native_cross_firmware_replay.c').read_text()
with tempfile.TemporaryDirectory(prefix='beetle-v182-stock-') as td:
    d=Path(td)
    # A synthetic image with the native stock loader layout. This is NOT an
    # official package, and is never used outside a host-only test.
    original=fw.parse_ufw((root/'reference/BEETLE-V1.7.14-WARM-DMA-NEW-PROJECT-EXPERIMENTAL.fwsc').read_bytes())
    flash=next(x['data'] for x in original['entries'] if x['name']=='flash.bin')
    body=bytearray(((i*29+47)&0xff) for i in range(0x4de1))
    inner=bytearray(32)
    inner[8:12]=struct.pack('<I',0x01c0a800)
    inner[:2]=struct.pack('<H',fw.crc16(inner[2:]))
    body[:32]=inner
    loader=bytearray(32+len(body))
    loader[4:8]=struct.pack('<I',0x20)
    loader[8:12]=struct.pack('<I',len(body))
    loader[12]=0x41
    loader[16:32]=b'usb_hid_ota.bin\0'
    loader[32:]=body
    loader[2:4]=struct.pack('<H',fw.crc16(loader[32:]))
    loader[:2]=struct.pack('<H',fw.crc16(loader[2:32]))
    v15_fake=fw.build_fwsc(flash,bytes(loader),'FM-1_015')
    assert fw.parse_ufw(v15_fake)['header_crc_ok']
    def makehex(b):return ','.join('0x%02x'%x for x in b)
    # Only this temporary HOST copy has fixture digest pins. Native shipping
    # code retains M-VAVE's SHA-256 pins, which refuse this synthetic package.
    one='static const uint8_t beetle_stock_v15_flash_sha256[32]={'
    two='static const uint8_t beetle_stock_v15_loader_sha256[32]={'
    a=source.index(one)+len(one);b=source.index('};',a)
    source=source[:a]+makehex(hashlib.sha256(flash).digest())+source[b:]
    a=source.index(two)+len(two);b=source.index('};',a)
    source=source[:a]+makehex(hashlib.sha256(loader).digest())+source[b:]
    source=source.replace('#define OTA_LDR_HCRC 0xEBAAu',f'#define OTA_LDR_HCRC 0x{int.from_bytes(loader[:2],"little"):04X}u')
    source=source.replace('#define OTA_LDR_DCRC 0x5881u',f'#define OTA_LDR_DCRC 0x{int.from_bytes(loader[2:4],"little"):04X}u')
    p=d/'ota_fixture.c';p.write_text(source)
    harness=src_harness.replace('#include "../../firmware/fm1-native/ota.c"','#include "ota_fixture.c"')
    harness=harness.replace('if(argc!=4)', 'if(argc<4||argc>5)')
    harness=harness.replace('memset(device_flash,0xff,sizeof(device_flash));','''memset(device_flash,0xff,sizeof(device_flash));
 if(argc==5 && strcmp(argv[4],"dirty")==0)device_flash[OTA_AREA+128]=0u;''')
    (d/'test.c').write_text(harness)
    binary=d/'test'
    subprocess.run(['cc','-std=c11','-O2','-I'+str(root/'firmware/fm1-native'),str(d/'test.c'),str(root/'firmware/fm1-native/native_digest.c'),'-o',str(binary)],check=True)
    def run(payload,expected,dirty=False):
        path=d/'incoming.bin'
        path.write_bytes(fw.decode_fwsc(payload)[1]);
        p=subprocess.run([str(binary),str(path),str(expected),str(int(expected==0)),*(['dirty'] if dirty else [])],capture_output=True,text=True)
        print(f'expected={expected} dirty={dirty} {p.stdout.strip()}')
        if p.returncode:print('STDERR',p.stderr);raise AssertionError('native replay result differs')
    run(v15_fake,0,False)
    run(v15_fake,0,True)
    # A valid community image and foreign OTA staging must not be allowed.
    run((root/'BEETLE-V1.8.1-AUDIO-STABILITY-EXPERIMENTAL.fwsc').read_bytes(),-15,True)
    broken=bytearray(loader);broken[1000]^=1
    # Recreate a valid UFW but keep the signed stock-identity + outer nested
    # header. This case fails the separate full-loader hash before any erases.
    run(fw.build_fwsc(flash,broken,'FM-1_015'),-25,False)
    bad_flash=bytearray(flash);bad_flash[0x85000]^=0x08
    run(fw.build_fwsc(bad_flash,bytes(loader),'FM-1_015'),-16,False)
    # Production, non-overridden host code MUST reject the synthetic stock.
    assert hashlib.sha256(flash).digest()!=bytes.fromhex('c1068a403c04a6378a626afb66cc217f72d13f10aa200e983b7a9817cc1dac03')
    print('PASS: native OTA accepts pinned V15-shaped fixture with stale staged data; rejects corrupt loader and unsafe community overwrite')
    if opts.official_v15:
        raw=opts.official_v15.read_bytes()
        expected='db1642b2b6fa5c2cccb11ffd13878068bb28601678d3644049f99dc40e7edb8a'
        assert len(raw)==699956 and hashlib.sha256(raw).hexdigest()==expected,'NOT genuine M-VAVE V15'
        actual=fw.parse_ufw(raw)
        assert actual['product']=='FM-1_015'
        assert hashlib.sha256(next(e['data'] for e in actual['entries'] if e['name']=='ota.bin')).hexdigest()=='ef1f2db89b011fa53e71df363f0122de0ea896618c53fb53e8da676322004541'
        actual_source=(root/'tests/ota/native_cross_firmware_replay.c').read_text()
        (d/'real.c').write_text(actual_source.replace('#include "../../firmware/fm1-native/ota.c"',f'#include "{root}/firmware/fm1-native/ota.c"').replace('if(argc!=4)','if(argc<4||argc>5)').replace('memset(device_flash,0xff,sizeof(device_flash));','memset(device_flash,0xff,sizeof(device_flash));\n if(argc==5 && strcmp(argv[4],"dirty")==0)device_flash[OTA_AREA+128]=0u;'))
        realbin=d/'real'
        subprocess.run(['cc','-std=c11','-O2','-I'+str(root/'firmware/fm1-native'),str(d/'real.c'),str(root/'firmware/fm1-native/native_digest.c'),'-o',str(realbin)],check=True)
        exact=d/'exact_v15.bin';exact.write_bytes(fw.decode_fwsc(raw)[1])
        for dirty in ([],['dirty']):
            p=subprocess.run([str(realbin),str(exact),'0','1',*dirty],capture_output=True,text=True)
            print('OFFICIAL V15',p.stdout.strip())
            assert p.returncode==0,'Official V15 native OTA failed'
        print('PASS: exact official V15 package, original production digest pins, both staging states')
