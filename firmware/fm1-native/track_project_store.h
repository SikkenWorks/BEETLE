/* TRACK V0.14 isolated project serialization; NO flash writes. GPL-3.0-only. */
#ifndef TRACK_PROJECT_STORE_H
#define TRACK_PROJECT_STORE_H
#include <stdint.h>
#include "track_core.h"
#include "track_song.h"
#include "track_chip.h"
#include "track_drum.h"
#include "beetle_oneshots.h"
#include "track_mixer.h"
#include "beetle_performance_snapshots.h"
#include "beetle_mute_groups.h"
#define TRACK_STORE_MAGIC 0x314B5254u /* little endian "TRK1" */
#define TRACK_STORE_VERSION 23u
#define TRACK_STORE_MAX 24576u /* include legacy V1-V3 decode capacity */
#define TRACK_STORE_V4_MAX 12256u /* physical V4 payload budget */
struct track_store_instrument {
 uint8_t type,attack,decay,sustain,release,duty,volume;
 uint16_t cutoff;
 struct track_va_patch va;
};
#define BEETLE_LIBRARY_SLOTS 8u
#define BEETLE_LIBRARY_NAME 8u
struct beetle_instrument_library {
 uint8_t valid;
 char name[BEETLE_LIBRARY_SLOTS][BEETLE_LIBRARY_NAME];
 struct track_store_instrument patch[BEETLE_LIBRARY_SLOTS];
};
struct track_master_settings {uint8_t eq[4],eq_freq[4],eq_width[4],comp_threshold,comp_ratio,comp_gain,reverb_wet,reverb_feedback,reverb_tone,enabled[3],order[3],comp_attack,reverb_spread;};
struct track_store_project {
 struct track_store_instrument instruments[TRACK_CHIP_SLOTS];
 /* V17: independent TYPE settings in each of the four active patch slots. */
 uint16_t type_valid[TRACK_CHIP_SLOTS];
 struct track_store_instrument type_presets[TRACK_CHIP_SLOTS][TRACK_CHIP_TYPES];
 struct beetle_instrument_library library;
 struct track_song_bank song;
 struct track_mixer mixer;
 struct beetle_performance_snapshots snapshots;
 struct beetle_mute_groups groups;
 struct track_harmony harmony;
 struct track_drum_timbre drum[TRACK_DRUM_COUNT];
 struct beetle_oneshot_bank oneshots; /* V23 bounded project-owned µ-law bank */
 struct track_master_settings master;
 uint8_t punch_param[5]; /* Repeat, Ring Mod intensity, Filter, Tape, Stutter */
 char title[13];
 uint16_t bpm;
 uint8_t slot;
};
uint32_t track_store_crc32(const uint8_t *data,uint32_t n);
/* Encode/decode endian-defined data with CRC32; no samples or foreign objects. */
int track_store_encode(const struct track_store_project *p,uint8_t *out,uint32_t cap,uint32_t *used);
int track_store_decode(struct track_store_project *p,const uint8_t *data,uint32_t n);
/* Never write if a sector contains non-TRK1 content. Deliberately fail-closed. */
int track_store_can_claim(const uint8_t *sector,uint32_t size);
#endif
