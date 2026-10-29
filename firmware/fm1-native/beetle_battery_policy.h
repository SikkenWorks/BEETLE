/* SPDX-License-Identifier: GPL-3.0-only
 * FM1 PB1 ADC3 battery *voltage-level* display, not a fuel gauge.
 * Upstream reference: Felucca, firmware/src/ui_draw.c, batt_level(),
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments.
 * https://github.com/hugelton/Felucca ; GPL-3.0-only.
 * ADC3 raw cutoffs 531 / 561 / 591 are taken from that source.
 * SikkenWorks/BEETLE BLUE adds its own four-bar pixel presentation,
 * filtering, invalid-read handling and foreground polling.
 * These are broad voltage bands on the same M-VAVE FM1 hardware:
 * no chemistry-accurate percentage, charging-current or full-charge claim.
 * Some USB adapters do not enumerate and no charger-status GPIO is known.
 */
#ifndef BEETLE_BATTERY_POLICY_H
#define BEETLE_BATTERY_POLICY_H
#include <stdint.h>
#define BEETLE_BATTERY_20_ADC_RAW 531u /* compatibility name: LOW voltage boundary, NOT true 20% SoC */
#define BEETLE_BATTERY_MID_ADC_RAW 561u
#define BEETLE_BATTERY_HIGH_ADC_RAW 591u
/* An unconnected ADC or broken divider is not an empty/fully charged cell. */
static inline uint8_t beetle_battery_raw_valid(uint16_t raw){return (uint8_t)(raw>0u && raw<1023u);}
static inline uint16_t beetle_battery_filter(uint16_t prev,uint16_t raw,uint8_t valid){
 return valid?(uint16_t)(((uint32_t)prev*7u+raw+4u)/8u):raw;
}
static inline uint8_t beetle_battery_low20(uint16_t adc,uint8_t was_low){
 return (uint8_t)(adc<=(uint16_t)(BEETLE_BATTERY_20_ADC_RAW+(was_low?12u:0u)));
}
/* 0/1/3/4 uses Felucca's proven native 0..3 indication: empty, low,
 * two-thirds, full-voltage band. Do not invent interpolation thresholds. */
static inline uint8_t beetle_battery_bars(uint16_t adc){
 return adc>=BEETLE_BATTERY_HIGH_ADC_RAW?4u:
        adc>=BEETLE_BATTERY_MID_ADC_RAW?3u:
        adc>=BEETLE_BATTERY_20_ADC_RAW?1u:0u;
}
#endif
