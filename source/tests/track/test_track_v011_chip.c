/* TRACK V0.11 distinct chip/noise/punch DSP regression. GPL-3.0-only. */
#include "track_chip.h"
#include "track_mixer.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static struct track_chip_engine e;
static struct track_mixer mix;
static int16_t buf[8192*2];
static unsigned long long energy(const int16_t *a,unsigned n){unsigned long long x=0;for(unsigned i=0;i<n;i++){int32_t z=a[i];x+=(z<0?-z:z);}return x;}
static unsigned long long digest(uint8_t type){
 track_chip_init(&e);track_mixer_init(&mix);e.osc_type[0]=type;
 track_chip_begin_step(&e);assert(track_chip_note(&e,0,0,71,120));
 track_chip_render(&e,&mix,buf,8192);
 unsigned long long h=1469598103934665603ULL;
 for(unsigned i=1024;i<16384;i++){h^=(uint16_t)buf[i];h*=1099511628211ULL;}
 assert(energy(buf,16384)>100000);
 return h;
}
int main(void){
 unsigned long long hashes[10];
 for(unsigned i=0;i<10;i++){hashes[i]=digest((uint8_t)i);for(unsigned j=0;j<i;j++)assert(hashes[i]!=hashes[j]);}
 assert(hashes[0]!=hashes[1]);assert(hashes[3]!=hashes[7]);assert(hashes[7]!=hashes[9]);
 track_chip_init(&e);track_mixer_init(&mix);track_chip_begin_step(&e);track_chip_note(&e,0,0,66,127);
 track_chip_render(&e,&mix,buf,2048);
 int16_t dry[4096];memcpy(dry,buf,sizeof(dry));
 const uint8_t effects[]={1,2,4,8,16};
 for(unsigned k=0;k<5;k++){
  track_chip_init(&e);for(unsigned i=0;i<2048;i++){int v=(i%80<40?9000:-9000);buf[2*i]=v;buf[2*i+1]=v;}
  track_chip_process_punch(&e,effects[k],buf,2048);
  unsigned difference=0;for(unsigned i=1024;i<4096;i++)if(buf[i]!=((i/2)%80<40?9000:-9000))difference++;
  assert(difference>32);
  track_chip_process_punch(&e,0,buf,2048);
 }
 track_chip_init(&e);memcpy(buf,dry,sizeof(dry));track_chip_process_punch(&e,128,buf,2048);assert(energy(buf,4096)==0);
 printf("V0.11 PASS 10 distinct oscillator/noise spectra, 5 PCM punch modes and MUTE\n");
}
