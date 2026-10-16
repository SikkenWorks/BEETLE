from playwright.sync_api import sync_playwright
h=open(__import__('pathlib').Path(__file__).resolve().parents[2]/'web'/'BEETLE-Project-Manager.html').read()
with sync_playwright() as p:
 b=p.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox'])
 page=b.new_page(viewport={'width':1300,'height':980}); errors=[];page.on('pageerror',lambda x:errors.append(str(x)))
 page.set_content(h)
 page.evaluate('''() => {
  Object.defineProperty(window,'isSecureContext',{configurable:true,value:true});
  const inp={id:'A',name:'BEETLE FM1 USB MIDI',onmidimessage:null,open:async()=>{},close:async()=>{}};
  const out={id:'B',name:'BEETLE FM1 USB MIDI',open:async()=>{},close:async()=>{},send(data){
    const req=parseFrame(data),seq=req.body[0],a=req.body.slice(1);let c,p;
    if(req.command===0x50){c=0x60;p=[seq,1,12,17,1]}
    else if(req.command===0x58){c=0x64;p=[seq]}
    else if(req.command===0x51){let slot=a[0],used=slot===0||slot===8;
      c=0x61;p=[seq,slot,used?1:0,...Array.from({length:12},(_,i)=>used?('HELLO BEETLE'[i]||' ').charCodeAt(0):32)];
    }else{c=0x6f;p=[seq,req.command,10]}
    setTimeout(()=>inp.onmidimessage?.({data:frame(c,p)}),0);
  }};
  Object.defineProperty(navigator,'requestMIDIAccess',{configurable:true,value:async()=>({inputs:new Map([['A',inp]]),outputs:new Map([['B',out]])})});
 }''')
 page.get_by_role('button',name='CONNECT FM1').click()
 page.wait_for_function("document.querySelector('#connectionState').textContent==='FM1 CONNECTED' && document.querySelector('#used').textContent==='02'",timeout=10000)
 print('connected',page.locator('#connectionState').inner_text(),'used',page.locator('#used').inner_text(),'slot0',page.locator('.slot').nth(0).inner_text())
 page.locator('.slot').nth(8).click();print('selected',page.locator('#selectIndex').inner_text(),page.locator('#selectName').inner_text())
 print('errors',errors)
 assert not errors
 b.close()
