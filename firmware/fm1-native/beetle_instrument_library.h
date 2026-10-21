/* BEETLE V0.69 - project-local reusable instrument slots. GPL-3.0-only.
 * This module never writes flash; persistence is via the TRK1 V16 project codec.
 * The library is local to the project, NOT shared between hardware project slots.
 */
#ifndef BEETLE_INSTRUMENT_LIBRARY_H
#define BEETLE_INSTRUMENT_LIBRARY_H
#include "track_project_store.h"
static inline void beetle_library_init(struct beetle_instrument_library *lib){unsigned char *p=(unsigned char*)lib;for(unsigned i=0u;i<sizeof(*lib);i++)p[i]=0u;}
static inline uint8_t beetle_library_valid(const struct beetle_instrument_library *lib,uint8_t slot){return slot<BEETLE_LIBRARY_SLOTS&&!!(lib->valid&(1u<<slot));}
static inline uint8_t beetle_library_store(struct beetle_instrument_library *lib,uint8_t slot,const struct track_store_instrument *src){
 if(slot>=BEETLE_LIBRARY_SLOTS||!src||!track_va_patch_valid(&src->va)||src->type>=TRACK_CHIP_TYPES)return 0u;
 lib->patch[slot]=*src;lib->valid|=(uint8_t)(1u<<slot);return 1u;
}
static inline uint8_t beetle_library_load(const struct beetle_instrument_library *lib,uint8_t slot,struct track_store_instrument *dst){
 if(!dst||!beetle_library_valid(lib,slot))return 0u;
 *dst=lib->patch[slot];return 1u;
}
static inline uint8_t beetle_library_copy(struct beetle_instrument_library *lib,uint8_t from,uint8_t to){
 if(to>=BEETLE_LIBRARY_SLOTS||!beetle_library_valid(lib,from))return 0u;
 lib->patch[to]=lib->patch[from];for(uint8_t k=0u;k<BEETLE_LIBRARY_NAME;k++)lib->name[to][k]=lib->name[from][k];lib->valid|=(uint8_t)(1u<<to);return 1u;
}
static inline void beetle_library_rename(struct beetle_instrument_library *lib,uint8_t slot,const char *value){
 if(slot>=BEETLE_LIBRARY_SLOTS)return;
 uint8_t ended=0u;
 for(uint8_t i=0u;i<BEETLE_LIBRARY_NAME;i++){
  char c=' ';
  if(value&&!ended){c=value[i];if(!c){ended=1u;c=' ';}}
  lib->name[slot][i]=(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c==' '||c=='-'||c=='_'?c:' ';
 }
}
#endif
