from pathlib import Path
p=Path('/mnt/data/beetle_v134_final')
def patch(rel,old,new,n=1):
 f=p/rel;s=f.read_text();c=s.count(old)
 if c!=n:raise RuntimeError(f'{rel}: expected {n}, got {c} for {old[:110]}')
 f.write_text(s.replace(old,new))
# eliminate misleading full popup on invalid insertion
patch('firmware/fm1-native/track_runtime.h','if(rc<0)track_fx_full_until=etch_ms+1800u;','if(rc==-1)track_fx_full_until=etch_ms+1800u;')
patch('firmware/fm1-native/track_runtime.h','}else if(next)track_fx_full_until=etch_ms+1800u;','} /* An empty FX1 is not a full storage pool. */')
patch('firmware/fm1-native/track_runtime.h',"cap[at++]='1';cap[at++]='6';cap[at]=0;","cap[at++]='4';cap[at++]='8';cap[at]=0;")
# prune deleted primary fx from block edits
f=p/'firmware/fm1-native/beetle_pattern_edit.h';s=f.read_text();idx=s.index('static inline int beetle_edit_block_execute(');end=s.index('#endif',idx);s0=s[idx:end];j=s0.rfind('if(beetle_edit_commit(');print('block trailer',s0[j:j+160]);
if j>=0 and 'track_extra_prune' not in s0[j:j+160]:
 s=s[:idx]+s0[:j]+'track_extra_prune(bank,eng->pattern_slot,&eng->active);\n '+s0[j:]+s[end:];f.write_text(s)
# update manual
f=p/'firmware/fm1-native/beetle_help.h';s=f.read_text()
old=' "FX: EDIT STEP EFFECTS",';new=''' "FX: EDIT STEP EFFECTS (3 SLOTS)",
 "FX1 PRIMARY; FX2/FX3 ADDITIONAL",
 "FX MENU K2: CHOOSE FX SLOT",
 "FX MENU OCT+: NEXT FX SLOT",
 "FX MENU OCT-: DELETE FX SLOT",
 "FX MENU K1 PARAM K3 ADJUST",
 "CH6 FX: K2 NOTE1/NOTE2",
 "TRK FX: MUL IF 2 OR MORE FX",
 "48 EXTRA FX PROJECT-WIDE",
 "FULL: FX STORAGE FULL OVERLAY",'''
assert old in s;s=s.replace(old,new);f.write_text(s)
patch('firmware/fm1-native/build.py','BEETLE V1.3.3','BEETLE V1.3.4',2)
# JS simulator
f=p/'simulator.html';s=f.read_text();s=s.replace('BEETLE V1.3.3','BEETLE V1.3.4').replace('V1.3.3','V1.3.4')
s=s.replace('function emptyStep(){return {note:null,inst:0,chd:0,fx:0,amount:50,dyn:0,groove:0};}', 'function emptyStep(){return {note:null,inst:0,chd:0,fx:0,amount:50,extraFx:[],dyn:0,groove:0};}')
# no need enumerate all models; attach attribute following constant
s=s.replace('const model={rangeSelection:null,usbMultitrack:false,','const model={fxSlot:0,fxFull:false,rangeSelection:null,usbMultitrack:false,')
# global pool functions in closure; drum NOTE2 own storage
idx=s.index('function fxEditor(){')
helpers='''const FX_EXTRA_LIMIT=48;
function fxExtras(st,drum2=false){if(drum2)return st.extraFx2??(st.extraFx2=[]);return st.extraFx??(st.extraFx=[]);}
function fxPoolCount(p){let n=0;for(const pat of Object.values(p.patterns||{}))for(let ch=0;ch<6;ch++)for(const st of pat.channels[ch]||[]){n+=(st.extraFx?.length||0)+(st.extraFx2?.length||0);}return n;}
function fxTarget(st){const drum2=model.ch===5&&model.fxDrumLane===1;
 if(drum2){const p=pattern(model.project,model.project.selectedPattern,true);const f=p.note2Fx??(p.note2Fx=Array.from({length:16},()=>({effect:0,value:50})));return {root:f[model.row],list:fxExtras(st,true),second:true};}
 return {root:st,list:fxExtras(st),second:false};
}
function fxFullOverlay(){model.fxFull=true;announce('FX STORAGE FULL');}
'''
s=s[:idx]+helpers+s[idx:]
start=s.index('function fxEditor(){');end=s.index('function punchScreen(){',start)
s=s[:start]+'''function fxEditor(){top('PER STEP FX');line(42);
 const st=step(model.project,model.project.selectedPattern,model.ch,model.row,true);
 const drum=model.ch===5,hit=drum?(model.fxDrumLane===0?st.note:(st.chd>0?st.chd-1:null)):st.note;
 const labelHit=drum?(hit==null||hit<0||hit>=DRUM_NAMES.length?'---':DRUM_NAMES[hit]):noteLabel(hit);
 text('STEP '+String(model.row).padStart(2,'0')+' '+(drum?'NOTE'+(model.fxDrumLane+1):'NOTE'),16,52,PINK,9);
 text(labelHit,174,52,SOFT,10);
 const tgt=fxTarget(st),rows=[tgt.second?{fx:tgt.root.effect,amount:tgt.root.value}:{fx:st.fx,amount:st.amount},...tgt.list];
 const options=Array.from({length:3},(_,j)=>{const x=rows[j]??{fx:0,amount:50};return 'FX'+(j+1)+' '+STEP_FX[x.fx||0]+' '+(x.amount??50);});
 list(options,model.fxSlot,78,26);
 text('K1 PARAM  K3 CHANGE',15,176,PINK,8);
 text('OCT+ SLOT  OCT- REMOVE',15,189,PINK,8);
 text(drum?'K2 NOTE1/NOTE2':'K2 FX SLOT',15,201,WHITE,8);
 text('FX STORAGE '+fxPoolCount(model.project)+'/48',15,214,PINK,8);
 if(model.fxFull){rect(13,95,214,53,'#20051c');rect(13,95,214,2,PINK);text('FX STORAGE FULL',120,109,WHITE,9,'center');text('REMOVE FX TO FREE SPACE',120,127,PINK,8,'center');}
}
''' +s[end:]
# change both tracker FX cell rendering strings on one line with literal matching
s=s.replace("st.fx===0?'---':STEP_FX[st.fx].substring(0,3)","st.fx===0?'---':(st.extraFx?.length?'MUL':STEP_FX[st.fx].substring(0,3))")
# update direct tracker FX modification to wipe extras when primary becomes empty
s=s.replace("s.fx=((s.fx+delta)%STEP_FX.length+STEP_FX.length)%STEP_FX.length;","s.fx=((s.fx+delta)%STEP_FX.length+STEP_FX.length)%STEP_FX.length;if(s.fx===0)s.extraFx=[];")
old="else if(model.screen==='stepfx'){if(model.fxParam===0)st.fx=(st.fx+delta+STEP_FX.length)%STEP_FX.length;else st.amount=clamp(st.amount+delta*5,0,100);mark();}"
new='''else if(model.screen==='stepfx'){
 const tgt=fxTarget(st),slot=model.fxSlot,old=slot===0?(tgt.second?{fx:tgt.root.effect,amount:tgt.root.value}:{fx:st.fx,amount:st.amount}):tgt.list[slot-1];
 if(model.fxParam===0){
  const next=((old?.fx??0)+delta+STEP_FX.length)%STEP_FX.length;
  if(slot===0){if(tgt.second){tgt.root.effect=next;if(!next)tgt.list.length=0;}else{st.fx=next;if(!next)tgt.list.length=0;}}
  else if(old){if(next)old.fx=next;else tgt.list.splice(slot-1,1);}
  else if(next&&(tgt.second?tgt.root.effect:st.fx)){
   if(tgt.list.length!==slot-1){announce('ADD FX2 FIRST');return;}
   if(fxPoolCount(p)>=FX_EXTRA_LIMIT){fxFullOverlay();return;}
   tgt.list.push({fx:next,amount:50});
  }
 }else if(old){const val=clamp((old.amount??50)+delta*5,0,100);if(slot===0){if(tgt.second)tgt.root.value=val;else st.amount=val;}else old.amount=val;}
 mark();
}'''
assert s.count(old)==1;s=s.replace(old,new)
old="}else if(model.screen==='stepfx')model.fxParam=clamp(model.fxParam+dir,0,1);"
assert old in s;s=s.replace(old,"}else if(model.screen==='stepfx')model.fxParam=clamp(model.fxParam+dir,0,1);")
old="else if(model.screen==='stepfx'&&model.ch===5){model.fxDrumLane=(model.fxDrumLane+dir+2)%2;}"
assert s.count(old)==1;s=s.replace(old,"else if(model.screen==='stepfx'){if(model.ch===5)model.fxDrumLane=(model.fxDrumLane+dir+2)%2;else model.fxSlot=(model.fxSlot+dir+3)%3;}")
old="else if(action==='FX'){if(model.screen!=='stepfx'&&model.screen!=='punch')model.fxDrumLane=model.ch===5&&model.col===2?1:0;model.screen=model.screen==='stepfx'?'punch':model.screen==='punch'?'stepfx':'stepfx';}"
assert s.count(old)==1;s=s.replace(old,"else if(action==='FX'){if(model.screen!=='stepfx'&&model.screen!=='punch'){model.fxDrumLane=model.ch===5&&model.col===2?1:0;model.fxSlot=0;model.fxFull=false;}model.screen=model.screen==='stepfx'?'punch':model.screen==='punch'?'stepfx':'stepfx';}")
# insert direct action handling, before other actions inside action() after boot check
needle="if(model.screen==='instrument' &&!['EDIT','OCT+','OCT-'].includes(action))releaseEditVoices();"
assert s.count(needle)==1
inject='''if(model.screen==='stepfx'&&(action==='OCT+'||action==='OCT-')){
  if(action==='OCT+')model.fxSlot=(model.fxSlot+1)%3;
  else if(!model.playing)rememberEdit(()=>{const st=step(model.project,model.project.selectedPattern,model.ch,model.row,true),t=fxTarget(st);
    if(model.fxSlot===0){if(t.list.length){const v=t.list.shift();if(t.second){t.root.effect=v.fx;t.root.value=v.amount;}else{st.fx=v.fx;st.amount=v.amount;}}
      else if(t.second){t.root.effect=0;t.root.value=50;}else{st.fx=0;st.amount=50;}}
    else t.list.splice(model.fxSlot-1,1);mark();
  });
  model.fxFull=false;render();return;
 }
 '''
s=s.replace(needle,inject+needle)
# If directly clear selected steps delete extra FX
s=s.replace('st.note=null;st.inst=0;st.fx=0;st.amount=0;st.dyn=0;st.groove=0;st.chd=0;', 'st.note=null;st.inst=0;st.fx=0;st.extraFx=[];st.extraFx2=[];st.amount=0;st.dyn=0;st.groove=0;st.chd=0;')
# Keep popup visible until user interaction or a timer after 1.9s in render, announce timer
needle="function announce(msg){model.toast=msg;model.toastUntil=performance.now()+1900;$('#info').textContent=msg;render();}"
assert s.count(needle)==1
s=s.replace(needle, "function announce(msg){model.toast=msg;model.toastUntil=performance.now()+1900;$('#info').textContent=msg;render();if(msg==='FX STORAGE FULL')setTimeout(()=>{model.fxFull=false;render();},1900);}")
# UI help from firmware lines
import re
helpstr=(p/'firmware/fm1-native/beetle_help.h').read_text().split('static const char *const beetle_help_lines[] = {',1)[1].split('};',1)[0]
lines=re.findall(r'"([^"]*)"',helpstr)
import json
s,n=re.subn(r'const HELP_LINES=\[.*?\];',lambda m:'const HELP_LINES='+json.dumps(lines,ensure_ascii=False,separators=(',',':'))+';',s,count=1)
assert n==1
# native codec v21 read/write 48 pooled records
s=s.replace('if(schema<12||schema>20)', 'if(schema<12||schema>21)').replace('V12–V20','V12–V21')
needle='''   const events=read();if(events>64)'''
assert s.count(needle)==1
s=s.replace(needle,'''   if(schema>=21){
    const count=read();if(count>48)throw Error('Invalid native FX pool count.');
    if(!pat&&count)pat=emptyPattern();
    for(let e=0;e<count;e++){
      const key=read(),fx=read(),amount=read(),row=key%16,ch=(key&127)>>>4,second=!!(key&128);
      if(ch>=6||(second&&ch!==5)||!fx||fx>=12||amount>100)throw Error('Invalid native extra FX.');
      const st=pat.channels[ch][row],list=second?(st.extraFx2??(st.extraFx2=[])):(st.extraFx??(st.extraFx=[]));
      if(list.length>=2)throw Error('More than 3 FX on one step.');
      const root=second?pat.note2Fx?.[row]?.effect:st.fx;
      if(!root)throw Error('Orphan native extra FX.');
      list.push({fx,amount});
    }
   }
   const events=read();if(events>64)''')
needle="   const events=pat.punch||[];"
assert s.count(needle)==1
s=s.replace(needle,"""   const extra=[];for(let ch=0;ch<6;ch++)for(let row=0;row<16;row++){
    const st=pat.channels[ch][row];
    for(const [second,arr] of [[false,st.extraFx],[true,st.extraFx2]]){
      if(second&&ch!==5)continue;
      if((arr?.length||0)>2)throw Error('Too many FX per step.');
      if(arr?.length&&!(second?pat.note2Fx?.[row]?.effect:st.fx))throw Error('FX2/FX3 require FX1.');
      for(const fx of arr||[])extra.push({key:ch*16+row+(second?128:0),fx:limited(fx.fx,1,11,'extra FX'),amount:limited(fx.amount,0,100,'extra FX value')});
    }
   }
   if(extra.length>48)throw Error('FX STORAGE FULL');
   byte(extra.length);for(const x of extra){byte(x.key);byte(x.fx);byte(x.amount);}
   const events=pat.punch||[];""")
s=s.replace('write16(4,20);','write16(4,21);')
s=s.replace('Serialize the full editor model into the firmware V20 TRK1 wire format.','Serialize the full editor model into the firmware V21 TRK1 wire format.')
# project-wide 48 check before encode
s=s.replace('beetleVerifyProjectShape(p);\n  const out=new Uint8Array(12256)',"beetleVerifyProjectShape(p);if(fxPoolCount(p)>48)throw Error('FX STORAGE FULL');\n  const out=new Uint8Array(12256)")
# prevent excess 48 from fresh browser/project JSON exports
# Replays multi FX in WebAudio approximate via extra volume/other fx one voice (native is authoritative)
s=s.replace('for(let r=0;r<repeats;r++)for(const n of notes)this.voice(c,n,time+micro+r*gap,duration,inst,st.fx,st.amount,project,velocity);', '''for(let r=0;r<repeats;r++)for(const n of notes){
     const effects=[{fx:st.fx,amount:st.amount},...(st.extraFx||[])];
     const vol=effects.filter(x=>x.fx===10).reduce((n,x)=>n*x.amount/100,1);
     const fx=effects.find(x=>x.fx&&x.fx!==10)||{fx:st.fx,amount:st.amount};
     this.voice(c,n,time+micro+r*gap,duration,inst,fx.fx,fx.amount,project,Math.max(0,Math.round(velocity*vol)));
    }''')
f.write_text(s)
# update PM label only
f=p/'BEETLE-Project-Manager.html';s=f.read_text().replace('BEETLE V1.3.3','BEETLE V1.3.4');f.write_text(s)
print('patch_finish complete. help lines',len(lines))
