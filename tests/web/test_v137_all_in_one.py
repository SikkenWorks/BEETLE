"""Browser regression: project-author all-in-one shell embeds the exact three release files."""
from pathlib import Path
import base64,hashlib,re
from playwright.sync_api import sync_playwright
root=Path(__file__).resolve().parents[2]
html=(root/'BEETLE-MANAGER.html').read_text()
for name,original in [('emulator','simulator.html'),('projectmanager','BEETLE-Project-Manager.html'),('installer','WebInstaller.html')]:
 m=re.search(r'<script id="source-'+name+r'" type="application/octet-stream">(.*?)</script>',html,re.S)
 assert m and base64.b64decode(m[1])==(root/original).read_bytes(),(name,'not exact original')
assert 'V1.3.7 · ALL-IN-ONE' in html
# Test the actual shell code and routing with tiny stand-in iframe documents.
# Independently, the byte-exact assertions above prove its shipped payloads.
# Avoid executing the 884 KB USB WebInstaller while testing only tab routing.
html_smoke=html
for name in ('emulator','projectmanager','installer'):
 fixture=base64.b64encode((f'<!doctype html><html><body id="loaded-{name}">{name}</body></html>').encode()).decode()
 html_smoke=re.sub(r'(<script id="source-'+name+r'" type="application/octet-stream">)[\s\S]*?(</script>)',lambda m:m[1]+fixture+m[2],html_smoke,count=1)
with sync_playwright() as p:
 browser=p.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
 page=browser.new_page(viewport={'width':1480,'height':920})
 errors=[]
 page.on('pageerror',lambda e:errors.append(str(e)))
 page.set_content(html_smoke,wait_until='domcontentloaded',timeout=60000)
 for name in ('emulator','projectmanager','installer'):
  page.locator('#tab-'+name).click()
  page.wait_for_function('''name=>{
    const f=document.getElementById('frame-'+name);return f&&f.srcdoc&&f.contentDocument&&f.contentDocument.body&&f.contentDocument.body.textContent.includes(name) && document.getElementById('tab-'+name).getAttribute('aria-selected')==='true'
  }''',arg=name,timeout=10000)
  assert page.locator('#tab-'+name).get_attribute('aria-selected')=='true'
  assert page.locator('#frame-'+name).evaluate('(f)=>!!f.srcdoc')
  assert page.locator('#loading-'+name).is_hidden()
 assert not errors,errors
 browser.close()
print('PASS V1.3.7 all-in-one: identical payloads; all three tabs mounted without JS errors')
