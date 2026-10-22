const fs=require('fs'),assert=require('assert');
const s=fs.readFileSync(require('path').resolve(__dirname,'../../BEETLE-Project-Manager.html'),'utf8');
const section=s.slice(s.indexOf('async function verifiedConvertedBackup('),s.indexOf("$('connect').onclick=",s.indexOf('async function verifiedConvertedBackup(')));
let prompt=[],uploads=[],reads=[],downloads=[],logMessages=[];
let deviceSchema=24,selected=0,connected=true;
let slots=[{state:'empty',name:''}];
const bytes=new Uint8Array([1,2,3]);
let file={name:'OLD.beetle',slice:()=>({arrayBuffer:async()=>Uint8Array.from([66,69,69,84,76,69,48,49,2]).buffer})};
async function decryptProject(){return bytes;}
function validateProject(raw){if(raw===bytes)return {version:22,slot:0,name:'OLD'};return {version:24,slot:0,name:'OLD'};}
const BeetleFileV2={signature:a=>a[8]};
function setProgress(){};function hideProgress(){};
async function modal(title,msg,btns){prompt.push(title);return btns.find(x=>x.value==='go')?.value||'cancel';}
async function uploadProject(slot,raw,overwrite){uploads.push({slot,raw,overwrite});slots[slot]={state:'used',name:'OLD',version:24};}
async function readProjectFromSlot(slot){reads.push(slot);return {raw:new Uint8Array([7,8]),info:{version:24,slot,name:'OLD'}};}
async function encryptProject(){return 'ENCRYPTED';}
function download(blob,name){downloads.push({blob,name});}
function filename(name,slot){return name+'-'+slot+'.beetle';}
function setBusy(){}function draw(){}function log(m){logMessages.push(m);}
function $(id){return {}};
eval(section);
(async()=>{
 await importWithPrompt(file);
 assert.strictEqual(uploads.length,1);
 assert.strictEqual(reads.length,1);
 assert(prompt[0].startsWith('LEGACY PROJECT DETECTED'),prompt);
 assert(downloads[0].name.includes('V24'));
 assert(logMessages.at(-1).includes('Converted legacy'));
 console.log('PASS import old V22 -> V24 with post-commit read-back and encrypted new backup');
})();
