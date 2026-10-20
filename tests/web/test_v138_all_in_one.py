"""Test embedded release copies, direct tool navigation and explicit MIDI/fullscreen delegation."""
from pathlib import Path
import base64,re
from playwright.sync_api import sync_playwright
root=Path(__file__).resolve().parents[2]
html=(root/'BEETLE-MANAGER.html').read_text()
for name,original in [('emulator','simulator.html'),('projectmanager','BEETLE-Project-Manager.html'),('installer','WebInstaller.html')]:
 m=re.search(r'<script id="source-'+name+r'" type="application/octet-stream">(.*?)</script>',html,re.S)
 assert m and base64.b64decode(m[1])==(root/original).read_bytes(),(name,'embedded mismatch')
assert 'V1.3.8 · ALL-IN-ONE' in html
assert 'OPEN CURRENT TOOL DIRECTLY' in html
assert html.count('allow="midi; fullscreen; autoplay"')==3
assert 'frame.src=documentUrl(id);' in html
smoke=html
for name in ('emulator','projectmanager','installer'):
 fixture=base64.b64encode((f'<!doctype html><html><body id="loaded-{name}">{name}</body></html>').encode()).decode()
 smoke=re.sub(r'(<script id="source-'+name+r'" type="application/octet-stream">)[\s\S]*?(</script>)',lambda m:m[1]+fixture+m[2],smoke,count=1)
with sync_playwright() as p:
 browser=p.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
 page=browser.new_page(viewport={'width':1480,'height':920})
 errors=[]
 page.on('pageerror',lambda e:errors.append(str(e)))
 page.set_content(smoke,wait_until='domcontentloaded',timeout=45000)
 for name in ('emulator','projectmanager','installer'):
  page.locator('#tab-'+name).click()
  page.wait_for_function('''name=>{
   const f=document.getElementById('frame-'+name);
   return f&&f.getAttribute('src')?.startsWith('blob:')&&document.getElementById('tab-'+name).getAttribute('aria-selected')==='true';
  }''',arg=name,timeout=10000)
  assert page.locator('#loading-'+name).count()==1
  assert page.locator('#frame-'+name).get_attribute('allow')=='midi; fullscreen; autoplay'
 assert not errors,errors
 browser.close()
print('PASS V1.3.8 3-in-1 exact payloads, embedded blob launch, top-level alternative, permissions')
