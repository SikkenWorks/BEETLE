/* Preview from BEETLE native C DSP, not a recording from FM1 hardware.
 * SPDX-License-Identifier: GPL-3.0-only */
#include "track_chip.h"
#include "track_mixer.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static struct track_chip_engine chip;
static void le16(FILE *f,uint16_t n){fputc(n&255,f);fputc((n>>8)&255,f);}
static void le32(FILE *f,uint32_t n){le16(f,(uint16_t)n);le16(f,(uint16_t)(n>>16));}
static int writewav(const char *file,uint8_t type){
 FILE *f=fopen(file,"wb");if(!f){perror(file);return 1;}
 const uint32_t frames=44100*2u,bytes=frames*4u;
 fwrite("RIFF",1,4,f);le32(f,bytes+36);fwrite("WAVEfmt ",1,8,f);
 le32(f,16);le16(f,1);le16(f,2);le32(f,44100);le32(f,176400);le16(f,4);le16(f,16);
 fwrite("data",1,4,f);le32(f,bytes);
 struct track_mixer mix;track_mixer_init(&mix);track_chip_init(&chip);
 chip.osc_type[0]=type;chip.env_attack[0]=1;chip.env_decay[0]=25;chip.env_sustain[0]=100;chip.env_release[0]=25;
 chip.cutoff[0]=96;chip.patches[0].wave_a=0;chip.patches[0].wave_b=1;
 chip.patches[0].mix=40;chip.patches[0].drive=30;chip.patches[0].morph=0;
 if(!track_chip_note(&chip,0,0,48,110))return 2;
 int16_t samples[256*2];
 for(uint32_t pos=0;pos<frames;pos+=256u){
  uint32_t n=frames-pos;if(n>256)n=256;
  if(pos>=70000u&&!chip.voices[0].released)track_chip_note_off(&chip,0,48);
  memset(samples,0,sizeof(samples));track_chip_render(&chip,&mix,samples,n);
  if(fwrite(samples,sizeof(int16_t),n*2u,f)!=(size_t)n*2u){fclose(f);return 3;}
 }
 fclose(f);return track_chip_health(&chip)?0:4;
}
int main(int argc,char **argv){if(argc!=3)return 2;
 return writewav(argv[1],TRACK_CHIP_SID_6581)||writewav(argv[2],TRACK_CHIP_SID_8580);
}
