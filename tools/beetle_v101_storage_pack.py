#!/usr/bin/env python3
"""Package BEETLE 1.01 RC1 experimental persistent storage using only the exact ETCH V1.37 image.

This script never talks to the FM1. The resulting .fwsc is a valid installable
container, not a declaration that installing it is safe. An unbootable app can
render software-based rollback impossible.
"""
from pathlib import Path
import argparse, hashlib, json, sys
sys.path.insert(0,str(Path(__file__).resolve().parent))
import fm1_fwsc as fw
REF_SHA='0ca1d0f16bc6658cbf13b0010a5341b8d20d0a56904c0d0118a393081b0f47d9'
ID='FM-1_2101'
REF_ID='FM-1_1037'
def sha(b):return hashlib.sha256(b).hexdigest()
def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('--app',type=Path,required=True)
    ap.add_argument('--reference',type=Path,required=True)
    ap.add_argument('--output',type=Path,required=True)
    a=ap.parse_args()
    if a.output.suffix!='.fwsc':ap.error('Output must be .fwsc, explicit experimental hardware pilot.')
    app=a.app.read_bytes();ref_raw=a.reference.read_bytes()
    if sha(ref_raw)!=REF_SHA:ap.error('NOT the uploaded exact ETCH V1.37 release; refusing')
    rr,re=fw.verify_fwsc(a.reference)
    if re or rr['product']!=REF_ID:ap.error('V1.37 reference validation error '+str(re))
    opts=json.loads((a.app.parent/'native-build-options.json').read_text())
    audit,errs=fw.validate_app_audit(a.app,Path(str(a.app)+'.audit.json'))
    if not audit or fw.validate_app_binary(app):ap.error('App audit failed '+repr(errs))
    if not(opts.get('version')=='BEETLE-1.01-RC1' and opts.get('app_sha256')==sha(app) and opts.get('beetle_storage_experimental') is True and opts.get('storage_requires_destructive_user_consent') is True and opts.get('ota_writes') is True):
        ap.error('Only audited BEETLE 1.01 RC1 standalone project storage build is allowed')
    assets=fw.extract_stock_assets(a.reference);ref_pkg=fw.parse_ufw(ref_raw)
    ref_flash=next(x['data'] for x in ref_pkg['entries'] if x['name']=='flash.bin')
    built=bytearray(fw.build_flash_image_from_assets(app,assets['uboot'],assets['cfg_tool'],assets['eq_cfg'],assets['key'],isd_config=assets['isd_config'],flash_header_payload=assets['flash_header_payload']))
    built[:0x4000]=ref_flash[:0x4000]
    data=fw.build_fwsc(bytes(built),assets['ota'],ID)
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(data)
    br,be=fw.verify_fwsc(a.output)
    cmp=fw.compare_packages(a.reference,a.output)
    pkg=fw.parse_ufw(data)
    flash=next(x['data'] for x in pkg['entries'] if x['name']=='flash.bin')
    ota=next(x['data'] for x in pkg['entries'] if x['name']=='ota.bin')
    dec=bytearray(flash[fw.APP_AREA_FLASH:]);fw.sfc_xor(dec,0,len(dec),0,assets['key'])
    app_in_image=bytes(dec[0x120:0x120+len(app)])
    checks={
      'V1.37_exact_SHA_verified':sha(ref_raw)==REF_SHA,
      'V1.37_reference_structural_verify':not re,
      'BEETLE_structural_verify':not be and br['product']==ID,
      'container_shape_matches_V1.37':cmp['same_container_shape'],
      'boot_first_16KiB_identical':flash[:0x4000]==ref_flash[:0x4000],
      'OTA_loader_identical':ota==assets['ota'],
      'app_byte_for_byte_identical_to_pilot_compile':app_in_image==app,
      'app_audit_pass':audit,
      'persistent_BEETLE_project_storage_enabled':opts['beetle_storage_experimental'] is True,
      'flash_package_stops_before_BEETLE_storage':len(flash)<=0x93000 and 0x97000>len(flash),
      'OTA_return_path_code_compiled':opts['ota_writes'] is True,
    }
    if not all(checks.values()):
      a.output.unlink(missing_ok=True);ap.error('FAILED: '+repr(checks))
    m={'name':'BEETLE','version':'1.01 RC1','kind':'EXPERIMENTAL_STORAGE_TEST',
       'hardware_tested_by_maintainer': 'Each public beta is hardware-tested by the maintainer; this build is pending their release sign-off','independent_ROM_recovery_proven':False,'brick_risk':'NONZERO',
       'image_product':ID,'package_file':a.output.name,'package_bytes':len(data),'package_sha256':sha(data),
       'app_bytes':len(app),'app_sha256':sha(app),'reference_firmware':'ETCH V1.37',
       'reference_product':REF_ID,'reference_fwsc_sha256':REF_SHA,'checks':checks,
       'ota_write_capable':True,'project_flash_writes_enabled':True,'explicit_format_required':True,'erases_prior_ETCH_Felucca_data':True,
       'note':'First-install risk remains; manual format destroys prior ETCH/Felucca data in 0x97000..0xDFFFF; cannot guarantee rollback from a nonbooting app.'}
    a.output.with_name('BEETLE-1.01-RC1-STORAGE-EXPERIMENTAL-MANIFEST.json').write_text(json.dumps(m,indent=2)+'\n')
    print(json.dumps(m,indent=2))
if __name__=='__main__':main()
