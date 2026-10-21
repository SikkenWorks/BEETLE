from pathlib import Path
from playwright.sync_api import sync_playwright
root=Path(__file__).resolve().parents[3]
with sync_playwright() as p:
 browser=p.chromium.launch(headless=True, executable_path='/usr/bin/chromium', args=['--no-sandbox'])
 page=browser.new_page(viewport={'width':1200,'height':900})
 errors=[]
 page.on('pageerror',lambda e:errors.append(str(e)))
 page.set_content((root/'simulator.html').read_text().replace('})(TRACK_CORE,TRACK_AUDIO);</script>', '''window.__testRC102=()=>{
model.screen='tracker';model.project=freshProject();
const pat=pattern(model.project,0,true);
pat.channels[0][0].note=60;pat.channels[0][0].chd=1;
pat.channels[1][1].note=66;pat.channels[5][0].note=62;
model.screen='guide';model.menuIndex=3;route('SEL');
const first=model.reverseResults;const exact=first.matches.find(r=>r.root===0&&r.mode===0);
rotate('k1',1);const choice=first.matches[model.reverseIndex];route('SEL');
return {screen:model.screen,events:first.events,exact:exact.outside,fit:exact.fit,selected:choice.root===model.project.harmony.root&&choice.mode===model.project.harmony.mode,currentScaleName:SCALES[model.project.harmony.mode][0]};
};
})(TRACK_CORE,TRACK_AUDIO);</script>'''))
 page.wait_for_timeout(1350)
 report=page.evaluate('window.__testRC102()')
 assert report['screen']=='guide',report
 assert report['events']==2,report
 assert report['exact']>0 and report['fit']<100,report
 assert report['selected'],report
 assert not errors,errors
 print('PASS RC1.02 WebSimulator: reverse search includes chords, excludes drum, SEL applies candidate')
 page.close();browser.close()
