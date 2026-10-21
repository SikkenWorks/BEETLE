/* TRACK V0.3 standalone four-channel mixer; no dynamic allocation / I/O.
 * SPDX-License-Identifier: GPL-3.0-only
 */
#ifndef TRACK_MIXER_H
#define TRACK_MIXER_H
#include <stdint.h>
#define TRACK_MIXER_CHANNELS TRACK_CHANNELS
#include "track_core.h"
#define TRACK_MIXER_FIELDS 5u /* VOL PAN MUTE SOLO SWING; CH label is not editable */
struct track_mixer_channel {uint8_t volume;int8_t pan;uint8_t mute,solo,swing;};
struct track_mixer {struct track_mixer_channel ch[TRACK_MIXER_CHANNELS];uint8_t row,col,quantize_mutes;};
/* ISR-observed post-lane-volume peaks, in actual signed-16 PCM units.
 * The UI clears these only after both LCD slices have completed. */
extern volatile uint16_t track_lane_pcm_peak[TRACK_CHANNELS];
void track_mixer_init(struct track_mixer *m);
void track_mixer_move(struct track_mixer *m,int32_t rows,int32_t columns);
int track_mixer_change(struct track_mixer *m,int32_t delta);
uint8_t track_mixer_velocity(const struct track_mixer *m,uint8_t ch,uint8_t input);
#endif
