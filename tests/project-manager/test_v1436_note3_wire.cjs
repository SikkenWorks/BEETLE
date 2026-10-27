'use strict';
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),assert=require('node:assert/strict');
const root=path.resolve(__dirname,'../..'),src=fs.readFileSync(path.join(root,'simulator.html'),'utf8');
const a=src.indexOf('const BeetleNative=(()=>{'),b=src.indexOf('const BEETLE_BACKUP',a);assert(a>0&&b>a);
const codec=src.slice(a,b);
const shim=`
const clamp=(x,a,b)=>Math.min(b,Math.max(a,x));
const emptyStep=()=>({note:null,inst:0,chd:0,fx:0,amount:50,dyn:0,groove:0});
const emptyPattern=()=>({length:16,channels:Array.from({length:6},(_,c)=>Array.from({length:16},()=>({...emptyStep(),inst:c===5?3:c}))),punch:[],note3:Array(16).fill(null)});
const freshProject=name=>({version:1,name,bpm:120,selectedPattern:0,patterns:{},instruments:Array.from({length:8},()=>({})),mixer:Array.from({length:6},()=>({volume:.5,pan:0,mute:false,solo:false,swing:0})),song:[],songLoop:true,nativeSongEnabled:false,drumNote3Enabled:false,harmony:{root:0,mode:0,keylock:0},drumComponents:Array.from({length:14},()=>Array(9).fill(50)),master:{},typeBanks:Array.from({length:8},()=>({})),library:Array(8).fill(null),snapshots:[null,null,null,null],muteGroups:[0,0,0,0],punchParams:Array(16).fill(50)});
const songLanes=e=>Array.isArray(e.lanes)?e.lanes:Array(6).fill(e.pattern??0);
const beetleVerifyProjectShape=p=>true;
`;
const env=vm.createContext({Uint8Array,Array,String,Object,Number,Math,Error,console});vm.runInContext(shim+'\n'+codec,env);
const old=fs.readFileSync(path.join(root,'tests/track/fixtures/valid_v25_note2.beetle'));
env.raw=Uint8Array.from(old);vm.runInContext('var p=BeetleNative.decode(raw);',env);
assert.equal(vm.runInContext('p.nativeSchema',env),25);assert.equal(vm.runInContext('p.drumNote3Enabled',env),false);
vm.runInContext('p.drumNote3Enabled=true;p.patterns[0].note3[2]=26;p.patterns[0].note3[5]=254;',env);
vm.runInContext('var packed=BeetleNative.encode(p);var q=BeetleNative.decode(packed);',env);
assert.equal(vm.runInContext('q.nativeSchema',env),26);assert.equal(vm.runInContext('q.drumNote3Enabled',env),true);
assert.equal(vm.runInContext('q.patterns[0].note3[2]',env),26);assert.equal(vm.runInContext('q.patterns[0].note3[5]',env),254);
assert.equal(vm.runInContext('q.patterns[0].channels[5][1].note',env),3);
assert.equal(vm.runInContext('q.patterns[0].channels[5][1].chd',env),6);
vm.runInContext('q.drumNote3Enabled=false;var q2=BeetleNative.decode(BeetleNative.encode(q));',env);
assert.equal(vm.runInContext('q2.drumNote3Enabled',env),false);assert.equal(vm.runInContext('q2.patterns[0].note3[2]',env),26);
// Save native V26 file for independent C roundtrip verification.
const buf=vm.runInContext('packed',env);fs.writeFileSync(path.join(root,'tests/track/fixtures/valid_browser_v26_note3.beetle'),Buffer.from(buf));
console.log('PASS V1.4.36 WebSimulator TRK1 V25 decode -> V26 NOTE3 ON/OFF roundtrip, NOTE2 preserved, portable V26 fixture');
