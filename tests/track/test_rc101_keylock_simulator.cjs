/* Verify HTML simulator's actual TRACK_CORE rather than a reimplementation. */
const fs=require('fs'),vm=require('vm'),assert=require('assert'),path=require('path');
const html=fs.readFileSync(path.resolve(__dirname,'../../web/simulator.html'),'utf8');
const a=html.indexOf('const TRACK_CORE=(()=>{');
const b=html.indexOf('})();',a);
assert(a>=0&&b>a);
const context={module:{exports:{}}};
vm.runInNewContext(html.slice(a,b+5)+'\nmodule.exports=TRACK_CORE;',context,{timeout:5000});
const core=context.module.exports;
const allowed=(n,h)=>core.SCALES[h.mode][1].includes(((n%12-h.root)%12+12)%12);
let count=0;
for(let root=0;root<12;root++)for(let mode=0;mode<9;mode++){
 const h={root,mode,keylock:2};
 const sequence=Array.from({length:128},(_,i)=>i).filter(n=>allowed(n,h)).concat([null,254]);
 for(let i=0;i<sequence.length;i++)for(let d=-4;d<=4;d++){
  const st={note:sequence[i],chd:0};core.stepChange(st,1,d,false,h);
  const idx=((i+d)%sequence.length+sequence.length)%sequence.length;
  assert.strictEqual(st.note,sequence[idx]);count++;
 }
 for(let note=0;note<128;note++)for(let type=0;type<12;type++){
  const tones=core.chordPitches(note,type,h);
  for(const n of tones)assert(allowed(n,h),`wrong scale: ${root} ${mode} ${note} ${type} ${n}`);
  count++;
 }
}
const c={root:0,mode:0,keylock:2};
assert.deepEqual(Array.from(core.chordPitches(62,1,c)),[62,65,69]);
const st={note:60,chd:1};core.stepChange(st,1,1,false,c);assert.strictEqual(st.note,62);
core.stepChange(st,1,-1,false,c);assert.strictEqual(st.note,60);
for(const keylock of [0,1]){
 const st={note:60};core.stepChange(st,1,1,false,{...c,keylock});assert.strictEqual(st.note,61);
 assert.deepEqual(Array.from(core.chordPitches(62,1,{...c,keylock})),[62,66,69]);
}
const drum={note:3,chd:0};core.stepChange(drum,1,1,true,c);assert.strictEqual(drum.note,4);
console.log('Simulator KEYLOCK scan and chords: PASS',count,'cases');
