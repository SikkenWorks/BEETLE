"""RC1.0.8 simulator integration; requires playwright and Chromium. No hardware."""
from pathlib import Path
import os
from playwright.sync_api import sync_playwright
ROOT=Path(__file__).resolve().parents[2]
with sync_playwright() as pw:
    browser=pw.chromium.launch(headless=True,executable_path=os.getenv('CHROME_BIN','/usr/bin/chromium'),args=['--no-sandbox','--disable-dev-shm-usage'])
    page=browser.new_page(viewport={'width':1280,'height':940})
    errors=[]
    page.on('pageerror',lambda e:errors.append(str(e)))
    page.set_content((ROOT/'simulator.html').read_text(),wait_until='domcontentloaded')
    page.wait_for_timeout(1300)
    page.locator('[data-action="SEL"]').click()
    page.locator('#keyboard .white-key').first.click()
    img=lambda: page.evaluate("Array.from(document.querySelector('#screen').getContext('2d').getImageData(0,0,240,240).data).join(',')")
    before=img()
    page.locator('[data-edit="clearLane"]').click()
    assert img()!=before,'Confirmation modal did not appear'
    page.locator('[data-action="SEL"]').click() # NO by default
    assert img()==before,'NO unexpectedly changed the lane'
    page.locator('[data-edit="clearLane"]').click()
    page.locator('[data-turn="k1:1"]').click() # YES
    page.locator('[data-action="SEL"]').click()
    after=img()
    assert after!=before,'YES did not clear the current lane'
    page.locator('[data-edit="undo"]').click()
    assert img()==before,'Undo did not restore the selected lane'
    assert not errors,errors
    print('PASS RC1.0.8 browser: NO cancels; YES clears; UNDO restores; no JS exceptions')
    browser.close()
