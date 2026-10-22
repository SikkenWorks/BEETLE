'use strict';
const fs=require('fs'),vm=require('vm'),assert=require('assert'),path=require('path');
const root=path.resolve(__dirname,'../..');const html=fs.readFileSync(path.join(root,'BEETLE-Hub-Diagnostics.html'),'utf8');
const script=html.match(/<script>([\s\S]*?)<\/script>/)[1];
assert(!script.includes('0x57')&&!script.includes('0x5c')&&!script.includes('0x7f'));
function pack7(p){let a=0,b=0,o=[];for(const x of p){a|=x<<b;b+=8;while(b>=7){o.push(a&127);a>>>=7;b-=7;}}if(b)o.push(a&127);return o;}
function unpack7(p){let a=0,b=0,o=[];for(const x of p){a|=x<<b;b+=7;if(b>=8){o.push(a&255);a>>>=8;b-=8;}}return o;}
function envelope(cmd,body){let sum=body.reduce((a,x)=>(a+x)&255,0);let h=[0,0x59,cmd,body.length&255,0,0,...body,(~sum)&255];return [0xf0,...pack7(h),0xf7];}
class FakeElement{constructor(){this.value='';this.disabled=false;this.options=[];this.textContent='';this.onclick=null;}replaceChildren(...a){this.options=[...a];}add(a){this.options.push(a);}}
const ids={};for(const key of ['connect','input','output','probe','copy','state','report'])ids[key]=new FakeElement();
const input={name:'BEETLE Audio MIDI In',manufacturer:'SikkenWorks',listeners:new Set(),open:async()=>{},addEventListener(_kind,fn){this.listeners.add(fn);},removeEventListener(_kind,fn){this.listeners.delete(fn);},emit(data){for(const fn of [...this.listeners])fn({data:Uint8Array.from(data)});}};
const sent=[];const output={name:'BEETLE Audio MIDI Out',manufacturer:'SikkenWorks',open:async()=>{},send(data){const b=unpack7(data.slice(1,-1));assert.equal(b[1],0x59);assert([0x11,0x50].includes(b[2]),'write command attempted');sent.push(b[2]);let response;
 if(b[2]===0x11){const text=Buffer.from('FM-1_2123');response=envelope(0x11,[...text,...new Array(27-text.length).fill(0)]);}
 if(b[2]===0x50)response=envelope(0x60,[37,4,12,24,1]);
 setTimeout(()=>input.emit(response),5);
}};
const doc={getElementById:x=>ids[x],addEventListener:()=>{}};
const context={document:doc,Option:function(text,value){this.text=text;this.value=value;},navigator:{requestMIDIAccess:async()=>({inputs:new Map([[1,input]]),outputs:new Map([[1,output]])})},console,Uint8Array,Date,Number,Math,Array,Error,Promise,setTimeout,clearTimeout};
vm.runInNewContext(script,context,{filename:'BEETLE-Hub-Diagnostics.html'});
(async()=>{
 await ids.connect.onclick();assert.equal(ids.probe.disabled,false);assert.equal(ids.input.value,'0');assert.equal(ids.output.value,'0');
 await ids.probe.onclick();assert.deepStrictEqual(sent,[0x11,0x50]);assert(ids.report.textContent.includes('FM-1_2123'));assert(ids.report.textContent.includes('CONFIRMED (0x60)'));assert(ids.report.textContent.includes('Flash writes: NONE'));
 console.log('PASS V1.4.13 read-only Hub probe: identity, app HELLO, report and no write commands');
})().catch(e=>{console.error(e.stack);process.exitCode=1;});
