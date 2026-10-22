#include "beetle_global_sample_route.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 for(unsigned note=0;note<14;note++)assert(beetle_global_sample_slot(note)==255u);
 for(unsigned note=14;note<27;note++){
  unsigned slot=note-14u;
  assert(beetle_global_sample_slot(note)==slot);
  assert(beetle_global_sample_triggerable(note,(uint16_t)(1u<<slot)));
  assert(!beetle_global_sample_triggerable(note,(uint16_t)(0x1fffu^(1u<<slot))));
 }
 for(unsigned note=27;note<256;note++)assert(beetle_global_sample_slot(note)==255u);
 puts("PASS: factory drums unchanged, 13 global user slots only on notes 14..26");
}
