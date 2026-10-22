/* GPL-3.0-only. Offline bridge for pinned-generation prefetch tests. */
#include "beetle_bd4_prefetch.h"
#include <stdint.h>
static struct beetle_bd4_stream lab;
void pin_reset(void){beetle_bd4_stream_reset(&lab);}
int pin_prepare(unsigned voice,uint8_t slot,uint32_t len,uint32_t gen){return beetle_bd4_stream_prepare_pinned(&lab,voice,slot,len,gen);}
int pin_prefetch(beetle_bd4_pinned_reader cb,void *ctx,unsigned budget){return beetle_bd4_prefetch_poll_pinned(&lab,cb,ctx,budget);}
int pin_unpinned_poll(beetle_bd4_blob_reader cb,void *ctx,unsigned budget){return beetle_bd4_prefetch_poll(&lab,cb,ctx,budget);}
int pin_start(unsigned voice){return beetle_bd4_stream_start(&lab,voice);}
void pin_disarm(unsigned voice){beetle_bd4_stream_disarm(&lab,voice);}
void pin_render(unsigned voice,int16_t *buffer,uint32_t n){beetle_bd4_stream_render(&lab,voice,buffer,n);}
uint32_t pin_underruns(unsigned voice){return lab.voices[voice].underruns;}
uint32_t pin_generation(unsigned voice){return lab.voices[voice].generation;}
