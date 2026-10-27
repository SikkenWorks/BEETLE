'use strict';
const assert=require('node:assert/strict');
const fs=require('node:fs');
const vm=require('node:vm');
const {TextEncoder,TextDecoder}=require('node:util');
const path=require('node:path');
const src=fs.readFileSync(path.join(__dirname,'../../BEETLE-Project-Manager.html'),'utf8');
function grab(a,b){const i=src.indexOf(a);assert(i>=0,`missing ${a}`);const j=src.indexOf(b,i);assert(j>i,`missing bound ${b}`);return src.slice(i,j);}
const pure=[
 grab('function crc32(', 'function frame('),
 grab('function makeBackupZip(', '/* BEETLE V1.4.36:'),
 grab("const BULK_FORMAT='BEETLE-PROJECTS-BULK-1';", 'async function exportProjectsZip('),
].join('\n');
const test=`
const MAX_PROJECT=12256;
const assert=globalAssert;
const en=new TextEncoder();
const data0=Uint8Array.from([1,2,3,4,5]);
const data1=Uint8Array.from([9,8,7,6,5,4]);
const members=[
 {slot:0,name:'TEST',file:'TEST-SLOT-01.beetle',bytes:data0},
 {slot:11,name:'END',file:'END-SLOT-12.beetle',bytes:data1},
];
const man={format:BULK_FORMAT,created:'2026-10-25',projects:members.map(e=>({slot:e.slot,file:e.file,name:e.name,version:25,rawSize:100,rawCrc:200,encryptedCrc:crc32(e.bytes)}))};
const files=[{name:'PROJECTS-MANIFEST.json',bytes:en.encode(JSON.stringify(man))}, ...members.map(e=>({name:e.file,bytes:e.bytes}))];
const zip=makeBackupZip(files);
const parsed=parseBulkProjectsZip(zip);
assert.equal(parsed.projects.length,2);
assert.equal(parsed.projects[1].slot,11);
assert.deepEqual(Array.from(parsed.files.get(members[0].file)),[1,2,3,4,5]);
function rejected(v,why){assert.throws(()=>parseBulkProjectsZip(v),undefined,why);}
let bad=zip.slice();bad[bad.indexOf(1,40)]=0;rejected(bad,'corrupted member CRC');
bad=zip.slice(0,zip.length-1);rejected(bad,'truncated EOCD');
bad=zip.slice();bad[bad.length-22+16]^=1;rejected(bad,'invalid central directory offset');
bad=zip.slice();bad[bad.length-22+8]=4;rejected(bad,'wrong entry count');
const dupe={...man,projects:[{...man.projects[0]}, {...man.projects[0],file:members[1].file, encryptedCrc:crc32(data1)}]};
rejected(makeBackupZip([{name:'PROJECTS-MANIFEST.json',bytes:en.encode(JSON.stringify(dupe))}, ...files.slice(1)]),'duplicated slot');
const unsafe=[{name:'PROJECTS-MANIFEST.json',bytes:files[0].bytes},{name:'../p.beetle',bytes:data0},{name:members[1].file,bytes:data1}];
rejected(makeBackupZip(unsafe),'path traversal');
const compressed=zip.slice();compressed[8]=8;rejected(compressed,'unexpected compression');
console.log('PASS bulk ZIP stored-mode roundtrip, original slots, CRC, EOCD, central directory, duplicates, unsafe names, compression');
`;
vm.runInNewContext(pure+'\n'+test,{TextEncoder,TextDecoder,globalAssert:assert,console},{timeout:10000});
