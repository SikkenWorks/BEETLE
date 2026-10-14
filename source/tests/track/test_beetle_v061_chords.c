/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "beetle_pattern_length_shortcut.h"
#include "beetle_help.h"
int main(void){
 assert(!beetle_pattern_length_chord(0,0,0,-1));
 assert(!beetle_pattern_length_chord(19,(1u<<11),0,1));
 assert(beetle_pattern_length_chord(0,(1u<<11),0,1));
 assert(beetle_pattern_length_chord(0,0,(1u<<11),1));
 assert(!beetle_pattern_length_chord(0,(1u<<11),0,0));
 assert(beetle_pattern_length_target(16,5)==16);
 assert(beetle_pattern_length_target(1,-5)==1);
 assert(beetle_pattern_length_target(16,-2)==14);
 assert(beetle_pattern_length_target(3,2)==5);
 assert(beetle_help_scroll(0,1)==1);
 assert(beetle_help_scroll(0,-1)==0);
 puts("PASS V0.61 native chord inputs, length limits and HELP K1 range");
 return 0;
}
