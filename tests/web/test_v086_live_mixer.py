#!/usr/bin/env python3
"""Browser-level regression: V0.86 Live Mixer, default volumes, quantized queues."""
from pathlib import Path
from playwright.sync_api import sync_playwright
p=Path(__file__).resolve().parents[2]/'web/simulator.html'
s=p.read_text()
# Private test-only handle injected in memory, not shipped with simulator.
anchor='})(TRACK_CORE,TRACK_AUDIO);</script>'
assert anchor in s
s=s.replace(anchor,"window.__testV086={model,route,whiteDown,rotate,changeLiveMute};\n"+anchor)
with sync_playwright() as pw:
 b=pw.chromium.launch(executable_path='/usr/bin/chromium',args=['--no-sandbox'])
 page=b.new_page(viewport={'width':1280,'height':960})
 errors=[]
 page.on('pageerror',lambda e:errors.append(str(e)))
 page.set_content(s)
 page.wait_for_timeout(1450)
 got=page.evaluate('''() => {
  const d=window.__testV086;
  const p=TRACK_CORE.freshProject('TEST 86');
  if(p.quantizeMutes!==false)throw Error('quantized default must be off');
  if(JSON.stringify(p.mixer.map(m=>m.volume))!==JSON.stringify([.5,.5,.5,.5,.5,.8]))throw Error('channel defaults');
  const restored=TRACK_CORE.parseProject(JSON.stringify({...p,mixer:p.mixer.map((m,i)=>({...m,volume:i===0?1.2:m.volume}))}));
  if(restored.mixer[0].volume!==1.2)throw Error('120% project reload');
  d.model.screen='tracker';d.route('ARP');if(d.model.screen!=='song')throw Error('first ARP must open song');
  d.route('ARP');if(d.model.screen!=='livemixer')throw Error('second ARP must open Live Mixer');
  // Test physical key press while playback active without starting the sound device.
  d.model.playing=true;d.model.muteQueue={};d.whiteDown(0);d.whiteDown(5);
  if(d.model.muteQueue[0]!==true||d.model.muteQueue[5]!==true)throw Error('quantized CH1/CH6 queue');
  if(d.model.project.mixer[0].mute||d.model.project.mixer[5].mute)throw Error('Q changes applied early');
  d.whiteDown(0);if(0 in d.model.muteQueue)throw Error('second tap should cancel CH1 pending');
  d.model.screen='tracker';d.route('ARP');if(d.model.screen!=='song')throw Error('ARP must return to song');
  d.model.playing=false;d.model.muteQueue={};d.route('ARP');d.whiteDown(2);
  if(d.model.project.mixer[2].mute!==true)throw Error('stopped Live Mixer should directly toggle CH3');
  // Normal MIXER should NOT be forced into Q mode
  d.route('GLO');if(d.model.screen!=='mixer')throw Error('GLO should open mixer');
  d.model.mixerRow=0;d.model.mixerCol=0;d.rotate('k3',40);
  if(d.model.project.mixer[0].volume!==1.2)throw Error('120% mixer encoder clamp');
  return {screen:d.model.screen,volume:d.model.project.mixer[0].volume,quantizeDefault:p.quantizeMutes};
 }''')
 print('PASS V0.86 Chromium Live Mixer routes, 6-key queue and cancellation, Q default, 120% fader, project read',got)
 assert not errors,errors
 b.close()
