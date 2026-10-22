/* SPDX-License-Identifier: GPL-3.0-only
 * UI-only FM1 battery ADC policy. Threshold is a provisional raw ADC value;
 * measure real hardware voltage/ADC values before claiming true 20% SOC.
 * Threshold is an unverified 1-cell Li-ion 20% approximation. Real FM1
 * voltage/ADC calibration is needed before calling it exact.
 */
#ifndef BEETLE_BATTERY_POLICY_H
#define BEETLE_BATTERY_POLICY_H
#include <stdint.h>
/* ADC3 uses an unverified battery-divider ratio; tune using measured raw
 * ADC values from an actual FM1 at 20% remaining capacity. */
#ifndef BEETLE_BATTERY_20_ADC_RAW
#define BEETLE_BATTERY_20_ADC_RAW 550u
#endif
static inline uint16_t beetle_battery_filter(uint16_t prev,uint16_t raw,uint8_t valid){
 return valid?(uint16_t)(((uint32_t)prev*7u+raw+4u)/8u):raw;
}
static inline uint8_t beetle_battery_low20(uint16_t adc,uint8_t was_low){
 const uint16_t threshold=BEETLE_BATTERY_20_ADC_RAW;
 return (uint8_t)(threshold>0u && adc<=(uint16_t)(threshold+(was_low?12u:0u)));
}
#endif
