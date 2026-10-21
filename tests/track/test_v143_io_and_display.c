#include <stdio.h>
#include <assert.h>
#include "beetle_input_safety.h"
#include "beetle_rc_diagnostics.h"
int main(void){
 struct beetle_io_input_gate g={0};
 beetle_io_input_begin(&g,1000u);
 assert(beetle_io_input_block(&g,1000u,1u<<9,0,0,0,0));
 assert(beetle_io_input_block(&g,1150u,0,1u<<4,0,1u<<4,0));
 assert(beetle_io_input_block(&g,1400u,0,0,0,0,1));
 assert(beetle_io_input_block(&g,1719u,0,0,0,0,0));
 assert(beetle_io_input_block(&g,1720u,0,0,0,0,0));
 assert(!g.active);
 assert(!beetle_io_input_block(&g,1721u,0,0,0,0,0));
 beetle_io_input_begin(&g,0xfffffff0u);
 assert(beetle_io_input_block(&g,0x30u,0,0,0,0,0));
 assert(beetle_io_input_block(&g,0x130u,0,0,0,0,0));
 assert(!g.active);
 assert(beetle_lcd_min_interval(0,1)==0);
 assert(beetle_lcd_min_interval(4,1)==125);
 assert(beetle_lcd_min_interval(10,1)==210);
 assert(beetle_lcd_min_interval(24,1)==380);
 assert(beetle_lcd_min_interval(24,0)==0);
 puts("PASS: all-control I/O quiet gate and measured-pressure LCD governor");
 return 0;
}
