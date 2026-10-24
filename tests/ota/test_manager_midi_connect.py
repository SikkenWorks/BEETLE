from pathlib import Path
import re, base64
base = Path(__file__).resolve().parents[2]
m = (base / 'BEETLE-MANAGER.html').read_text()
w = (base / 'WebInstaller.html').read_text()
assert 'frame.srcdoc=base64ToUtf8' in m and 'frame.src=documentUrl(id)' not in m
assert 'allow="midi; autoplay"' in m
assert 'async function connect(){if(installing||!approvedPackage)return;' in w
assert "$('port-picker').hidden=true;$('install').disabled=false;" in w
assert 'this.waitFor(id=>!id.loader,C.BOOT_WAIT_MS)' in w
assert '<button type="button" id="connect" class="btn secondary" disabled>01 / CONNECT FM1</button>' in w
for tag, path in [('emulator','simulator.html'),('projectmanager','BEETLE-Project-Manager.html'),('installer','WebInstaller.html')]:
    payload = re.search(fr'<script id="source-{tag}" type="application/octet-stream">(.*?)</script>',m,re.S)
    assert payload, tag
    assert base64.b64decode(payload.group(1)) == (base/path).read_bytes(), tag
print('PASS: manager 3-in-1 parity, srcdoc, connection active, INSTALL safety gate')
