/* BEETLE V0.53: regression comparing intrinsic 6581 / 8580 timbre at
 * IDENTICAL oscillator, ADSR, cutoff, resonance and mixer settings.
 * The test compares waveforms after matching amplitude (not just peaks).
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "track_chip.h"
#include "track_mixer.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define N 4096
static struct track_chip_engine chip;
static struct track_mixer mix;
static int16_t pcm[2][N*2];
static uint64_t energy(const int16_t *p){
 uint64_t total=0;
 for(unsigned i=256;i<N;i++){int32_t s=p[2*i];total+=(uint64_t)((int64_t)s*s);}
 return total;
}
static uint64_t normalized_error(const int16_t *a,const int16_t *b){
 /* Best-fit least-squares scalar uses integer Q15 to remove volume effects.
  * Returns normalized squared residual in ppm. */
 int64_t ab=0,bb=0,aa=0;
 for(unsigned i=256;i<N;i++){
  int64_t x=a[i*2],y=b[i*2];ab+=x*y;bb+=y*y;aa+=x*x;
 }
 assert(aa>0&&bb>0);
 int64_t k=(ab<<13)/bb;int64_t err=0;
 for(unsigned i=256;i<N;i++){
  int64_t r=(int64_t)a[2*i]-((int64_t)b[2*i]*k>>13);
  err+=r*r;
 }
 return (uint64_t)(err*1000000LL/aa);
}
int main(void){
 track_mixer_init(&mix);
 const uint8_t cutoffs[]={40u,96u,160u,220u};
 for(unsigned c=0;c<4;c++){
  for(unsigned m=0;m<2;m++){
   track_chip_init(&chip);
   chip.osc_type[0]=m?TRACK_CHIP_SID_8580:TRACK_CHIP_SID_6581;
   chip.cutoff[0]=cutoffs[c];
   chip.env_attack[0]=1;chip.env_decay[0]=25;chip.env_sustain[0]=100;
   chip.patches[0].wave_a=0;chip.patches[0].wave_b=1;
   chip.patches[0].mix=40u;chip.patches[0].drive=30u;
   chip.patches[0].morph=0u;
   assert(track_chip_note(&chip,0u,0u,48u,110u));
   track_chip_render(&chip,&mix,pcm[m],N);
   assert(track_chip_health(&chip));
   for(unsigned i=0;i<2*N;i++) assert(pcm[m][i]<32767 && pcm[m][i]>-32768);
   assert(energy(pcm[m])>20000000ULL);
  }
  const uint64_t difference=normalized_error(pcm[0],pcm[1]);
  printf("cutoff=%u 6581_power=%llu 8580_power=%llu gain_invariant_difference_ppm=%llu\n",
   cutoffs[c],(unsigned long long)energy(pcm[0]),(unsigned long long)energy(pcm[1]),(unsigned long long)difference);
  assert(difference>10000u); /* More than 1% of energy after gain matching. */
 }
 puts("PASS 6581/8580: different timbre independent of loudness, finite audio, 4 cutoffs");
}
