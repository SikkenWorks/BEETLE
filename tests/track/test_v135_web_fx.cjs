const fs=require('fs'),vm=require('vm'),assert=require('assert');
const file=fs.readFileSync('web/BEETLE-FirmwareSimulator-V1.3.5.html','utf8');
function between(start,end){const i=file.indexOf(start);assert(i>=0,start);const j=file.indexOf(end,i+start.length);assert(j>=0,end);return file.slice(i,j+end.length);}
const core=between('const TRACK_CORE=(()=>{','\n})();');
const verify=between('function beetleVerifyProjectShape(p){','\n}\nfunction beetleDownload(').slice(0,-'\nfunction beetleDownload('.length);
const codec=between('const BeetleNative=(()=>{','\n})();');
const multi=between('const FX_EXTRA_LIMIT=48;','function fxFullOverlay(){').replace('function fxFullOverlay(){','');
const c=vm.createContext({TextEncoder,Uint8Array,console,Buffer});
const script=core+'\nconst {freshProject,parseProject,emptyPattern,DRUM_FACTORY,TYPES,songLanes}=TRACK_CORE;\n'+verify+'\n'+multi+'\n'+codec+'\n({freshProject,parseProject,emptyPattern,BeetleNative,fxPoolCount});';
const m=vm.runInContext(script,c);
const p=m.freshProject();p.patterns[0]=m.emptyPattern();
const st=p.patterns[0].channels[1][3];st.note=63;st.fx=0;st.extraFx=[null,{fx:2,amount:81}];
assert.equal(m.fxPoolCount(p),1);
const binary=m.BeetleNative.encode(p);assert.equal(binary[4],22);
const loaded=m.BeetleNative.decode(binary);
const actual=loaded.patterns[0].channels[1][3];
assert.equal(actual.fx,0);assert.equal(actual.extraFx[0],null);
assert.equal(actual.extraFx[1].fx,2);assert.equal(actual.extraFx[1].amount,81);
const data=m.BeetleNative.encode(loaded),round=m.BeetleNative.decode(data);
assert.equal(round.patterns[0].channels[1][3].extraFx[1].amount,81);
assert.equal(m.fxPoolCount(round),1);
// FX2 independently, without ever populating primary slot.
st.extraFx=[{fx:10,amount:65},null];
const round2=m.BeetleNative.decode(m.BeetleNative.encode(p));
assert.equal(round2.patterns[0].channels[1][3].extraFx[0].fx,10);
assert(!round2.patterns[0].channels[1][3].extraFx[1]);
// V19 must remain importable.
const legacy=Uint8Array.from(fs.readFileSync('tests/fixtures/BEETLE-V19-real-legacy-project.trk'));
assert.equal(m.BeetleNative.decode(legacy).instruments.length,8);
console.log('PASS V1.3.5 Web: FX3 before FX1/FX2, V22 export/import, FX2 alone, V19 import');

// Migration test using the actual previous V1.3.4/V21 browser codec.
const legacySource=fs.readFileSync('tests/fixtures/BEETLE-V1.3.4-Legacy-Simulator.html','utf8');
const legacyBetween=(start,end)=>{const i=legacySource.indexOf(start),j=legacySource.indexOf(end,i+start.length);assert(i>=0&&j>=0);return legacySource.slice(i,j+end.length);};
const legacyCore=legacyBetween('const TRACK_CORE=(()=>{','\n})();');
const legacyVerify=legacyBetween('function beetleVerifyProjectShape(p){','\n}\nfunction beetleDownload(').slice(0,-'\nfunction beetleDownload('.length);
const legacyCodec=legacyBetween('const BeetleNative=(()=>{','\n})();');
const legacyMulti=legacyBetween('const FX_EXTRA_LIMIT=48;','function fxFullOverlay(){').replace('function fxFullOverlay(){','');
const legacyCtx=vm.createContext({TextEncoder,Uint8Array,console,Buffer});
const oldCodec=vm.runInContext(legacyCore+'\nconst {freshProject,parseProject,emptyPattern,DRUM_FACTORY,TYPES,songLanes}=TRACK_CORE;\n'+legacyVerify+'\n'+legacyMulti+'\n'+legacyCodec+'\n({freshProject,emptyPattern,BeetleNative});',legacyCtx);
const oldProject=oldCodec.freshProject();oldProject.patterns[0]=oldCodec.emptyPattern();
const oldStep=oldProject.patterns[0].channels[0][1];oldStep.note=60;oldStep.fx=1;oldStep.extraFx=[{fx:2,amount:44},{fx:4,amount:66}];
const oldBinary=oldCodec.BeetleNative.encode(oldProject);assert.equal(oldBinary[4],21);
const upgraded=m.BeetleNative.decode(oldBinary);
const upgradedFx=upgraded.patterns[0].channels[0][1].extraFx;
assert.equal(upgradedFx[0].fx,2);assert.equal(upgradedFx[0].amount,44);assert.equal(upgradedFx[1].fx,4);assert.equal(upgradedFx[1].amount,66);
const reloaded=m.BeetleNative.decode(m.BeetleNative.encode(upgraded));
assert.equal(reloaded.patterns[0].channels[0][1].extraFx[1].amount,66);
console.log('PASS V1.3.5 migration: real V1.3.4 V21 -> V22 keeps FX2 + FX3 order and values');
