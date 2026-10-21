#!/usr/bin/env python3
from pathlib import Path
fw=Path('firmware/fm1-native');html=Path('web/simulator.html').read_text()
s=(fw/'track_runtime.h').read_text()
assert 'track_drum_note1_serial, track_drum_note2_serial' in s
assert 'if(dr->note==TRACK_NOTE_STOP && track_drum_note1_serial)' in s
assert 'if(dr->chord==15u && track_drum_note2_serial)' in s
assert 'else if(dr->note==TRACK_NOTE_STOP){track_drum_stop(&track_drum);}' not in s
assert 'track_drum_trigger_pair_tuned(&track_drum,first,second' in s
assert 'st.note===254&&this.lastNote1Voice' in html
assert 'st.chd===15&&this.lastNote2Voice' in html
assert 'st.note=254;if(model.ch!==5)st.chd=0;' in html
assert "if(model.menuIndex===i)rect(24,y-2,194,16,'#480038')" in html
assert 'NO NES_SND_EMU CODE USED' not in s and 'NO NES_SND_EMU CODE USED' not in html
print('PASS 1.01 independent note stops, playback, menu frames, credits')
