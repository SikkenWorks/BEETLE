#!/usr/bin/env python3
"""Exercise V1.3.5 real Workbench controls against the embedded BEETLE engine.
Browser capture is software-only, not a hardware FM1 test.
"""
import re,base64
from pathlib import Path
from playwright.sync_api import sync_playwright
root=Path(__file__).resolve().parents[2]
s=(root/'simulator.html').read_text()
m=re.search(r"const inner='([^']+)';",s);assert m
engine=base64.b64decode(m[1]).decode()
bridge='window.BEETLE_WORKBENCH_BRIDGE={'
assert bridge in engine
engine=engine.replace(bridge,"window.__BEETLE_SET_TEST_ROW=n=>model.row=n; window.__BEETLE_BLACK_KEY_TEST=()=>({note0:step(model.project,model.project.selectedPattern,0,0,false).note,note1:step(model.project,model.project.selectedPattern,0,1,false).note,note2:step(model.project,model.project.selectedPattern,0,2,false).note,row:model.row,mute0:!!model.project.mixer[0].mute,screen:model.screen,fxPool:typeof fxPoolCount==='function'?fxPoolCount(model.project):null});\n"+bridge,1)
s=s[:m.start(1)]+base64.b64encode(engine.encode()).decode()+s[m.end(1):]
with sync_playwright() as p:
 browser=p.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
 page=browser.new_page(viewport={'width':1400,'height':900})
 errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
 page.set_content(s,wait_until='domcontentloaded',timeout=45000)
 page.wait_for_function('!!document.getElementById("beetle-engine").contentWindow?.BEETLE_WORKBENCH_BRIDGE',timeout=30000)
 def run(statement):return page.evaluate('''s=>{let w=document.getElementById('beetle-engine').contentWindow,b=w.BEETLE_WORKBENCH_BRIDGE;return (function(){'''+statement+'''})();}''','')
 def state():return page.evaluate('document.getElementById("beetle-engine").contentWindow.__BEETLE_BLACK_KEY_TEST()')
 run("b.press('SEL',true);b.press('SEL',false);")
 assert page.locator('.piano-key').count()==27
 assert page.locator('.piano-key.black').count()==11
 # Piano F3 -> F#3 -> G3 sequentially stores MIDI 53,54,55.
 for k in [0,1,2]:run(f'b.physicalKey({k},true);b.physicalKey({k},false);')
 result=state();assert [result['note0'],result['note1'],result['note2']]==[53,54,55],result
 # Check all 27 physical semitones at their real note values (F3=53 .. G5=79).
 for k in range(27):
  run(f'w.__BEETLE_SET_TEST_ROW(0);b.physicalKey({k},true);b.physicalKey({k},false);')
  assert state()['note0']==53+k,(k,state())
 # Restore known steps for subsequent mixer-isolation assertion.
 for k in [0,1,2]:
  run(f'w.__BEETLE_SET_TEST_ROW({k});b.physicalKey({k},true);b.physicalKey({k},false);')
 result=state()
 # Chromatic notes do not toggle mute in TRACKER.
 assert result['mute0']==False,result
 # SEQ+OP1 invokes copy shortcut without inserting an extra note.
 run("b.press('SEQ',true);b.physicalKey(1,true);b.physicalKey(1,false);b.press('SEQ',false);")
 after=state();assert after['row']==result['row'],after
 # Mixer black key 1 toggles CH1 mute (separate from tracker behavior).
 run("b.page('mixer');b.physicalKey(1,true);b.physicalKey(1,false);")
 assert state()['mute0'] is True,state()
 # Mixer white key does not insert a note.
 run("b.physicalKey(0,true);b.physicalKey(0,false);")
 assert [state()[f'note{i}'] for i in range(3)]==[53,54,55],state()
 assert errors==[], errors
 print('PASS: all 27 keys rendered, chromatic F3 F#3 G3 stored, context-sensitive SEQ OP1 and mixer mute retained, no JS errors')
 browser.close()
