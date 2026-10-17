from playwright.sync_api import sync_playwright
from pathlib import Path
html=(Path(__file__).resolve().parents[3]/'BEETLE-Project-Manager.html').read_text()
with sync_playwright() as p:
 browser=p.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox'])
 for protocol in (2,3):
  page=browser.new_page(viewport={'width':1180,'height':850}); errors=[]
  page.on('pageerror',lambda error:errors.append(str(error)))
  page.set_content(html)
  page.evaluate('''(protocol)=>{
   Object.defineProperty(window,'isSecureContext',{configurable:true,value:true});
   window.deleted=[];window.refuseForeign=false;
   window.testState=Array(12).fill(0);window.testState[3]=2;window.testState[4]=1;
   const inp={id:'A',name:'FM1',onmidimessage:null,open:async()=>{},close:async()=>{}};
   const out={id:'B',name:'FM1',open:async()=>{},close:async()=>{},send(data){
    const req=parseFrame(data),seq=req.body[0],args=req.body.slice(1);let cmd,response;
    switch(req.command){
     case 0x50:cmd=0x60;response=[seq,protocol,12,17,1];break;
     case 0x58:cmd=0x64;response=[seq];break;
     case 0x51:{const slot=args[0],state=window.testState[slot];cmd=0x61;
      const name=state===1?'VALID':'';
      response=[seq,slot,state,...Array.from({length:12},(_,i)=>name.charCodeAt(i)||32)];break;}
     case 0x5a:{const slot=args[0];if(protocol!==3||window.refuseForeign||window.testState[slot]!==2||args[1]!==0xd3||args[2]!==0x7e||args[3]!==0xc9||args[4]!==0x34){cmd=0x6f;response=[seq,0x5a,11];}
      else{window.deleted.push(slot);window.testState[slot]=0;cmd=0x66;response=[seq,slot];}break;}
     default:cmd=0x6f;response=[seq,req.command,10];break;
    }
    setTimeout(()=>inp.onmidimessage?.({data:frame(cmd,response)}),0);
   }};
   Object.defineProperty(navigator,'requestMIDIAccess',{configurable:true,value:async()=>({inputs:new Map([['A',inp]]),outputs:new Map([['B',out]])})});
  }''',protocol)
  page.locator('#connect').click()
  page.wait_for_function("document.querySelector('#connectionState').textContent==='FM1 CONNECTED' && document.querySelector('#selectState')",timeout=15000)
  page.locator('.slot').nth(3).click()
  assert 'READ ERROR' in page.locator('#selectState').inner_text()
  if protocol==2:
   assert page.locator('#recover').is_disabled();print('PASS V2 corrupt DELETE refused until firmware V3')
  else:
   assert page.locator('#recover').is_enabled()
   page.locator('#recover').click();page.get_by_role('button',name='CANCEL').click()
   assert page.evaluate('window.deleted.length')==0
   page.locator('#recover').click();page.get_by_role('button',name='CONTINUE TO FINAL CONFIRMATION').click()
   page.locator('#modalExtra input').check()
   page.get_by_role('button',name='ERASE CORRUPT SLOT').click()
   page.wait_for_function("document.querySelectorAll('.slot')[3].textContent.includes('[ EMPTY ]')",timeout=20000)
   assert page.evaluate('window.deleted.join()')=='3'
   assert 'VALID' in page.locator('.slot').nth(4).inner_text()
   assert page.locator('#recover').is_disabled()
   print('PASS V3 corrupt recovery, confirmations, selected slot only, readback EMPTY')
  assert not errors,errors
  page.close()
 browser.close()
