from pathlib import Path
root=Path(__file__).resolve().parents[2]
n=(root/'firmware/fm1-native/track_runtime.h').read_text()
s=(root.parent/'simulator.html').read_text()
i=(root.parent/'WebInstaller.html').read_text()
assert 'if(sel){int32_t v=(int32_t)track_master_focus+sel;' in n
assert 'if(pressed&(1u<<3)){track_master_cfg.enabled[fx]^=1u;' in n
assert 'if(pressed&(1u<<4)){track_master_cfg.enabled[fx]^=1u;' not in n
assert 'name[2]=\'[\';' in n and "name[4]=']'" in n
assert 'K1-4 EQ SEQ+K4 ORDER' not in n
assert 'SELECT FX SEL ON/OFF SEQ+K4' in n
assert 'track_load_from_boot?29u:1u' in n
assert 'if(model.screen===\'load\'&&model.loadFromBoot)' in s
assert "model.screen==='masterfx'" in s
assert 'RING MOD' in n and 'RING MOD' in s
assert 'REVERSE' not in n[n.index('static void track_fm1_master_screen'):]
assert '<details class="advanced-detail"' in i
assert 'id="freedom-enable"' in i and 'id="risk"' in i
print('PASS V0.84: master control, toggle brackets, boot HOME, RingMod, uncluttered installer')
