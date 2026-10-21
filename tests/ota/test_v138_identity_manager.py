"""Build and HTML coherence regression. This is NOT a physical FM1 test."""
from pathlib import Path
import hashlib,base64,re,json,subprocess,tempfile,sys
r=Path(__file__).resolve().parents[2];sys.path.insert(0,str(r/'tools'))
import fm1_fwsc
fw=r/'BEETLE-V1.3.8-USB-MULTI-FX-EXPERIMENTAL.fwsc';raw=fw.read_bytes();meta=json.loads((r/'FIRMWARE-MANIFEST.json').read_text())
assert meta['identity']=='FM-1_2123' and meta['schema']==22
assert meta['sha256']==hashlib.sha256(raw).hexdigest()
assert all(meta['checks'].values())
parsed,err=fm1_fwsc.verify_fwsc(fw);assert not err and parsed['product']=='FM-1_2123',(err,parsed)
assert (r/'firmware/fm1-native/bringup.c').read_text().count('#define FELUCCA_ID BEETLE_PRODUCT_ID')==1
assert '#define BEETLE_PRODUCT_ID "FM-1_2123"' in (r/'firmware/fm1-native/bringup.c').read_text()
app=r/'build/track-fm1-native-beetle-storage-lab-updater-uac/app.bin'
assert app.exists() and b'FM-1_2123\x00' in app.read_bytes()
assert b'FM-1_2113\x00' not in app.read_bytes()
opts=json.loads((app.parent/'native-build-options.json').read_text())
assert opts['ota_writes'] and opts['product']=='FM-1_2123'
web=(r/'WebInstaller.html').read_text();assert "ID_ET: 'FM-1_2123'" in web and "HASH_ET: '"+meta['sha256']+"'" in web
embedded=re.search(r'<script id="firmware-embedded" type="application/octet-stream">\s*(.*?)\s*</script>',web,re.S)
assert embedded and base64.b64decode(embedded[1])==raw
assert 'this.waitFor(id=>!id.loader&&id.text===pkg.product,C.BOOT_WAIT_MS)' in web
assert 'isApprovedOtaLoader(dev.id)' in web and 'ota' in (r/'firmware/fm1-native/ota.c').read_text()
assert 'ota_host_app_compatible(fl_off,b,mine)' in (r/'firmware/fm1-native/ota.c').read_text()
readme=(r/'README.md').read_text();assert 'https://thefirmwarehub.com/?q=sikkenworks' in readme and '▶_THE_FIRMWARE_HUB-000000' in readme
sim=(r/'simulator.html').read_text();assert 'id="beetle-fullscreen"' in sim and 'image-rendering:pixelated' in sim
assert '240*factor' in sim and 'getDisplayMedia' in sim and 'getAudioTracks().length' in sim
assert 'video/webm' in sim and 'MediaRecorder' in sim
manager=(r/'BEETLE-MANAGER.html').read_text();assert 'id="authorize-midi"' in manager and 'navigator.requestMIDIAccess({sysex:true})' in manager
assert 'OPEN CURRENT TOOL DIRECTLY' in manager and 'frame.srcdoc=base64ToUtf8' in manager and 'frame.src=documentUrl(id)' not in manager
for label,path in [('emulator','simulator.html'),('projectmanager','BEETLE-Project-Manager.html'),('installer','WebInstaller.html')]:
 match=re.search(r'<script id="source-'+label+r'" type="application/octet-stream">\s*(.*?)\s*</script>',manager,re.S)
 assert match and base64.b64decode(match[1])==(r/path).read_bytes(),label
for filename in ['WebInstaller.html','simulator.html','BEETLE-Project-Manager.html']:
 s=(r/filename).read_text();scripts=re.findall(r'<script(?:\s[^>]*)?>(.*?)</script>',s,re.S)
 with tempfile.NamedTemporaryFile(mode='w',suffix='.js') as t:
  t.write('\n'.join(scripts));t.flush();subprocess.run(['node','--check',t.name],check=True,timeout=25)
print('PASS V1.3.8 native OTA identity, package checksum, product matching, fail-closed loader gates, manager 3-in-1 parity, fullscreen + audio recording, JS syntax')
