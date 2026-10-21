from playwright.sync_api import sync_playwright
from pathlib import Path
h=(Path(__file__).resolve().parents[3]/'BEETLE-Project-Manager.html').read_text()
with sync_playwright() as p:
 b=p.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox'])
 for proto in (1,2):
  page=b.new_page(viewport={'width':1200,'height':900});errors=[];page.on('pageerror',lambda e:errors.append(str(e)));page.set_content(h)
  page.evaluate('''(protocol) => {
   Object.defineProperty(window,'isSecureContext',{configurable:true,value:true});
   window.testDeleteCount=0;window.slotNames=['SONG ONE',...Array(7).fill(null),'SONG NINE',...Array(3).fill(null)];
   const input={id:'A',name:'FM1 MIDI',onmidimessage:null,open:async()=>{},close:async()=>{}};
   const output={id:'B',name:'FM1 MIDI',open:async()=>{},close:async()=>{},send(data){
    const req=parseFrame(data),seq=req.body[0],a=req.body.slice(1);let c,p;
    if(req.command===0x50){c=0x60;p=[seq,protocol,12,17,1]}
    else if(req.command===0x58){c=0x64;p=[seq]}
    else if(req.command===0x51){const slot=a[0],name=window.slotNames[slot];c=0x61;
     p=[seq,slot,name?1:0,...Array.from({length:12},(_,i)=>name?(name[i]||' ').charCodeAt(0):32)];}
    else if(req.command===0x59&&protocol===2){const slot=a[0];if(a[1]!==0xd3||a[2]!==0x7e||!window.slotNames[slot]){c=0x6f;p=[seq,req.command,7]}
     else{window.testDeleteCount++;window.slotNames[slot]=null;c=0x66;p=[seq,slot]}}
    else{c=0x6f;p=[seq,req.command,10]}
    setTimeout(()=>input.onmidimessage?.({data:frame(c,p)}),0);
   }};
   Object.defineProperty(navigator,'requestMIDIAccess',{configurable:true,value:async()=>({inputs:new Map([['A',input]]),outputs:new Map([['B',output]])})});
  }''',proto)
  page.locator('#connect').click();page.wait_for_function("document.querySelector('#connectionState').textContent==='FM1 CONNECTED' && document.querySelector('#used').textContent==='02'",timeout=15000)
  page.locator('.slot').nth(8).click()
  if proto==1:
   assert page.locator('#clear').is_disabled(), 'V1 must prohibit delete'
   assert page.locator('#import').is_enabled() and page.locator('#export').is_enabled()
   print('PASS V1: delete disabled, import/export retained')
  else:
   assert page.locator('#clear').is_enabled()
   page.locator('#clear').click()
   page.get_by_role('button',name='CANCEL').click();assert page.evaluate('window.testDeleteCount')==0
   page.locator('#clear').click();page.get_by_role('button',name='DELETE WITHOUT BACKUP').click()
   page.locator('#modalExtra input').check()
   page.get_by_role('button',name='YES · DELETE PROJECT').click()
   page.wait_for_function("document.querySelector('#used').textContent==='01'",timeout=20000)
   assert '[ EMPTY ]' in page.locator('.slot').nth(8).inner_text()
   assert 'SONG ONE' in page.locator('.slot').nth(0).inner_text()
   assert page.evaluate('window.testDeleteCount')==1
   assert page.locator('#clear').is_disabled()
   print('PASS V2: cancel safe, delete one slot, [ EMPTY ] after readback, other slot retained')
  assert not errors,errors
  page.close()
 b.close()
