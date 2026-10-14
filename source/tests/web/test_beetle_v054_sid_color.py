"""Browser smoke check for non-identical SID filter and edit non-SID sound."""
from pathlib import Path
from playwright.sync_api import sync_playwright
p=Path(__file__).resolve().parents[2]/'web/BEETLE-V0.54-WebSimulator.html'
with sync_playwright() as pw:
 browser=pw.chromium.launch(headless=True, executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage','--autoplay-policy=no-user-gesture-required'])
 page=browser.new_page()
 page.set_content(p.read_text().replace('const audio=new TrackAudio();','const audio=new TrackAudio();window.__beetleTestAudio=audio;window.__beetleTestInstrument=()=>model.project.instruments[0];'),wait_until='load',timeout=30000)
 result=page.evaluate('''async () => {
   const audio=window.__beetleTestAudio; await audio.init();
   const i=window.__beetleTestInstrument();
   const f1=audio.ctx.createBiquadFilter(),f2=audio.ctx.createBiquadFilter(),f0=audio.ctx.createBiquadFilter();
   let freq6581=0,freq8580=0;
   const orig1=f1.frequency.setValueAtTime.bind(f1.frequency),orig2=f2.frequency.setValueAtTime.bind(f2.frequency);
   f1.frequency.setValueAtTime=(v,t)=>{freq6581=v;return orig1(v,t);};
   f2.frequency.setValueAtTime=(v,t)=>{freq8580=v;return orig2(v,t);};
   const a=audio.sidCharacter(4,i,f1,audio.ctx.currentTime),b=audio.sidCharacter(5,i,f2,audio.ctx.currentTime);
   f0.frequency.value=20000;
   const other=audio.sidCharacter(0,i,f0,audio.ctx.currentTime);
   return {f6581:freq6581,f8580:freq8580,shape6581:!!a,shape8580:!!b,other:other===null,defaultNonSid:f0.frequency.value};
 }''')
 print(result)
 assert result['f6581'] < result['f8580'],result
 assert result['shape6581'] and not result['shape8580'],result
 assert result['other'] and result['defaultNonSid'] == 20000,result
 browser.close()
print('PASS V0.54 SID-specific browser graph and unaffected non-SID audition')
