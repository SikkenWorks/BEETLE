#!/usr/bin/env python3
from pathlib import Path
p=Path(__file__).resolve().parents[2]/'firmware/fm1-native/track_runtime.h'
s=p.read_text()
assert 'DAC READY' not in s and 'DAC WAIT' not in s
assert 'track_grid_lane_status();' in s
assert 'track_lane_has_data(&track_state.active,lane)' in s
assert '36u+(uint16_t)(lane&1u)*6u' in s
assert '210u+(uint16_t)(lane>>1u)*6u' in s
assert 'lcd_fill(x,y,5u,5u,' in s
assert 'lcd_fill(x,y-1u,cell_width+4u,9u,TRACK_PINK)' in s
assert 'lcd_fill(x+1u,y,cell_width+2u,7u,0x680Bu)' in s
assert 'widths[index]' not in s
# 240x240 screen, 10px row spacing: frame must not touch another row.
for row in range(16):
 y=43+10*row
 assert y-1>=0 and y+7<240
 if row<15:assert y+7<y+10-1
for lane in range(6):
 x=36+(lane&1)*6;y=210+(lane>>1)*6
 assert 0<=x and x+5<=240 and y+5<=240
print('PASS V0.48: cursor confines to its row; six correct 2x3 pixel blocks')
