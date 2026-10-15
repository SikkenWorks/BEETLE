#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "track_drum.h"
#include "track_mixer.h"
static uint8_t last_id(const struct track_drum_engine *e){
 for(unsigned i=0;i<TRACK_DRUM_POLY;i++)if(e->voices[i].active)return e->voices[i].id;
 return 255u;
}
int main(void){
 static const char *expected[]={"KIK","SNR","HHC","HHO","TM1","TM2","TM3","CRS","RDE","CLP","PC1","PC2","PC3","RIM"};
 static const uint8_t sample[]={0,1,6,10,14,12,11,13,15,3,5,7,9,4};
 for(unsigned k=0;k<14u;k++){
  struct track_drum_engine d;track_drum_init(&d);
  assert(strcmp(track_drum_name((uint8_t)k),expected[k])==0);
  assert(track_drum_trigger(&d,(uint8_t)k));
  uint8_t phys=last_id(&d);
  if(k==1)assert(phys==1u||phys==2u);
  else if(k==2)assert(phys==6u||phys==8u);
  else assert(phys==sample[k]);
 }
 struct track_drum_engine d;track_drum_init(&d);
 assert(track_drum_trigger(&d,1));uint8_t s=d.sn_last;
 assert(track_drum_trigger(&d,1));assert(d.sn_last!=s);
 assert(track_drum_trigger(&d,2));uint8_t h=d.hh_last;
 assert(track_drum_trigger(&d,2));assert(d.hh_last!=h);
 assert(track_drum_trigger(&d,3));
 unsigned hats=0;for(unsigned i=0;i<TRACK_DRUM_POLY;i++)if(d.voices[i].active&&(d.voices[i].id==6||d.voices[i].id==8||d.voices[i].id==10)&&!d.voices[i].release_frames)hats++;
 assert(hats==1);
 assert(!track_drum_trigger(&d,14));
 puts("TRACK V0.11: exact 14 ordered instruments, sample remapping, SN/HH round-robin, hat choke PASS");
 return 0;
}
