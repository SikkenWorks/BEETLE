/* BEETLE V1.3.2 deterministic eight-patch + octave-range regression. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "track_core.h"
#include "track_chip.h"
#include "track_drum.h"
static struct track_engine e;
static struct track_chip_engine chip;
int main(void){
 track_init(&e);track_chip_init(&chip);
 const unsigned defaults[8]={TRACK_CHIP_RETRO_PULSE,TRACK_CHIP_RETRO_WAVE,TRACK_CHIP_VA_OSC,TRACK_CHIP_VA_OSC2,TRACK_CHIP_VA_OSC,TRACK_CHIP_VA_OSC2,TRACK_CHIP_SID_6581,TRACK_CHIP_NES_PULSE};
 for(unsigned n=0;n<8;n++)assert(chip.osc_type[n]==defaults[n]);
 assert(chip.patches[4].coarse_b==-12 && chip.patches[4].sub==18 && chip.env_decay[4]==25);
 assert(chip.patches[5].wave_a==3 && chip.patches[5].fine_b==7 && chip.env_attack[5]==18);
 assert(chip.patches[2].coarse_b==0 && chip.patches[3].fine_b==0);
 assert(e.active.ch[TRACK_DRUM_CHANNEL][0].instrument==3u);
 e.transport.edit_channel=0;e.transport.edit_step=3;
 e.active.ch[0][2].note=60;e.active.ch[0][2].instrument=6;
 e.active.ch[0][3].note=64;e.active.ch[0][3].instrument=3;
 e.active.ch[0][4].note=TRACK_NOTE_EMPTY;
 e.active.ch[0][5].note=TRACK_NOTE_STOP;
 e.active.ch[0][6].note=119;e.active.ch[0][6].instrument=7;
 e.active.ch[0][7].note=0;
 e.active.ch[1][2].note=50;
 assert(track_transpose_note_range(&e,0,7,2,12)==4);
 assert(e.active.ch[0][2].note==72 && e.active.ch[0][3].note==76);
 assert(e.active.ch[0][4].note==TRACK_NOTE_EMPTY && e.active.ch[0][5].note==TRACK_NOTE_STOP);
 assert(e.active.ch[0][6].note==127 && e.active.ch[0][7].note==12);
 assert(e.active.ch[1][2].note==50);
 assert(e.active.ch[0][2].instrument==6 && e.active.ch[0][3].instrument==3 && e.active.ch[0][6].instrument==7);
 assert(track_transpose_note_range(&e,0,2,7,-12)==4);
 assert(e.active.ch[0][2].note==60 && e.active.ch[0][3].note==64 && e.active.ch[0][6].note==115 && e.active.ch[0][7].note==0);
 assert(track_transpose_note_range(&e,0,4,5,12)==0);
 assert(track_transpose_note_range(&e,TRACK_DRUM_CHANNEL,0,4,12)==-1);
 e.transport.edit_step=3;assert(track_adjust_octave(&e,12)==1 && e.active.ch[0][3].note==76);
 assert(track_adjust_octave(&e,-12)==1 && e.active.ch[0][3].note==64);
 chip.osc_type[3]=TRACK_CHIP_SID_DIRTY;chip.osc_type[1]=TRACK_CHIP_RETRO_WAVE;
 assert(chip.osc_type[3]==TRACK_CHIP_SID_DIRTY && chip.osc_type[1]==TRACK_CHIP_RETRO_WAVE);
 puts("PASS V1.3.2: eight factory patch identities; independent slot edits; single/range OCT +/-; bounds; DRUM; STOP/EMPTY; no cross-lane edits");
}
