/* SPDX-License-Identifier: GPL-3.0-only
 * V1.4.26 host regression, no physical FM1 timing claims. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "track_core.h"
#include "track_drum.h"
#include "track_chip.h"
#define N 12000u
static int16_t source[N*2],wave[N*2];
static void fill(void){
 uint32_t q=0x239A512Du;
 for(unsigned i=0;i<N;i++){
  q^=q<<13;q^=q>>17;q^=q<<5;
  int32_t s=((int32_t)(q&65535u)-32768)/6+(int32_t)((i*37u)%1024u)-512;
  source[2u*i]=source[2u*i+1u]=(int16_t)s;
 }
}
static uint64_t changed(void){uint64_t acc=0;
 for(unsigned i=512;i<N*2;i++){int32_t d=(int32_t)wave[i]-source[i];acc+=(uint32_t)(d<0?-d:d);}return acc;
}
static void run(uint16_t bit,uint8_t param){
 struct track_chip_engine chip;track_chip_init(&chip);
 chip.punch_param[param]=50u;
 memcpy(wave,source,sizeof wave);
 track_chip_process_punch(&chip,bit,wave,N);
 assert(changed()>100000u);
}
int main(void){
 struct track_engine e;track_init(&e);e.transport.edit_channel=TRACK_DRUM_CHANNEL;
 e.transport.edit_column=1u;e.transport.edit_step=3u;
 struct track_step *s=&e.active.ch[TRACK_DRUM_CHANNEL][3];
 s->note=8u;s->instrument=2u;s->effect=3u;s->value=88u;
 for(unsigned n=0;n<27u;n++){
  assert(track_enter_drum_note2(&e,(uint8_t)n)==0);
  assert(track_note2_id(s)==n);
  assert(s->note==8u && (s->instrument&3u)==2u&&s->effect==3u&&s->value==88u);
  uint8_t encoded[TRACK_PATTERN_BYTES];struct track_pattern copy;
  int len=track_pattern_encode(&e.active,encoded,sizeof encoded);
  assert(len==(int)TRACK_PATTERN_BYTES);
  assert(track_pattern_decode(&copy,encoded,(uint32_t)len)==0);
  assert(track_note2_id(&copy.ch[TRACK_DRUM_CHANNEL][3])==n);
 }
 track_note2_set(s,TRACK_NOTE_STOP);assert(s->chord==15u&&track_note2_id(s)==TRACK_NOTE_EMPTY);
 track_note2_set(s,TRACK_NOTE_EMPTY);assert(!s->chord&&!(s->instrument&4u));
 fill();run((uint16_t)(1u<<9),9);run((uint16_t)(1u<<13),13);
 run((uint16_t)(1u<<14),14);run((uint16_t)(1u<<15),15);
 struct track_chip_engine scratch;track_chip_init(&scratch);scratch.punch_param[8]=100u;
 memcpy(wave,source,sizeof wave);track_chip_process_punch(&scratch,0u,wave,6000u);
 memcpy(wave,source,sizeof wave);track_chip_process_punch(&scratch,(uint16_t)(1u<<8),wave,N);
 assert(changed()>100000u);
 printf("PASS V1.4.26: NOTE2 all 27 drums, STOP, native codec, 5 audible FX\n");return 0;
}
