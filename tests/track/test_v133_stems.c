/* V1.3.3: native per-lane PCM taps are independent, before stereo PAN/master. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "track_chip.h"
#include "track_drum.h"
#include "track_mixer.h"
static struct track_chip_engine chip;
static struct track_drum_engine drums;
static struct track_mixer mixer;
static int16_t stereo[512*2],stems[512*6];
int main(void){
 track_chip_init(&chip);track_drum_init(&drums);track_mixer_init(&mixer);
 chip.env_attack[0]=chip.env_attack[1]=0;
 chip.env_sustain[0]=chip.env_sustain[1]=100;
 mixer.ch[0].pan=-100;mixer.ch[1].pan=100;
 assert(track_chip_note(&chip,0,0,60,100));
 assert(track_chip_note(&chip,1,1,65,100));
 beetle_stem_capture=stems;beetle_stem_offset=0;
 track_chip_render(&chip,&mixer,stereo,512u);
 int nonzero[6]={0};
 for(unsigned i=0;i<512;i++)for(unsigned j=0;j<6;j++)nonzero[j]+=(stems[i*6+j]!=0);
 assert(nonzero[0]>64&&nonzero[1]>64);
 for(unsigned j=2;j<6;j++)assert(nonzero[j]==0);
 /* With both lanes muted no stem may contain stale samples. */
 mixer.ch[0].mute=mixer.ch[1].mute=1;memset(stems,0,sizeof(stems));
 track_chip_render(&chip,&mixer,stereo,512u);
 for(unsigned i=0;i<512*6;i++)assert(!stems[i]);
 /* Independent CH6 PCM capture is available even without synth lanes. */
 memset(stems,0,sizeof(stems));mixer.ch[0].mute=mixer.ch[1].mute=1;
 assert(track_drum_trigger(&drums,0));
 memset(stereo,0,sizeof(stereo));track_drum_render(&drums,&mixer,stereo,512u);
 int drumnz=0;
 for(unsigned i=0;i<512;i++){drumnz+=(stems[i*6+5]!=0);for(unsigned j=0;j<5;j++)assert(stems[i*6+j]==0);}
 assert(drumnz>0);
 beetle_stem_capture=0;
 puts("PASS V1.3.3: isolated CH1,CH2,CH6 PCM, pan-independent stems, mute isolation");return 0;
}
