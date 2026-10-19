/* Test fixture usable with original RC1.0.11 and RC1.0.11b. Prints checksum
 * of deterministic stereo render sequences. Must be identical at no overload. */
#include <stdio.h>
#include <stdint.h>
#include "track_chip.h"
#include "track_mixer.h"
static struct track_chip_engine chip;
static struct track_mixer mix;
static int16_t stereo[2*256];
static uint32_t crc=2166136261u;
int main(void){
 track_chip_init(&chip);track_mixer_init(&mix);
 for(int i=0;i<4;i++){
  chip.osc_type[i]=(uint8_t)i;
  for(int j=0;j<2;j++)track_chip_note(&chip,(uint8_t)i,(uint8_t)i,(uint8_t)(48+i*4+j),100);
 }
 for(int blk=0;blk<40;blk++){
  if(blk==8)mix.ch[1].pan=-70;
  if(blk==12)mix.ch[3].volume=55;
  if(blk==16)mix.ch[2].solo=1;
  if(blk==22)mix.ch[2].solo=0;
  if(blk==25)mix.ch[0].mute=1;
  if(blk==30)mix.ch[0].mute=0;
  track_chip_render(&chip,&mix,stereo,256);
  for(int k=0;k<512;k++){
   uint16_t v=(uint16_t)stereo[k];
   crc^=(uint8_t)v;crc*=16777619u;
   crc^=(uint8_t)(v>>8);crc*=16777619u;
  }
 }
 printf("PCM FNV1A32 %08x\n",crc);
 return 0;
}
