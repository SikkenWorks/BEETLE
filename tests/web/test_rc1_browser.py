#!/usr/bin/env python3
from pathlib import Path
from playwright.sync_api import sync_playwright
p=Path(__file__).resolve().parents[3]/'simulator.html'
s=p.read_text()
anchor='})(TRACK_CORE,TRACK_AUDIO);</script>'
assert anchor in s
s=s.replace(anchor,"window.__rc1={model,route,whiteDown,liveValue,toggleLive,commitLiveMix,rotate,render};\n"+anchor)
with sync_playwright() as pw:
 b=pw.chromium.launch(executable_path='/usr/bin/chromium',args=['--no-sandbox'])
 page=b.new_page(viewport={'width':1100,'height':900}); errors=[]
 page.on('pageerror', lambda e:errors.append(str(e)))
 page.set_content(s);page.wait_for_timeout(1500)
 got=page.evaluate('''() => {
 const d=window.__rc1, m=d.model;
 m.screen='tracker';m.songMode=false;d.route('ARP');d.route('ARP');if(m.screen==='livemixer')throw Error('live opened with song OFF');
 d.route('SEL');if(!m.songMode)throw Error('song not armed');d.route('ARP');
 if(m.screen!=='livemixer')throw Error('live failed while song ON');
 m.playing=true;d.whiteDown(0);if(m.live.pendingSolo[0]!==true||m.project.mixer[0].solo)throw Error('Q solo not queued or base mutated');
 d.render();if(document.querySelector('#keyboard .white-key').classList.contains('solo-active'))throw Error('queued solo LED lit before commit');
 d.whiteDown(0);if(0 in m.live.pendingSolo)throw Error('Q solo cancellation broken');
 d.whiteDown(0);d.commitLiveMix();d.render();if(!d.liveValue('solo',0)||m.project.mixer[0].solo)throw Error('live solo commit/base isolation');if(!document.querySelector('#keyboard .white-key').classList.contains('solo-active'))throw Error('solo LED missing after commit');
 d.route('OCT+');if(m.live.qSolo)throw Error('OCT+ Q Solo toggle failed');
 d.route('OCT-');if(m.live.qMute)throw Error('OCT- Q Mute toggle failed');
 d.whiteDown(1);if(!d.liveValue('solo',1)||m.project.mixer[1].solo)throw Error('unquantized solo');
 m.screen='song';d.route('SEL');if(m.songMode||Object.keys(m.live.solo).length)throw Error('SONG OFF must bypass live solo');
 d.route('ARP');if(m.screen==='livemixer')throw Error('SONG OFF opens live');
 m.screen='tracker';m.playing=false;m.ch=5;m.col=2;m.row=0;
 const st=TRACK_CORE.step(m.project,m.project.selectedPattern,5,0,true);st.note=3;st.chd=6;
 d.route('ENV');if(st.note!==3||st.chd!==15)throw Error('NOTE2 ENV STP mutated NOTE1');
 m.screen='punch';m.punchFocus=1;m.punchHeld.add(1);const old=m.punchParams[1];d.rotate('k1',1);
 if(m.punchParams[1]!==old+1)throw Error('K1 Ring Mod increment should be +1');
 return {bootGate:'pass',soloQueuedAndVisible:'pass',noPersistedLiveSolo:'pass',qMuteQSolo:'pass',note2Env:'pass',ringModStep:'pass'};
}''')
 print('PASS RC1 Chromium',got)
 assert not errors,errors
 b.close()
