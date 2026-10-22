/* SPDX-License-Identifier: GPL-3.0-only */
#include "beetle_global_bank_readonly.h"
#include <stdio.h>
#include <stdint.h>
static int get(void *c,uint32_t offset,uint8_t *dst,uint32_t n){FILE *f=(FILE*)c;
 if(fseek(f,(long)offset,SEEK_SET))return -1;
 return fread(dst,1u,n,f)==n?0:-1;
}
int main(int argc,char **argv){
 if(argc!=2)return 2;
 FILE *f=fopen(argv[1],"rb");if(!f)return 3;
 if(fseek(f,0,SEEK_END))return 3;
 long z=ftell(f);if(z<0||z>BEETLE_GBANK_MAX_FILE){fclose(f);return 4;}
 struct beetle_gbank_index index;
 int result=beetle_gbank_open_readonly(get,f,(uint32_t)z,&index);
 if(result){fprintf(stderr,"Rejected bank: %d\n",result);fclose(f);return 1;}
 printf("VALID %u total bytes, %u WAV bytes\n",index.bytes_total,index.bytes_audio);
 for(unsigned i=0;i<BEETLE_GBANK_SLOTS;i++){
  const struct beetle_gbank_slot *s=&index.slots[i];
  if(s->wav_bytes)printf("U%02u %s %u frames @ %u Hz, %u channels\n",i+1,s->name,s->frames,s->sample_rate,s->channels);
 }
 fclose(f);return 0;
}
