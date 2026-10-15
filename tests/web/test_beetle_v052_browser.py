from playwright.sync_api import sync_playwright
from pathlib import Path
p=Path(__file__).resolve().parents[2]/'web/BEETLE-V0.52-WebSimulator.html'
s=p.read_text().replace('const audio=new TrackAudio();', '''window.__beetleDev=()=>({oct:model.keyboardOctave,ch:model.ch,screen:model.screen,poly:model.editPoly,notes:pattern(model.project,model.project.selectedPattern,false)?.channels[model.ch]?.[model.row]?.note,slots:PROJECT_COUNT});const audio=new TrackAudio();''')
with sync_playwright() as play:
 browser=play.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
 page=browser.new_page(viewport={'width':1280,'height':1100});errs=[]
 page.on('pageerror',lambda exc:errs.append(str(exc)))
 page.set_content(s,wait_until='load',timeout=30000)
 page.wait_for_timeout(1430)
 assert page.locator('#splash').get_attribute('class')=='splash hidden'
 assert page.evaluate('window.__beetleDev().slots')==12
 page.locator('[data-action="EDIT"]').click()
 plus=page.locator('[data-action="OCT+"]')
 plus.dispatch_event('pointerdown');plus.dispatch_event('pointerup')
 assert page.evaluate('window.__beetleDev().oct')==1
 minus=page.locator('[data-action="OCT-"]')
 minus.dispatch_event('pointerdown');plus.dispatch_event('pointerdown');plus.dispatch_event('pointerup');minus.dispatch_event('pointerup')
 assert page.evaluate('window.__beetleDev().oct')==1
 assert page.evaluate('window.__beetleDev().poly')==True
 page.locator('[data-action="SEQ"]').dispatch_event('pointerdown')
 page.locator('[data-action="SEQ"]').dispatch_event('pointerup')
 assert page.evaluate('window.__beetleDev().screen')=='tracker'
 assert page.evaluate('window.__beetleDev().oct')==1
 page.locator('.white-key').first.dispatch_event('pointerdown');page.locator('.white-key').first.dispatch_event('pointerup')
 assert page.evaluate('window.__beetleDev().notes')==72
 for _ in range(5):page.locator('[data-turn="select:1"]').click()
 assert page.evaluate('window.__beetleDev().ch')==5
 page.locator('[data-action="LFO"]').click()
 assert page.evaluate('window.__beetleDev().screen')=='masterfx'
 page.locator('[data-turn="select:1"]').click()
 page.locator('[data-turn="k4:1"]').click()
 assert page.locator('#screen').count()==1
 # Optional screenshot: page.screenshot(path='/tmp/beetle-v051-web.png')
 print('PASS V0.52 WebSimulator: no console errors; 12 project slots; six lanes; EDIT OCT+; pair MONO/POLY; shared tracker octave; tracker note 72')
 assert not errs,errs
 assert 'AUDIO ERROR' not in page.locator('#info').inner_text(),page.locator('#info').inner_text()
 browser.close()
