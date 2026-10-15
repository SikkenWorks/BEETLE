"""V0.55 six-lane WebSimulator layout, navigation and K3 field parity.
Requires Playwright + /usr/bin/chromium. Browser-side state stays in-memory.
"""
from pathlib import Path
from playwright.sync_api import sync_playwright

base = Path(__file__).resolve().parents[2]
file = base / 'web/BEETLE-V0.55-WebSimulator.html'
s = file.read_text()
s = s.replace('const audio=new TrackAudio();', '''window.__v055 = {
  state:()=>({screen:model.screen,row:model.mixerRow,col:model.mixerCol,menu:model.menuIndex,
    six:model.project.mixer.length,vol:model.project.mixer[model.mixerRow].volume,
    mute:model.project.mixer[model.mixerRow].mute,solo:model.project.mixer[model.mixerRow].solo}),
  screen:(name)=>{model.screen=name;render();},
  setup:(name)=>{model.screen=name;model.menuIndex=11;model.songRow=7;
    model.project.song=Array.from({length:8},(_,i)=>({pattern:i,repeats:1}));render();}
};const audio=new TrackAudio();''')
assert 'window.__v055' in s
with sync_playwright() as pw:
  browser=pw.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-dev-shm-usage'])
  page=browser.new_page(viewport={'width':1240,'height':900}, device_scale_factor=1)
  errors=[]
  page.on('pageerror',lambda ex:errors.append(str(ex)))
  page.set_content(s,wait_until='load',timeout=60000)
  page.wait_for_timeout(1400)
  assert page.locator('[data-action="ARP"]').is_enabled()
  page.locator('[data-action="ARP"]').click()
  assert page.evaluate('window.__v055.state().screen') == 'song'
  page.locator('[data-action="GLO"]').click()
  assert page.evaluate('window.__v055.state().screen') == 'mixer'
  assert page.evaluate('window.__v055.state().six') == 6
  assert page.evaluate('window.__v055.state().row') == 0
  old_vol=page.evaluate('window.__v055.state().vol')
  page.locator('[data-turn="k3:1"]').click()
  assert page.evaluate('window.__v055.state().vol')>old_vol,'K3 must control selected VOL cell'
  for _ in range(5):page.locator('[data-turn="k1:1"]').click()
  assert page.evaluate('window.__v055.state().row') == 5, 'K1 must reach lane 6'
  page.locator('[data-turn="k2:1"]').click()
  page.locator('[data-turn="k2:1"]').click()
  page.locator('[data-turn="k3:1"]').click()
  assert page.evaluate('window.__v055.state().mute') is True, 'K3 must edit selected MUTE'
  page.locator('[data-action="GLO"]').click()
  assert page.evaluate('window.__v055.state().screen') == 'tracker', 'Second GLO closes Mixer, never opens Song'
  page.locator('[data-action="ARP"]').click()
  assert page.evaluate('window.__v055.state().screen') == 'song'
  page.locator('[data-action="SEQ"]').click()
  assert page.evaluate('window.__v055.state().screen') == 'tracker'

  # Fail if any raster primitive paints outside the 240x240 device display.
  page.evaluate('''() => {
    window.__rasterOverflow=[];
    const c=document.getElementById('screen').getContext('2d');
    const old=c.fillRect.bind(c);
    c.fillRect=(x,y,w,h)=>{if(x<0||y<0||x+w>240||y+h>240)window.__rasterOverflow.push([x,y,w,h]);old(x,y,w,h);};
  }''')
  shots=Path('/mnt/data/beetle_v055_work/screens')
  shots.mkdir(parents=True,exist_ok=True)
  for screen in ['tracker','mixer','instrument','masterfx','projects','help','stepfx','punch','punchedit','saveas','load','new','name','overwrite','patterns','song','guide','env','lfo']:
    page.evaluate('(v) => window.__v055.setup(v)',screen)
    page.locator('#screen').screenshot(path=str(shots/(screen+'.png')))
    assert not page.evaluate('window.__rasterOverflow'),f'{screen} draws outside screen'
  assert not errors,errors
  print('PASS V0.55: ARP/GLO/SEQ, six Mixer lanes and K3 field edits, 19 pages fit within 240x240; no JS exceptions')
  browser.close()
