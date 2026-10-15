/* V0.5: bounded chip-style native renderer and mixer tests. GPL-3.0-only. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "track_chip.h"
#include "track_mixer.h"
static struct track_chip_engine synth;
static struct track_mixer mixer;
static int16_t pcm[2048*2];
static uint32_t energy(const int16_t *p,size_t samples){
 uint32_t n=0;for(size_t i=0;i<samples;i++){int32_t x=p[i];if(x<0)x=-x;n+=(uint32_t)x;}return n;
}
int main(void){
 track_chip_init(&synth);track_mixer_init(&mixer);
 assert(synth.voice_count==0u&&synth.duty[0]==2u);
 assert(!track_chip_note(&synth,4,0,60,100));
 assert(!track_chip_note(&synth,0,4,60,100));
 assert(!track_chip_note(&synth,0,0,128,100));
 for(uint8_t i=0;i<4;i++){
  track_chip_begin_step(&synth);
  assert(track_chip_note(&synth,i,i,60,100));
  track_chip_render(&synth,&mixer,pcm,2048);
  assert(energy(pcm,4096)>100000u);
  uint64_t peak=0;for(size_t j=0;j<4096;j++){
   int32_t n=pcm[j];if(n<0)n=-n;if((uint64_t)n>peak)peak=n;
  }assert(peak<=32767u);
 }
 track_chip_begin_step(&synth);
 for(int i=0;i<8;i++)assert(track_chip_note(&synth,i%4,i%4,(uint8_t)(60+i),127));
 assert(synth.voice_count==8u&&track_chip_note(&synth,0,0,64,127)); /* V0.14: steal rather than drop ninth voice */
 track_chip_render(&synth,&mixer,pcm,2048);
 assert(energy(pcm,4096)>0);
 track_chip_init(&synth); /* isolate the single-channel pan fixture */
 track_chip_begin_step(&synth);
 assert(track_chip_note(&synth,0,0,60,127));
 mixer.ch[0].volume=100;
 mixer.ch[0].pan=-100;
 track_chip_render(&synth,&mixer,pcm,2048);
 uint32_t l=0,r=0;for(size_t i=1024;i<2048;i++){
  int32_t a=pcm[i*2],b=pcm[i*2+1];l+=(uint32_t)(a<0?-a:a);r+=(uint32_t)(b<0?-b:b);
 }assert(l>100000u&&r==0u);
 mixer.ch[0].mute=1u;
 track_chip_silence(&synth);
 track_chip_begin_step(&synth);
 track_chip_note(&synth,0,1,60,127);
 track_chip_render(&synth,&mixer,pcm,2048);
 assert(energy(pcm+1024*2,2048)==0u);
 mixer.ch[0].mute=0;
 track_chip_init(&synth); /* isolated-patch comparison, not a sequencer step */
 track_chip_begin_step(&synth);
 track_chip_note(&synth,0,0,60,127);
 track_chip_render(&synth,&mixer,pcm,2048);
 int16_t out1[2048];memcpy(out1,pcm,sizeof(out1));
 track_chip_init(&synth);track_mixer_init(&mixer);mixer.ch[0].volume=100;mixer.ch[0].pan=-100;
 track_chip_begin_step(&synth);track_chip_note(&synth,0,0,60,127);
 track_chip_render(&synth,&mixer,pcm,2048);
 assert(memcmp(out1,pcm,sizeof(out1))==0); /* stable output */
 /* Step-local CUT must silence the voice after its sample threshold. */
 track_chip_init(&synth);track_mixer_init(&mixer);
 track_chip_begin_step(&synth);track_chip_note(&synth,0,0,60,127);
 synth.cut_after[0]=96;
 track_chip_render(&synth,&mixer,pcm,2048);
 assert(energy(pcm,192)>0u);
 assert(energy(pcm+2*400,2*512)==0u);
 /* Per-step DRIVE is audible and still saturates within signed PCM. */
 track_chip_init(&synth);track_chip_begin_step(&synth);
 track_chip_note(&synth,0,0,60,127);
 track_chip_render(&synth,&mixer,pcm,2048);
 uint32_t plain=energy(pcm,4096);
 track_chip_init(&synth);track_chip_begin_step(&synth);
 track_chip_note(&synth,0,0,60,127);
 synth.drive[0]=75u;
 track_chip_render(&synth,&mixer,pcm,2048);
 uint32_t driven=energy(pcm,4096);
 assert(plain!=driven);
 puts("TRACK V0.5 chip DSP: 4 oscillator types, 8-voice cap, pan, mute, deterministic output PASS");
 return 0;
}
