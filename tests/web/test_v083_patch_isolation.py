"""V0.83 live Chromium regression for legacy song waveform protection."""
from pathlib import Path
from playwright.sync_api import sync_playwright
r=Path(__file__).resolve().parents[2]
p=r.parent/'simulator.html'
with sync_playwright() as pw:
 b=pw.chromium.launch(executable_path='/usr/bin/chromium',args=['--no-sandbox'])
 page=b.new_page(viewport={'width':1200,'height':1000})
 errs=[];page.on('pageerror',lambda e:errs.append(str(e)))
 html=p.read_text().replace('function whiteDown(i){',
  'window.__v83test={model,rotate,resetInstrumentFocus,whiteDown:(i)=>whiteDown(i),core:TRACK_CORE};function whiteDown(i){')
 page.set_content(html)
 page.wait_for_timeout(750)
 result=page.evaluate('''() => {
  const {model,rotate,resetInstrumentFocus,whiteDown,core}=window.__v83test;
  model.project=core.freshProject('V83 TEST');resetInstrumentFocus();
  model.project.patterns[0]=core.emptyPattern();
  const old=model.project.patterns[0].channels[0][0];
  old.note=60;old.inst=0;
  const oldType=model.project.instruments[0].type;
  model.screen='instrument';model.ch=0;model.row=1;model.instBank=0;model.instRow=0;
  rotate('k3',1);
  const newSlot=model.inputInstrument[0];
  const previousType=model.project.instruments[0].type;
  const newType=model.project.instruments[newSlot].type;
  const preserved=old.note===60&&old.inst===0&&previousType===oldType;
  model.screen='tracker';model.col=1;model.row=1;
  whiteDown(0);
  const newStep=model.project.patterns[0].channels[0][1];
  const newNoteFocus=(newStep.inst===newSlot&&newStep.note!==null);
  // No vacant patches: editor must refuse rather than changing recorded song.
  model.project.patterns[1]=core.emptyPattern();
  for(let n=0;n<4;n++){const st=model.project.patterns[1].channels[n][0];st.note=62+n;st.inst=n;}
  const fourBefore=model.project.instruments.map(x=>x.type);
  model.screen='instrument';rotate('k3',1);
  const guarded=model.patchSlotsFull&&model.project.instruments.every((x,i)=>x.type===fourBefore[i]);
  return {newSlot,oldType,newType,preserved,newNoteFocus,guarded};
 }''')
 assert result['preserved'],result
 assert result['newSlot']!=0,result
 assert result['newType']!=result['oldType'],result
 assert result['newNoteFocus'],result
 assert result['guarded'],result
 assert not errs,errs
 print('PASS V0.83 Chromium: old song unchanged; new notes use new patch; full slots refuse edits')
 b.close()
