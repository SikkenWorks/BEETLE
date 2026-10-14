const fs=require('fs'),assert=require('assert');
const root=__dirname+'/../fixtures/legacy_web/';
const simulator=fs.readFileSync(root+'TRACK-WebSimulator-V0.4.html','utf8');
const js=simulator.match(/<script>([\s\S]*?)<\/script>/)[1];new Function(js);
assert(simulator.includes('V0.4'));
assert(simulator.includes('LONGSEQ')&&simulator.includes('1200'));
assert(simulator.includes('model.col=clamp(model.col+dir,1,4)'));
assert(simulator.includes('model.project.song.splice(model.songRow+1'));
assert(simulator.includes('TRACK_V03_SLOT_')); // keeps V0.3 browser projects accessible
assert(simulator.includes('function nameTurn('));
const inst=fs.readFileSync(root+'TRACK-WebInstaller-V0.4.html','utf8');
const iscript=inst.match(/<script>([\s\S]*?)<\/script>/)[1];new Function(iscript);
assert(inst.includes('MANIFEST MATCH')&&inst.includes('m.version===\'0.4\''));
assert(inst.includes('requestMIDIAccess'));
assert(inst.includes('INSTALLATION LOCKED'));
assert(inst.includes('id="install"'));
assert(/<button disabled id="install"/.test(inst));
assert(!inst.includes('output.send(')&&!inst.includes('.send('));
console.log('TRACK V0.4: websim, song controls, manifest integrity and installer safety PASS');
