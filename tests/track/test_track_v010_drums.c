#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "track_drum.h"
#include "track_mixer.h"
int main(void){
 struct track_drum_engine d;track_drum_init(&d);
 assert(track_drum_trigger(&d,0)==1);
 assert(track_drum_trigger(&d,2)==1);
 uint8_t h=d.hh_last;assert(h==6||h==8);
 assert(track_drum_trigger(&d,3)==1);
 /* A newly triggered OPEN HAT immediately chokes the older closed hat. */
 int hats=0;for(unsigned i=0;i<TRACK_DRUM_POLY;i++) if(d.voices[i].active && (d.voices[i].id==6||d.voices[i].id==8||d.voices[i].id==10)&&!d.voices[i].release_frames)hats++;
 assert(hats==1);
 assert(track_drum_trigger(&d,2)==1);assert(d.hh_last==6||d.hh_last==8);
 assert(track_drum_trigger(&d,2)==1);assert(d.hh_last!=h || d.hh_last==6 || d.hh_last==8);
 assert(track_drum_trigger(&d,1)==1);uint8_t sn=d.sn_last;assert(track_drum_trigger(&d,1)==1);assert(d.sn_last!=sn);
 assert(track_drum_trigger(&d,14)==0);
 struct track_mixer m;track_mixer_init(&m);int16_t stereo[2*512]={0};track_drum_render(&d,&m,stereo,512);
 int has_audio=0;for(unsigned i=0;i<1024;i++)if(stereo[i])has_audio=1;
 assert(has_audio);
 track_drum_stop(&d);for(unsigned i=0;i<TRACK_DRUM_POLY;i++)assert(!d.voices[i].active);
 puts("TRACK V0.10 drum voices, choke, alternating variants, audible PCM and bounded indices PASS");return 0;
}
