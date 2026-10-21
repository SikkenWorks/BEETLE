/* BEETLE V0.69 instrument library V16 regression: project local and fail closed. */
#include "track_project_store.h"
#include "beetle_instrument_library.h"
#include "track_master_fx.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static struct track_store_project p,readback;
static uint8_t buffer[TRACK_STORE_MAX];
static void u32(uint8_t *b,uint32_t v){for(int i=0;i<4;i++)b[i]=(uint8_t)(v>>(8*i));}
static void seal(uint32_t n){u32(buffer+8,n);u32(buffer+12,track_store_crc32(buffer+24,n-24));u32(buffer+16,track_store_crc32(buffer,16));}
int main(void){
 struct track_engine e;track_init(&e);track_song_init(&p.song,&e);
 track_mixer_init(&p.mixer);track_master_default(&p.master);
 p.slot=0;p.bpm=120;p.mixer.quantize_mutes=1;
 for(uint8_t i=0;i<4;i++){
  p.instruments[i].volume=65;p.instruments[i].duty=50;
  p.instruments[i].cutoff=7000;p.instruments[i].type=i;
  track_va_patch_default(&p.instruments[i].va);
 }
 for(uint8_t i=0;i<TRACK_DRUM_COUNT;i++)track_drum_timbre_default(&p.drum[i]);
 beetle_library_init(&p.library);
 assert(!beetle_library_load(&p.library,0,&readback.instruments[0]));
 p.instruments[1].type=TRACK_CHIP_RESID_LITE;
 p.instruments[1].va.drive=37;p.instruments[1].va.mix=72;
 assert(beetle_library_store(&p.library,2,&p.instruments[1]));
 beetle_library_rename(&p.library,2,"DIRTY 1");
 assert(beetle_library_copy(&p.library,2,7));
 assert(beetle_library_valid(&p.library,7));
 assert(!beetle_library_copy(&p.library,6,0));
 p.instruments[1].va.drive=0;
 assert(beetle_library_load(&p.library,7,&p.instruments[1]));
 assert(p.instruments[1].va.drive==37);
 uint32_t n;
 assert(track_store_encode(&p,buffer,sizeof buffer,&n)==0);
 assert(buffer[4]==TRACK_STORE_VERSION&&n<TRACK_STORE_V4_MAX);
 assert(track_store_decode(&readback,buffer,n)==0);
 assert(readback.library.valid==(1u<<2 | 1u<<7));
 assert(readback.library.patch[2].type==TRACK_CHIP_RESID_LITE);
 assert(readback.library.patch[7].va.mix==72);
 assert(memcmp(readback.library.name[2],"DIRTY 1",7)==0);
 /* Older V15 contains no library bytes. This is a genuine version-shaped
  * V15 payload: strip all appended V16 bytes and recalculate both CRCs. */
 uint32_t old=n-(1u+2u*(8u+24u)); /* name8 + instrument24 */
 /* Determine V15 boundary from an otherwise-identical V16 empty-library
  * encoding rather than relying on C struct packing. */
 struct track_store_project copy=p;beetle_library_init(&copy.library);
 uint32_t empty_len;assert(track_store_encode(&copy,buffer,sizeof buffer,&empty_len)==0);
 assert(empty_len<n);
 buffer[4]=15;uint32_t old_len=empty_len-1u-8u;seal(old_len);
 assert(track_store_decode(&readback,buffer,old_len)==0);
 assert(readback.library.valid==0);
 (void)old;
 /* Rejected damaged records never silently restore a patch. */
 assert(track_store_encode(&p,buffer,sizeof buffer,&n)==0);
 buffer[n-1]^=0x01u;
 assert(track_store_decode(&readback,buffer,n)<0);
 puts("PASS V0.69 library save, copy, recall, names, V16 roundtrip, V15 migration, CRC guard");
}
