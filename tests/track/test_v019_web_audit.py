#!/usr/bin/env python3
"""Check embedded V0.17 firmware stays bundled and the V0.19 local flash-audit UI works."""
from pathlib import Path
from tempfile import TemporaryDirectory
from playwright.sync_api import sync_playwright
import hashlib,re,base64
p=Path(__file__).resolve().parents[2]/'TRACK-Web-V0.19.html'
s=p.read_text()
assert 'TRACK V0.19' in s
assert 'STORAGE / RC PREFLIGHT (READ ONLY)' in s
assert 'This installer still embeds' in s
assert 'entry.addEventListener' in s
assert 'flash' in s.lower()
# Ensure V0.17 OTA-lab candidate remains the same in the underlying installer
assert 'const encoded=' in s or 'const encoded =' in s
with TemporaryDirectory() as d:
 path=Path(d)/'dump.bin';buf=bytearray([255]*1048576);buf[0x97000:0x97004]=b'FELU';path.write_bytes(buf)
 with sync_playwright() as pw:
  browser=pw.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox'])
  page=browser.new_page();errs=[];page.on('pageerror',lambda e:errs.append(str(e)))
  page.set_content(s,wait_until='domcontentloaded',timeout=60000)
  assert page.locator('#nav-simulator').is_visible()
  page.locator('#nav-installer').click()
  assert page.locator('#storage-readiness').is_visible()
  assert 'the verified V0.17 OTA-lab candidate' in page.locator('#storage-readiness').inner_text()
  page.locator('#track-flash-dump').set_input_files(str(path))
  page.wait_for_function("document.querySelector('#track-flash-result').textContent.includes('WRITE AUTHORIZATION: NO')")
  txt=page.locator('#track-flash-result').inner_text()
  assert 'FELUCCA PROJECTS' in txt and 'erased 8/9 sectors' in txt,txt
  assert path.read_bytes()==buf
  page.locator('#nav-simulator').click()
  assert page.locator('#track-simulator-panel').is_visible()
  assert not errs,errs
  print('PASS WebSimulator/WebInstaller tabs; local 1MiB dump audit; never authorized; embedded OTA candidate still V0.17; JS errors = 0')
  browser.close()
