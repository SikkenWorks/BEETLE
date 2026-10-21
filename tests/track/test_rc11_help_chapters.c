/* SPDX-License-Identifier: GPL-3.0-only. Native HELP navigation regression. */
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "../../firmware/fm1-native/beetle_help.h"
int main(void){
 unsigned count=beetle_help_chapter_count();
 assert(count==13u);assert(BEETLE_HELP_VISIBLE==9u);
 assert(!strcmp(beetle_help_lines[0],"# TRACKER BASICS"));
 unsigned covered=0u;
 for(unsigned chapter=0u;chapter<count;chapter++){
  unsigned heading=beetle_help_chapter_heading((uint8_t)chapter);
  unsigned end=beetle_help_chapter_end((uint8_t)chapter);
  unsigned length=beetle_help_chapter_length((uint8_t)chapter);
  assert(beetle_help_lines[heading][0]=='#');
  assert(length>0u && heading+length+1u==end);
  assert(beetle_help_chapter_scroll((uint8_t)chapter,0u,10000)==beetle_help_chapter_maxscroll((uint8_t)chapter));
  assert(beetle_help_chapter_scroll((uint8_t)chapter,0u,-10000)==0u);
  for(unsigned j=heading+1u;j<end;j++){
   assert(beetle_help_lines[j][0]!='#');
   covered++;
  }
 }
 assert(beetle_help_chapter_shift(0u,-5)==0u);
 assert(beetle_help_chapter_shift(0u,100)==(uint8_t)(count-1u));
 assert(beetle_help_chapter_shift((uint8_t)(count-1u),-100)==0u);
 assert(!strcmp(beetle_help_lines[beetle_help_chapter_heading((uint8_t)(count-1u))],"# USB-C AUDIO"));
 assert(covered==BEETLE_HELP_COUNT-count);
 puts("PASS RC1.1: chapter browsing, boundaries, scrolling and USB help");
 return 0;
}
