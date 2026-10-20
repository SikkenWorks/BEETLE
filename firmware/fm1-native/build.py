#!/usr/bin/env python3
"""Build GPL native FM1 lab variants; no device access or release export."""
import os,subprocess,sys
import argparse
ap=argparse.ArgumentParser();ap.add_argument("--storage",action="store_true",help="compile experimental persistent flash writes; lab only")
ap.add_argument("--beetle-storage-lab",action="store_true",help="EXPERIMENTAL: BEETLE-only physical journal with explicit destructive takeover confirmation; NOT hardware validated")
ap.add_argument("--updater",action="store_true",help="compile MIDI OTA staging/commit for isolated LAB validation only; never flash from this build without physical test")
ap.add_argument("--pilot",action="store_true",help="V0.32 field diagnostic: OTA recovery available but persistent projects always locked")
ap.add_argument("--hardware-test",action="store_true",help="read-only full-app diagnostics: no storage and NO updater writes; Stage 0 remains the minimal first-boot probe")
ap.add_argument("--usb-audio",action="store_true",help="experimental class-compliant stereo USB capture at 44.1 kHz; no device test")
ap.add_argument("--usb-audio-48k",action="store_true",help="EXPERIMENTAL: host-selectable 48 kHz FIR capture; implies --usb-audio")
ap.add_argument("--multi-project",action="store_true",help="EXPERIMENTAL: four project slots in one A/B snapshot; capacity limited")
ap.add_argument("--extended-store",action="store_true",help="LAB ONLY: expanded A/B flash region; refuses foreign data; backup and recovery required")
args=ap.parse_args()
if args.pilot and (args.storage or args.extended_store or args.beetle_storage_lab or args.multi_project or args.hardware_test): ap.error("Pilot mode must never enable persistent flash or full read-only hardware-test mode")
if args.pilot: args.updater=True
# TRACK must NEVER enable the old ETCH flash writer without a fully
# audited tracker-specific 4x48-bank mapping and successful recovery tests.
if args.storage or args.extended_store or args.multi_project:
 ap.error('BEETLE V1.3.6: legacy ETCH flash writes remain disabled. Only an isolated OTA LAB build is allowed.')

if args.beetle_storage_lab and not args.updater: ap.error("--beetle-storage-lab requires --updater; OTA recovery must remain present")
if args.usb_audio_48k:args.usb_audio=True
if args.multi_project and not args.storage:ap.error("--multi-project requires --storage")
# V0.95: persistent FM1 builds MUST include the implemented PROJECTS browser.
# Do not silently generate a firmware that displays PROJECTS DISABLED again.
if args.storage:args.multi_project=True
read_only=not (args.storage or args.updater or args.beetle_storage_lab)
if args.extended_store and (not args.storage or args.hardware_test):ap.error("--extended-store requires --storage and excludes --hardware-test")
if args.hardware_test and (args.storage or args.updater or args.usb_audio):ap.error("--hardware-test sets its own write policy")
if args.hardware_test:args.updater=False
if args.hardware_test and (args.storage or args.updater or args.extended_store):ap.error('hardware-test must be read-only')

from pathlib import Path
src=Path(__file__).resolve().parent
root=src.parents[1]
tc=Path(os.environ.get('JIELI_TOOLCHAIN',str(Path.home()/'.jieli/toolchain'))).resolve()
out=root/('build/track-fm1-native'+('-beetle-storage-lab' if args.beetle_storage_lab else '')+('-storage' if args.storage else '')+('-test' if args.hardware_test else '-pilot' if args.pilot else '-updater' if args.updater else '')+('-uac48' if args.usb_audio_48k else '-uac' if args.usb_audio else '')+('-ext' if args.extended_store else '')+('-p05' if args.multi_project else ''));out.mkdir(parents=True,exist_ok=True)
cc=tc/'pi32v2/bin/clang';ld=tc/'pi32v2/bin/ld'
if not cc.exists() or not ld.exists():sys.exit('Set JIELI_TOOLCHAIN to the directory containing pi32v2 and common.')
# V0.69: never accidentally drift away from the bundled Felucca HAL/boot
# baseline. Intentional upstream divergences (USB and OTA) are documented;
# their physical behaviour is not claimed to be verified.
subprocess.run([sys.executable,str(root/'tools/audit_felucca_core.py')],check=True)
objs=[]
for name in ['bringup.c','track_core.c','track_name.c','track_mixer.c','track_song.c','track_chip.c','track_drum.c','beetle_drum_rom.c','track_project_store.c','track_journal.c','beetle_flash_layout.c','audio_core.c','native_performance.c','native_seq_ui.c','native_pattern_bank.c','native_project_codec.c','native_wave_fx.c','native_fx.c','native_factory.c','wave_editor.c','wave_store.c','native_digest.c','native_panel_midi.c','native_performance_midi.c','crt0.S','hal/fm1_vec.S','hal/fm1_isr.S']:
 obj=out/(Path(name).stem+'.o');cmd=[str(cc),'-target','pi32v2']
 if name.endswith('.c'):cmd+=['-Os','-fno-builtin','-ffunction-sections','-I'+str(src/'hal'),'-isystem',str(tc/'pi32v2/include'),'-DTRACK_FIRMWARE=1','-DBEETLE_NATIVE_STORAGE='+str(int(args.beetle_storage_lab)),'-DETCH_FLASH_READ_ONLY='+str(int(read_only)),'-DETCH_NATIVE_STORAGE='+str(int(args.storage)),'-DETCH_NATIVE_OTA='+str(int(args.updater)),'-DBEETLE_PILOT='+str(int(args.pilot)),'-DETCH_TEST_RESTORE_ONLY='+str(int(args.hardware_test)),'-DETCH_NATIVE_UAC='+str(int(args.usb_audio)),'-DETCH_NATIVE_UAC_48K='+str(int(args.usb_audio_48k)),'-DETCH_P04_EXPANDED='+str(int(args.extended_store)),'-DETCH_P05_MULTI='+str(int(args.multi_project)),'-DETCH_STORE_SPARE_ENABLED=0','-DETCH_STORE_OWNED_ENABLED='+str(int(args.storage))]
 subprocess.run(cmd+['-c',str(src/name),'-o',str(obj)],check=True);objs.append(obj)
elf=out/'track-native.elf'
subprocess.run([str(ld),'--gc-sections','-e','_start','-T',str(src/'beetle_app.ld'),*map(str,objs),'-o',str(elf)],check=True)
subprocess.run([sys.executable,str(root/'tools/fm1_elf_to_app.py'),str(elf),str(out/'app.bin'),'--toolchain',str(tc)],check=True)
subprocess.run([sys.executable,str(root/'tools/fm1_app_validate.py'),str(out/'app.bin'),'--elf',str(elf),'--toolchain',str(tc),'--report',str(out/'app.bin.audit.json')],check=True)
if read_only:
 subprocess.run([sys.executable,str(root/'tools/check_readonly_images.py'),'--elf',str(elf)],check=True,env={**os.environ,'JIELI_TOOLCHAIN':str(tc)})
print('Compiled BEETLE V1.3.6', 'OTA-LAB' if args.updater else 'READ-ONLY', '; BEETLE persistent storage experimental:',args.beetle_storage_lab,'; updater writes:',args.updater,'; hardware validation is performed by project maintainer separately.')

import json,hashlib
(out/'native-build-options.json').write_text(json.dumps({'app_sha256':hashlib.sha256((out/'app.bin').read_bytes()).hexdigest(),'beetle_storage_experimental':args.beetle_storage_lab,'storage_writes':args.storage,'ota_writes':args.updater,'readonly_hardware_test':args.hardware_test,'flash_read_only':read_only,'version':'BEETLE-V1.3','pilot_ota_enabled_no_project_writes':args.pilot, 'storage_requires_destructive_user_consent':args.beetle_storage_lab,'overwrites_prior_ETCH_Felucca_user_storage':args.beetle_storage_lab,'hardware_verified':None,'intended_use':'Pilot OTA test - no project storage writes. Hardware test status: maintainer release process.' if args.pilot else ('EXPERIMENTAL BEETLE-only A/B flash writes gated by 5s physical confirmation' if args.beetle_storage_lab else 'OTA lab; BEETLE project flash writes locked'),'usb_audio_44100':args.usb_audio,'usb_audio_48000_experimental':args.usb_audio_48k,'extended_store_experimental':args.extended_store,'multi_project_experimental':args.multi_project,'product':'FM-1_2115','brand':'BEETLE','legacy_project_format':'TRACK-compatible'},indent=2)+'\n')
