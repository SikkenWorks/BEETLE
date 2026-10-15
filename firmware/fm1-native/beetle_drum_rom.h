/* BEETLE V0.44: original generated one-shot ROM, 22.05kHz 8-bit µ-law. */
#ifndef BEETLE_DRUM_ROM_H
#define BEETLE_DRUM_ROM_H
#include <stdint.h>
#define BEETLE_DRUM_ROM_BYTES 177943u
#define BEETLE_DRUM_ROM_RATE 22050u
#define BEETLE_DRUM_ROM_COUNT 16u
extern const uint8_t beetle_drum_rom[BEETLE_DRUM_ROM_BYTES];
extern const uint32_t beetle_drum_offsets[BEETLE_DRUM_ROM_COUNT+1u];
extern const int16_t beetle_ulaw_decode[256];
#endif
