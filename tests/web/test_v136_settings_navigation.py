#!/usr/bin/env python3
"""V1.3.6 Settings control is truly accessible through HOME → K1 → SEL."""
from pathlib import Path
from playwright.sync_api import sync_playwright
root=Path(__file__).resolve().parents[2]
with sync_playwright() as p:
 browser=p.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
 page=browser.new_page(viewport={'width':1450,'height':1000})
 errors=[]
 page.on('pageerror',lambda error: errors.append(str(error)))
 page.set_content((root/'simulator.html').read_text(),wait_until='domcontentloaded',timeout=60000)
 page.wait_for_function('!!document.getElementById("beetle-engine")?.contentWindow?.BEETLE_WORKBENCH_BRIDGE',timeout=45000)
 def state():
  return page.evaluate('document.getElementById("beetle-engine").contentWindow.BEETLE_WORKBENCH_BRIDGE.state()')
 def rotate(knob,n):
  page.evaluate('''a=>document.getElementById("beetle-engine").contentWindow.BEETLE_WORKBENCH_BRIDGE.rotate(a[0],a[1])''',[knob,n])
 def click(button):
  page.locator(f'.panel-button[data-button="{button}"]').click(timeout=15000)
 assert state()['screen']=='boot',state()
 click('SEL')
 assert state()['screen']=='tracker',state()
 click('HOME')
 assert state()['screen']=='projects',state()
 rotate('k1',6)
 assert state()['screen']=='projects',state()
 click('SEL')
 assert state()['screen']=='settings',state()
 off=state()['usbMultitrack']
 click('SEL')
 assert state()['screen']=='settings' and state()['usbMultitrack']!=off,state()
 click('SEL')
 assert state()['screen']=='settings' and state()['usbMultitrack']==off,state()
 click('HOME')
 assert state()['screen']=='projects',state()
 click('HOME')
 assert state()['screen']=='projects',state() # short HOME remains project menu
 rotate('k1',-6)
 click('SEL')  # SAVE is separate and must not trigger tracker delete
 assert state()['screen']=='name',state()  # SAVE correctly opens its own name editor
 click('HOME')
 assert state()['screen']=='projects',state()
 assert not errors,errors
 print('PASS V1.3.6: HOME → PROJECTS → SETTINGS SEL enters, SEL toggles, HOME backs, no JS exception')
 browser.close()
