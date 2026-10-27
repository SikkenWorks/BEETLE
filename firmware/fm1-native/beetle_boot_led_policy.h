/* BEETLE boot project chooser: only SELECT (FM1 button ID 3) is illuminated.
 * Pure policy so the runtime and host regression use identical conditions. */
#pragma once
#include <stdint.h>
#define BEETLE_START_SEL_LED_ID 3u
#define BEETLE_START_PROJECT_MENU 29u
static inline uint8_t beetle_start_led_only_sel(uint8_t page,uint32_t now,uint32_t splash_end){
 return (uint8_t)(page==BEETLE_START_PROJECT_MENU && (int32_t)(splash_end-now)<=0);
}
static inline uint8_t beetle_start_led_lit(uint8_t page,uint32_t now,uint32_t splash_end,uint8_t id){
 return (uint8_t)(beetle_start_led_only_sel(page,now,splash_end)&&id==BEETLE_START_SEL_LED_ID);
}
