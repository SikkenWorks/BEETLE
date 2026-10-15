from pathlib import Path
from playwright.sync_api import sync_playwright
p=Path(__file__).resolve().parents[2]/'web/simulator.html'
s=p.read_text()
s=s.replace("const BLOCK_OPS=['COPY','PASTE','CLEAR','TRANSPOSE','SHIFT','UNDO','REDO'];", "const BLOCK_OPS=['COPY','PASTE','CLEAR','TRANSPOSE','SHIFT','UNDO','REDO'];\nwindow.__beetleTest={model,editTools,rotate,editShortcut,historyMove,rememberDomain,libraryActionApply,editReset};",1)
# Run by set_content; no HTTP or file URL required.
with sync_playwright() as pw:
 browser=pw.chromium.launch(executable_path='/usr/bin/chromium',args=['--no-sandbox'])
 page=browser.new_page(viewport={'width':1280,'height':950})
 errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
 page.set_content(s)
 page.wait_for_timeout(1600)
 data=page.evaluate('''() => {
   const {model,editTools,rotate,historyMove,libraryActionApply}=window.__beetleTest;
   model.screen='instrument';model.instBank=0;model.instRow=1;
   const oldAttack=model.project.instruments[0].attack;
   rotate('k3',1);const instNew=model.project.instruments[0].attack;
   model.screen='mixer';model.mixerRow=0;model.mixerCol=0;
   const oldVol=model.project.mixer[0].volume;rotate('k3',1);const newVol=model.project.mixer[0].volume;
   model.screen='song';model.songRow=0;model.songCol=1;
   const oldRep=model.project.song[0].repeats;rotate('k3',1);const newRep=model.project.song[0].repeats;
   model.screen='masterfx';model.masterFocus=0;
   const oldEq=model.project.master.eq0;rotate('k1',1);const newEq=model.project.master.eq0;
   model.screen='library';model.librarySlot=0;model.libraryAction=0;model.libraryTarget=0;libraryActionApply();
   let count=editTools.count, cursor=editTools.cursor, kinds=editTools.history.map(x=>x.kind);
   let undo=[];for(let i=0;i<5;i++)undo.push(historyMove());
   const undone={attack:model.project.instruments[0].attack,volume:model.project.mixer[0].volume,repeats:model.project.song[0].repeats,eq0:model.project.master.eq0,lib:model.project.library[0]};
   let redo=[];for(let i=0;i<5;i++)redo.push(historyMove(true));
   const redone={attack:model.project.instruments[0].attack,volume:model.project.mixer[0].volume,repeats:model.project.song[0].repeats,eq0:model.project.master.eq0,lib:model.project.library[0]};
   return {oldAttack,instNew,oldVol,newVol,oldRep,newRep,oldEq,newEq,count,cursor,kinds,undo,redo,undone,redone};
 }''')
 print(data)
 assert not errors, errors
 assert data['kinds']==['instrument','mixer','song','master','library'],data['kinds']
 assert all(data['undo']) and all(data['redo'])
 assert data['undone']['attack']==data['oldAttack']
 assert data['undone']['volume']==data['oldVol']
 assert data['undone']['repeats']==data['oldRep']
 assert data['redone']['attack']==data['instNew']
 assert data['redone']['volume']==data['newVol']
 assert data['redone']['repeats']==data['newRep']
 # Do not create test images inside the release tree.
 browser.close()
print('PASS V0.70 browser mixed-domain Undo/Redo, five domains')
