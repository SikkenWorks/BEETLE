#!/usr/bin/env python3
"""Build a verified complete FM1 recovery image with BEETLE V2 project/sample partition.

Operates ON FILES ONLY. Requires original locally-held 1 MiB flash backup and
an explicit 12-slot migration manifest. No USB, MIDI, OTA or flash hardware API.
Output is NOT installable with legacy BEETLE. A matching native V2 firmware is
required, along with an independently confirmed device restore workflow.
Never upload private flash dumps to public repositories.
"""
from pathlib import Path
import argparse, ctypes, hashlib, json, struct, subprocess, sys, tempfile, zlib
from legacy_flash_audit import audit_image,AuditError,BASE,END,MARKER,MARKER_END,FLASH_BYTES
from migration_preflight import load_entries,MigrationError,ROOT
from storage_v2_sim import Nor,Storage,ProjectCodec,CorruptData,NotEnoughSafeSpace,UnsafeLayout

MARKER_MAGIC=b'BTLSTV2\x01'
def marker()->bytes:
    prefix=MARKER_MAGIC+struct.pack('<III',BASE,END,4096)
    return (prefix+struct.pack('<I',zlib.crc32(prefix)&0xffffffff)).ljust(32,b'\xff')

def sha(data:bytes)->str:return hashlib.sha256(data).hexdigest()

def make_image(source:bytes,projects:dict,samples:dict)->tuple[bytes,dict]:
    report,actual=audit_image(source)
    for slot in range(1,13):
        if slot not in projects:raise MigrationError(f'missing explicit slot {slot}')
        if projects[slot]!=actual.get(slot):
            raise MigrationError(f'slot {slot} does not exactly match authenticated full-flash dump')
    src=ROOT/'firmware/fm1-native/beetle_project_pack_v2.c'
    with tempfile.TemporaryDirectory(prefix='beetle-v2-converter-') as temp:
        so=Path(temp)/'p.so'
        subprocess.run(['cc','-std=c11','-O2','-fPIC','-shared','-Wall','-Wextra','-Werror',str(src),'-o',str(so)],check=True,capture_output=True)
        codec=ProjectCodec(ctypes.CDLL(str(so)))
        nor=Nor(authorized=True)
        dst=Storage(nor,codec)
        for slot in range(1,13):
            if projects[slot] is not None:dst.save_project(slot-1,projects[slot])
        for slot,blob in sorted(samples.items()):dst.replace_sample(slot-1,blob)
        valid=Storage(Nor(bytes(nor.bytes),authorized=True),codec)
        for slot in range(1,13):
            if valid.load_project(slot-1)!=projects[slot]:raise MigrationError(f'failed V2 project {slot} readback')
        for slot,blob in samples.items():
            if valid.load_sample(slot-1)!=blob:raise MigrationError(f'failed V2 sample {slot} readback')
        if valid.stats()['erased_sectors']<4:raise MigrationError('four-sector safety reserve violated')
        result=bytearray(source)
        result[BASE:END]=nor.bytes
        result[MARKER:MARKER+32]=marker()
        result[MARKER+32:MARKER_END]=b'\xff'*(MARKER_END-MARKER-32)
        protected=(result[:BASE]==source[:BASE] and result[MARKER_END:]==source[MARKER_END:])
        if not protected:raise MigrationError('write outside dedicated BEETLE storage area')
        if result[MARKER:MARKER+32]!=marker():raise MigrationError('V2 ownership marker missing')
        summary={'format':'BEETLE-RECOVERY-IMAGE-V2-1','output_is_installable_on_legacy_firmware':False,
          'source_sha256':sha(source),'output_sha256':sha(bytes(result)),
          'source_authenticated_projects':len(actual),'projects_migrated':sum(v is not None for v in projects.values()),
          'user_samples':len(samples),'user_sample_bytes':sum(len(v) for v in samples.values()),
          'reserved_erased_sectors':valid.stats()['erased_sectors'],
          'protected_flash_byte_identical':protected,
          'modified_regions':['0x97000..0xDEFFF (BEETLE project/sample volume)','0xDF000..0xDF01F (BEETLE V2 marker)'],
          'identity_note':'Application/boot/OTA and FM-1_2123 identity are not edited by this tool',
          'warning':'NOT A STANDALONE FIRMWARE. Requires matching V2 native application with verified boot/save/playback. Never flash this image on legacy V1.4.14.'}
        return bytes(result),summary

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source_image',type=Path)
    parser.add_argument('migration_manifest',type=Path)
    parser.add_argument('output_image',type=Path)
    parser.add_argument('--acknowledge-v2-firmware-required',action='store_true',help='Required. Output is not compatible with existing firmware.')
    args=parser.parse_args()
    try:
        if not args.acknowledge_v2_firmware_required:raise MigrationError('specify --acknowledge-v2-firmware-required')
        orig=args.source_image.resolve();out=args.output_image.resolve()
        if out==orig or out==args.migration_manifest.resolve() or args.output_image.exists():
            raise MigrationError('output must be a NEW path, never overwrite any source')
        if out.parent!=orig.parent and not out.parent.is_dir():raise MigrationError('output parent must already exist')
        projects,samples=load_entries(args.migration_manifest.resolve())
        source=args.source_image.read_bytes()
        if len(source)!=FLASH_BYTES:raise MigrationError('source dump must be 1,048,576 bytes')
        result,summary=make_image(source,projects,samples)
        with args.output_image.open('xb') as f:f.write(result)
        if args.output_image.read_bytes()!=result:
            args.output_image.unlink(missing_ok=True)
            raise MigrationError('image write/read-back check failed')
        print(json.dumps(summary,indent=2))
    except (AuditError,MigrationError,CorruptData,UnsafeLayout,NotEnoughSafeSpace,ValueError,OSError,subprocess.CalledProcessError) as error:
        print(f'RECOVERY IMAGE REJECTED: {error}',file=sys.stderr)
        return 1
    return 0
if __name__=='__main__':sys.exit(main())
