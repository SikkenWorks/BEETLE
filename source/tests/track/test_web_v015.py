"""UI behavioral integration test; injects read-only instrumentation in the browser copy.
No test hooks are added to the shipped release HTML.
"""
from pathlib import Path
from playwright.sync_api import sync_playwright
p=Path(__file__).resolve().parents[2]/'TRACK-Web-V0.15.html'
s=p.read_text(encoding='utf-8')
s=s.replace('const audio=new TrackAudio();let homeTimer=', 'const audio=new TrackAudio();window.__trackV015Test={model,audio};let homeTimer=')
assert 'window.__trackV015Test' in s
with sync_playwright() as pw:
 b=pw.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox'])
 page=b.new_page();errors=[];page.on('pageerror',lambda err:errors.append(str(err)))
 page.set_content(s,wait_until='domcontentloaded',timeout=30000)
 page.wait_for_timeout(1400)
 get=lambda expr:page.evaluate('()=>'+expr)
 assert get('window.__trackV015Test.model.project.selectedPattern')==0
 page.locator('[data-turn="algorithm:1"]').click()
 assert get('window.__trackV015Test.model.project.selectedPattern')==1
 page.locator('[data-turn="select:1"]').click()
 assert get('window.__trackV015Test.model.ch')==1
 # SELECT never edits CHD in TRACK V0.15.
 page.locator('[data-turn="k2:1"]').click();page.locator('[data-turn="k2:1"]').click()
 assert get('window.__trackV015Test.model.col')==3
 page.locator('[data-turn="select:1"]').click()
 assert get('window.__trackV015Test.model.ch')==2
 page.locator('[data-action="REC"]').click()
 assert get('window.__trackV015Test.model.record')
 page.locator('.white-key').first.dispatch_event('pointerdown',{'pointerId':1})
 assert get('window.__trackV015Test.model.punchHeld.has(0)')
 page.locator('[data-turn="algorithm:1"]').click()
 page.locator('[data-turn="algorithm:1"]').click()
 assert get('window.__trackV015Test.model.punchParams[0]')==56
 assert get('window.__trackV015Test.audio.punchParams[0]')==56
 assert get('window.__trackV015Test.model.project.selectedPattern')==1
 page.locator('.white-key').first.dispatch_event('pointerup',{'pointerId':1})
 assert get('window.__trackV015Test.model.punchParams[0]')==50
 assert get('window.__trackV015Test.audio.punchParams[0]')==50
 # No notes were written by REC keyboard input.
 assert not get('Object.keys(window.__trackV015Test.model.project.patterns).length')
 # MUTE is not a parameterized FX.
 page.locator('.white-key').nth(7).dispatch_event('pointerdown',{'pointerId':2})
 page.locator('[data-turn="algorithm:1"]').click()
 assert get('window.__trackV015Test.model.punchParams[7]')==50
 page.locator('.white-key').nth(7).dispatch_event('pointerup',{'pointerId':2})
 assert not errors,errors
 print('PASS browser ALG pattern, SELECT channel, 56->50 temporary FX, REC not note entry, MUTE fixed, no JS errors')
 b.close()
