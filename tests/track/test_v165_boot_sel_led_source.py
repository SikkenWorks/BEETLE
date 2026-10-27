from pathlib import Path
s=Path(__file__).resolve().parents[2].joinpath("firmware/fm1-native/track_runtime.h").read_text()
assert 'beetle_start_led_only_sel(track_page,etch_ms,track_boot_splash_until)' in s
assert 'fm1_led_key(BEETLE_START_SEL_LED_ID,1)' in s
block=s.rsplit('if(track_page==29u){',1)[1].split('if(track_page==13u){',1)[0]
assert 'track_fm1_leds();' in block, 'Boot chooser exits before the normal LED refresh'
print('PASS: startup selector updates LED matrix despite early boot-menu return')
