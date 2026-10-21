/* SPDX-License-Identifier: GPL-3.0-only
 * BEETLE NES APU-inspired, FM1-friendly fixed-point oscillator subset.
 * Historical/technical reference: Nes_Snd_Emu by Shay Green (blargg),
 * maintained at https://github.com/jamesathey/Nes_Snd_Emu (LGPL-2.1).
 * This is BEETLE's own C implementation; no upstream library code is
 * linked or incorporated. NOT a cycle-accurate NES APU emulator.
 * Three original APU voice concepts: 8-step duty pulse, 32-step 4-bit
 * triangle, and NTSC-period 15-bit/6-bit-tap noise. No DMC/expansions.
 * ISR-safe: no allocation, loops or division in the audio sample path.
 */
#ifndef BEETLE_NES_APU_H
#define BEETLE_NES_APU_H
#include <stdint.h>
#define BEETLE_NES_PULSE 9u
#define BEETLE_NES_TRIANGLE 10u
#define BEETLE_NES_NOISE 11u
#define BEETLE_NES_CLOCK_NTSC 1789773u
/* NES APU has four 8-stage pulse duty arrangements. Order corresponds to
 * 12.5%, 25%, 50% and 75% on-time, including the inverted 75% waveform. */
static inline int16_t beetle_nes_pulse(uint32_t phase,uint8_t duty){
 static const uint8_t seq[4]={0x02u,0x06u,0x1eu,0xf9u};
 uint8_t index=(uint8_t)(phase>>29u);
 return ((seq[duty&3u]>>index)&1u)?9600:-9600;
}
/* Authentic 32-step NES triangle DAC steps, 0..15..0; bipolar around DC. */
static inline int16_t beetle_nes_triangle(uint32_t phase){
 uint8_t k=(uint8_t)(phase>>27u);
 uint8_t dac=(k<16u)?(uint8_t)(15u-k):(uint8_t)(k-16u);
 return (int16_t)(((int32_t)dac*2-15)*650);
}
/* Noise register periods from the NTSC APU $400E register, in CPU cycles.
 * Higher MIDI notes choose shorter periods (16 pitches, as on hardware). */
static inline uint32_t beetle_nes_noise_increment(uint8_t midi_note){
 /* Offline computed from the NTSC 1.789773 MHz clock and APU periods.
  * Q32 increments at 44.1kHz; saturation for periods above Nyquist. */
 static const uint32_t inc_q32[16]={
  42848762u,85697524u,171563744u,228751659u,
  343127488u,458707274u,686254977u,862914674u,
  1089429776u,1361787220u,1815716293u,2723574440u,
  3631432587u,4294967295u,4294967295u,4294967295u
 };
 uint8_t slot=midi_note<=36u?0u:(uint8_t)((midi_note-36u)/4u);
 if(slot>15u)slot=15u;
 /* Compute in note-on/control path only; frequency is clock/period.
  * Limit impossible Q32 rates at 44.1kHz rather than integer overflow. */
 return inc_q32[slot];
}
/* 15-bit feedback with selectable short mode tap (bit 6 vs bit 1).
 * Caller accumulates phase increments; this function is step-triggered. */
static inline int16_t beetle_nes_noise(uint32_t *lfsr,uint8_t short_mode){
 uint32_t state=*lfsr & 0x7fffu;
 if(!state)state=1u;
 uint32_t bit=(state^(state>>(short_mode?6u:1u)))&1u;
 state=(state>>1u)|(bit<<14u);
 *lfsr=state;
 return (state&1u)?-8400:8400;
}
#endif
