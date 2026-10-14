const fs=require('fs'),assert=require('assert');
const f=fs.readFileSync(__dirname+'/../fixtures/legacy_web/TRACK-WebSimulator-V0.3.html','utf8');
const script=f.match(/<script>([\s\S]*?)<\/script>/)[1];new Function(script);
assert(f.includes("String(r).padStart(2,'0')"));
assert(f.includes('model.col=clamp(model.col+dir,1,4)'));
assert(f.includes('function nameTurn(') && f.includes('function finishName('));
assert(f.includes('TRACK_V03_SLOT_'));
assert(!f.includes('prompt(\'TRACK PROJECT NAME'));
const installer=fs.readFileSync(__dirname+'/../fixtures/legacy_web/TRACK-WebInstaller-V0.3.html','utf8');
assert(installer.includes('requestMIDIAccess'));
assert(installer.includes('INSTALLATION LOCKED'));
assert(!installer.includes('output.send(')); // no MIDI flashing in an unverified beta installer
console.log('TRACK V0.3: websim/installer static checks PASS');
