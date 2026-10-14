from pathlib import Path
from playwright.sync_api import sync_playwright
p=Path(__file__).resolve().parents[2]/'web/BEETLE-V0.54-WebSimulator.html'
s=p.read_text().replace('const audio=new TrackAudio();', "window.__v054=()=>({screen:model.screen,menu:model.menuIndex,help:model.helpOffset,len:HELP_LINES.length,fx:model.masterFocus});const audio=new TrackAudio();")
with sync_playwright() as playwright:
 browser=playwright.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
 page=browser.new_page(viewport={'width':1220,'height':1000})
 errors=[]
 page.on('pageerror',lambda err:errors.append(str(err)))
 page.set_content(s,wait_until='load',timeout=30000)
 page.wait_for_timeout(1400)
 page.locator('#homeButton').dispatch_event('pointerdown')
 page.locator('#homeButton').dispatch_event('pointerup')
 assert page.evaluate('window.__v054().screen')=='projects'
 for _ in range(4):page.locator('[data-turn="k1:1"]').click()
 assert page.evaluate('window.__v054().menu')==4
 page.locator('[data-action="SEL"]').click()
 assert page.evaluate('window.__v054().screen')=='help'
 assert page.evaluate('window.__v054().len')==43
 page.screenshot(path='/mnt/data/beetle_v054_work/v054_help.png')
 page.locator('[data-turn="k1:1"]').click()
 assert page.evaluate('window.__v054().help')==1
 page.locator('[data-turn="k4:1"]').click()
 assert page.evaluate('window.__v054().help')==4
 for _ in range(30):page.locator('[data-turn="k4:1"]').click()
 assert page.evaluate('window.__v054().help')==33
 page.locator('#homeButton').dispatch_event('pointerdown')
 page.locator('#homeButton').dispatch_event('pointerup')
 assert page.evaluate('window.__v054().screen')=='projects'
 page.locator('[data-action="LFO"]').click()
 page.locator('[data-turn="select:1"]').click()
 assert page.evaluate('window.__v054().fx')==1
 page.screenshot(path='/mnt/data/beetle_v054_work/v054_knobs.png')
 page.locator('[data-turn="select:1"]').click()
 assert page.evaluate('window.__v054().fx')==2
 assert not errors,errors
 print('PASS V0.54 WebSimulator HELP / K1-K4 scroll / HOME navigation / Compressor and Reverb screen, no JS exceptions')
 browser.close()
