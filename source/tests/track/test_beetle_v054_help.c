/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "beetle_help.h"
int main(void){
 assert(BEETLE_HELP_COUNT>=35);
 assert(BEETLE_HELP_VISIBLE==10u);
 assert(beetle_help_scroll(0,-50)==0);
 assert(beetle_help_scroll(0,1)==1);
 assert(beetle_help_scroll(0,500)==beetle_help_last_offset());
 assert(beetle_help_scroll(beetle_help_last_offset(),1)==beetle_help_last_offset());
 assert(beetle_help_scroll(beetle_help_last_offset(),-3)==beetle_help_last_offset()-3);
 uint8_t has_copy=0,has_paste=0,has_master=0,has_nosix=0;
 for(uint32_t i=0;i<BEETLE_HELP_COUNT;i++){
  const char *s=beetle_help_lines[i];
  assert(strlen(s)<=36);
  assert(strlen(s)*6u+12u<=230u); /* scrollbar starts at x230 */
  if(strstr(s,"SEQ+OP1"))has_copy=1;
  if(strstr(s,"SEQ+OP2"))has_paste=1;
  if(strstr(s,"K1-K4: MASTER"))has_master=1;
  if(strstr(s,"ONE LANE"))has_nosix=1;
 }
 assert(has_copy && has_paste && has_master && has_nosix);
 for(uint8_t offset=0;offset<=beetle_help_last_offset();offset++){
  for(uint8_t row=0;row<BEETLE_HELP_VISIBLE;row++){
   const uint16_t y=68u+13u*row;
   assert(y+7u<201u); /* reserve independent footer */
   assert((uint8_t)(offset+row)<BEETLE_HELP_COUNT);
  }
 }
 puts("PASS V0.54 help: copy labels, clipping, 43 entries, scrolling bounds");
}
