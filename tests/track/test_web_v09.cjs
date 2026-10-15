'use strict';
const fs=require('fs'),path=require('path'),assert=require('assert');
const root=path.resolve(__dirname,'../..');
const html=fs.readFileSync(path.join(root,'TRACK-Web-V0.9.html'),'utf8');
const runtime=fs.readFileSync(path.join(root,'firmware/fm1-native/track_runtime.h'),'utf8');
const scripts=[...html.matchAll(/<script>([\s\S]*?)<\/script>/g)].map(m=>m[1]);
assert.strictEqual(scripts.length,2);
scripts.forEach(s=>{new Function(s);});
assert(html.includes('TRACK V0.9'));
assert(html.includes('SIMULATOR')&&html.includes('WEBINSTALLER'));
assert(html.includes('function pixelText('));
assert(html.includes("CHORD_SHORT=['---','MAJ','MIN','DIM','AUG','SU2','SU4','MJ7','MN7','DO7','AD9','MN9']"));
assert(html.includes('const pc=NOTE_NAMES[midi%12]'));
assert(html.includes('if(midi==null)return \'---\''));
assert(html.includes('let r=first+i,y=43+i*10'));
assert(html.includes("48,216,SOFT,9"));
assert(html.includes("model.col=clamp(model.col+dir,1,4)"));
assert(html.includes('id="playIdle"')&&html.includes('id="playback"'));
assert(html.includes('function whiteDown(i)'));
assert(html.includes('function song()'));
assert(html.includes('function nameTurn('));
// The installer is embedded in the same user-facing HTML, isolated in srcdoc.
const base64=scripts[1].match(/const encoded='([A-Za-z0-9+/=]+)'/);
assert(base64,'missing inlined installer data');
const installer=Buffer.from(base64[1],'base64').toString('utf8');
assert(installer.includes('TRACK V0.9'));
assert(installer.includes('requestMIDIAccess'));
assert(installer.includes('<button disabled id="install"'));
assert(!installer.includes('output.send('));
assert(installer.includes("m.version==='0.9'"));
// V0.20 native LCD coordinates now mirror browser 240px grid exactly.
// CHD 3 chars fit within the bracket; all 16 rows end at or before y200.
assert(runtime.includes('const uint16_t xs[]={36u,77u,121u,163u,202u}'));
assert(runtime.includes('uint16_t y=(uint16_t)(43u+(uint16_t)i*10u)'));
assert(43+15*10+8<=201);
assert(runtime.includes('track_pix_text(hint,48u,216u,TRACK_BRIGHT)'));
assert(runtime.includes('lcd_fill(x,y-2u,(uint16_t)(widths[index]+4u),13u,0xF9BCu)'));
assert(runtime.includes('if(col>=TRACK_NAV_COLUMNS)col=0u'));
assert(!runtime.includes('xs[col+1u]')); // Regression: never read outside heading table
const codes=['---','MAJ','MIN','DIM','AUG','SU2','SU4','MJ7','MN7','DO7','AD9','MN9'];
assert(codes.every(c=>c.length===3));
assert(87+('SUSPENDED 4'.length*8)<240);
console.log('TRACK V0.9: single-page installer/simulator, 3-char CHD, LCD boundaries, native font and read-only installer PASS');
